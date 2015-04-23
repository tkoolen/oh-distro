#include "QPReactiveRecoveryPlan.hpp"
#include <unsupported/Eigen/Polynomials>
#include <Eigen/StdVector>
extern "C" {
  #include "iris_ldp/solver.h"
}

#define CVXGEN_MAX_ROWS 3
#define CVXGEN_MAX_PTS 8

Vars vars;
Params params;
Workspace work;
Settings settings;

VectorXd QPReactiveRecoveryPlan::closestPointInConvexHull(const Ref<const VectorXd> &x, const Ref<const MatrixXd> &V) {

  int dim = x.size();

  Matrix<double, CVXGEN_MAX_ROWS, 1> x_resized;
  Matrix<double, CVXGEN_MAX_ROWS, CVXGEN_MAX_PTS> V_resized;

  if (dim > CVXGEN_MAX_ROWS) {
    fprintf(stderr, "x can not be more than %d elements\n", CVXGEN_MAX_ROWS);
    exit(1);
  }
  if (V.rows() != dim) {
    fprintf(stderr, "V must have same number of rows as x\n");
    exit(1);
  }
  if (V.cols() > CVXGEN_MAX_PTS) {
    fprintf(stderr, "V can not be larger than %d x %d\n", CVXGEN_MAX_ROWS, CVXGEN_MAX_PTS);
    exit(1);
  }

  x_resized.head(dim) = x;
  V_resized.block(0, 0, dim, V.cols()) = V;

  if (dim < CVXGEN_MAX_ROWS) {
    x_resized.tail(CVXGEN_MAX_ROWS - dim) = VectorXd::Zero(CVXGEN_MAX_ROWS-dim);
    V_resized.block(dim, 0, CVXGEN_MAX_ROWS - dim, V_resized.cols()) = MatrixXd::Zero(CVXGEN_MAX_ROWS-dim, V_resized.cols());
  }

  for (int i=V.cols(); i < CVXGEN_MAX_PTS; i++) {
    for (int j=0; j < CVXGEN_MAX_ROWS; j++) {
      V_resized(j,i) = V_resized(j,i-1);
    }
  }

  for (int j=0; j < CVXGEN_MAX_PTS; j++) {
    V_resized.col(j) = V_resized.col(j) - x_resized;
  }

  set_defaults();
  setup_indexing();
  settings.verbose = 0;


  double *src = V_resized.data();
  double *dest = params.Y;
  for (int i=0; i < CVXGEN_MAX_ROWS * CVXGEN_MAX_PTS; i++) {
    *dest++ = *src++;
  }

  solve();


  Map<VectorXd>y(vars.v, CVXGEN_MAX_ROWS);
  Map<VectorXd>w(vars.w, CVXGEN_MAX_PTS);

  y = y + x.head(dim);
  return y.head(dim);
}

Isometry3d QPReactiveRecoveryPlan::closestPoseInConvexHull(const Isometry3d &pose, const Ref<const Matrix<double, 2, Dynamic>> &V) {
  Isometry3d new_pose(pose.rotation());
  Vector2d xy = QPReactiveRecoveryPlan::closestPointInConvexHull(pose.translation().head(2), V);
  new_pose.translate(Vector3d(xy(0), xy(1), pose.translation().z()));
  return new_pose;
}

Polynomial QPReactiveRecoveryPlan::bangBangPolynomial(double x0, double xd0, double u) {
  VectorXd coefs(3);
  coefs << x0 - 0.25*xd0*xd0/u,
           0.5*xd0,
           0.25*u;
  Polynomial p(coefs);
  return p;
}

std::vector<double> realRoots(Polynomial p) {
  VectorXd coefs = p.getCoefficients();
  double order = p.getOrder();
  std::vector<double> roots;
  if (order == 1) {
    // c0 + c1*t = 0;
    // t = -c0/c1
    roots.push_back(-coefs(0) / coefs(1));
  } else if (order == 2) {
    // c0 + c1*t + c2*t^2 = 0;
    // t = (-c1 +- sqrt(c1^2 - 4*c2*c0)) / (2*c2)
    double discriminant = pow(coefs(1), 2) - 4*coefs(2)*coefs(0);
    if (discriminant >= 0) {
      roots.push_back((-coefs(1) + sqrt(discriminant)) / (2*coefs(2)));
      roots.push_back((-coefs(1) - sqrt(discriminant)) / (2*coefs(2)));
    }
  } else {
    PolynomialSolver<double, Dynamic> poly_solver(coefs);
    poly_solver.realRoots(roots);
  }
  return roots;
}

std::vector<double> QPReactiveRecoveryPlan::expIntercept(const ExponentialForm &expform, double l0, double ld0, double u, int degree) {
  // Find the t >= 0 solutions to a*e^(b*t) + c == l0 + 1/2*ld0*t + 1/4*u*t^2 - 1/4*ld0^2/u
  // using a taylor expansion up to power [degree]

  Polynomial p_taylor = expform.taylorExpand(degree);
  VectorXd coefs = -QPReactiveRecoveryPlan::bangBangPolynomial(l0, ld0, u).getCoefficients();
  coefs.conservativeResize(6);
  coefs.tail(3).setZero();
  Polynomial p_bang(coefs);
  Polynomial p_int = p_taylor + p_bang;

  std::vector<double> roots = realRoots(p_int);
  std::vector<double> nonneg_roots;

  for (std::vector<double>::iterator it = roots.begin(); it != roots.end(); ++it) {
    if (*it > 0) {
      nonneg_roots.push_back(*it);
    } 
  }

  return nonneg_roots;
}

std::vector<BangBangIntercept> QPReactiveRecoveryPlan::bangBangIntercept(double x0, double xd0, double xf, double u_max) {
  std::vector<BangBangIntercept> intercepts;

  double us[2] = {u_max, -u_max};
  for (int i=0; i<2; i++) {
    double u = us[i];
    Polynomial p = QPReactiveRecoveryPlan::bangBangPolynomial(x0 - xf, xd0, u);
    std::vector<double> roots = realRoots(p);
    for (std::vector<double>::iterator it = roots.begin(); it != roots.end(); ++it) {
      double t = *it;
      if (t >= std::abs(xd0 / u)) {
        BangBangIntercept inter;
        inter.tf = t;
        inter.tswitch = 0.5 * (t - xd0 / u);
        inter.u = u;
        intercepts.push_back(inter);
        break;
      }
    }
  }
  return intercepts;
}

bool QPReactiveRecoveryPlan::isICPCaptured(Vector2d r_ic, std::map<FootID, FootState> foot_states, std::map<FootID, Matrix<double, 2, QP_REACTIVE_RECOVERY_VERTICES_PER_FOOT>> foot_vertices) {

  if (foot_states.size() != 2) {
    fprintf(stderr, "isICPCaptured doesn't yet support more than 2 feet\n");
    exit(1);
  }
  Matrix<double, 2, 8> all_vertices_in_world;

  int foot_count = 0;
  for (std::map<FootID, FootState>::iterator state = foot_states.begin(); state != foot_states.end(); ++state) {
    if (state->second.contact || 
        (state->second.pose.translation()(2) - state->second.terrain_height < this->capture_max_flyfoot_height)) {
      Matrix<double, 3, QP_REACTIVE_RECOVERY_VERTICES_PER_FOOT> foot_vertices_3d;
      auto vert_it = foot_vertices.find(state->first);
      if (vert_it == foot_vertices.end()) {
        fprintf(stderr, "Cannot find foot name: %s in foot_vertices\n", footIDToName[state->first].c_str());
        exit(1);
      }
      foot_vertices_3d.block(0,0,2,QP_REACTIVE_RECOVERY_VERTICES_PER_FOOT) = vert_it->second;
      foot_vertices_3d.row(2).setZero();
      foot_vertices_3d = foot_vertices_3d.array() * this->capture_shrink_factor;
      Matrix<double, 3, QP_REACTIVE_RECOVERY_VERTICES_PER_FOOT> foot_vertices_in_world;
      for (int i=0; i < 4; ++i) {
        foot_vertices_in_world.col(i) = state->second.pose * foot_vertices_3d.col(i);
      }
      all_vertices_in_world.block(0, QP_REACTIVE_RECOVERY_VERTICES_PER_FOOT*foot_count, 2, QP_REACTIVE_RECOVERY_VERTICES_PER_FOOT) = foot_vertices_in_world.block(0,0,2,QP_REACTIVE_RECOVERY_VERTICES_PER_FOOT);
    } else {
      // not captured unless both feet are down (or almost down), for stability purposes
      return false;
    }
    ++foot_count;
  }

  VectorXd r_ic_near = QPReactiveRecoveryPlan::closestPointInConvexHull(r_ic, all_vertices_in_world);
  if ((r_ic - r_ic_near).norm() < 1e-2) {
    return true;
  }
  return false;
}

ExponentialForm QPReactiveRecoveryPlan::icpTrajectory(double x_ic, double x_cop, double omega) {
  ExponentialForm icptraj((x_ic - x_cop), omega, x_cop);
  return icptraj;
}

bool tswitchComp(const BangBangIntercept int0, const BangBangIntercept int1) {
  return int0.tswitch < int1.tswitch;
}

Isometry3d QPReactiveRecoveryPlan::getTWorldToLocal(const Isometry3d &icp, const Isometry3d &cop) {
  Isometry3d T_world_to_local = Isometry3d::Identity();
  T_world_to_local.rotate(AngleAxis<double>(-std::atan2(icp.translation().y() - cop.translation().y(), icp.translation().x() - cop.translation().x()), Vector3d(0, 0, 1)));
  T_world_to_local.translate(-cop.translation());
  return T_world_to_local;
}

double QPReactiveRecoveryPlan::getMinTimeToXprimeAxis(const FootState foot_state, const BipedDescription &biped, Isometry3d &T_world_to_local) {
  std::vector<BangBangIntercept> xprime_axis_intercepts = 
    QPReactiveRecoveryPlan::bangBangIntercept((T_world_to_local * foot_state.pose).translation().y(),
                                              (T_world_to_local.linear() * foot_state.velocity.head(3))(1),
                                              0,
                                              biped.u_max);

  VectorXd times_to_xprime_axis(xprime_axis_intercepts.size());
  for (int i=0; i < xprime_axis_intercepts.size(); i++) {
    times_to_xprime_axis(i) = xprime_axis_intercepts[i].tf;
  }
  return times_to_xprime_axis.minCoeff();
}


std::vector<InterceptPlan> QPReactiveRecoveryPlan::getInterceptsWithCoP(const FootID &swing_foot, std::map<FootID, FootState> &foot_states, const BipedDescription &biped, const Isometry3d &icp, const Isometry3d &cop) {

  Isometry3d T_world_to_local = QPReactiveRecoveryPlan::getTWorldToLocal(icp, cop);

  double t_min_to_xprime = QPReactiveRecoveryPlan::getMinTimeToXprimeAxis(foot_states.find(swing_foot)->second, biped, T_world_to_local);

  double x0 = (T_world_to_local * foot_states[swing_foot].pose).translation().x();
  double xd0 = (T_world_to_local.linear() * foot_states[swing_foot].velocity.head(3))(0);

  std::vector<InterceptPlan> intercept_plans;

  double x_ic = (T_world_to_local * icp).translation()(0);
  double x_cop = 0; // by the definition of our local frame

  ExponentialForm icp_traj_in_local = QPReactiveRecoveryPlan::icpTrajectory(x_ic, x_cop, biped.omega);
  double x_ic_int = icp_traj_in_local.value(t_min_to_xprime) + this->desired_icp_offset;

  Polynomial x_foot_poly_plus = QPReactiveRecoveryPlan::bangBangPolynomial(x0, xd0, biped.u_max);
  Polynomial x_foot_poly_minus = QPReactiveRecoveryPlan::bangBangPolynomial(x0, xd0, -biped.u_max);
  Vector2d x_foot_int(x_foot_poly_plus.value(t_min_to_xprime), x_foot_poly_minus.value(t_min_to_xprime));

  if ((x_ic_int >= x_foot_int.minCoeff()) && (x_ic_int <= x_foot_int.maxCoeff())) {
    // The time to get onto the xprime axis dominates, and we can hit the ICP (plus offset) as soon as we get to that axis
    std::vector<BangBangIntercept> intercepts = QPReactiveRecoveryPlan::bangBangIntercept(x0, xd0, x_ic_int, biped.u_max);
    if (intercepts.size() > 0) {
      // if there are multiple options, take the one that switches sooner
      std::vector<BangBangIntercept>::iterator it_min = std::min_element(intercepts.begin(), intercepts.end(), tswitchComp);
      Isometry3d intercept_pose_in_local = Isometry3d::Identity();
      intercept_pose_in_local.translate(Vector3d(x_ic_int, 0, 0));
      Isometry3d intercept_pose_in_world = T_world_to_local.inverse() * intercept_pose_in_local;
      intercept_pose_in_world = QPReactiveRecoveryPlan::closestPoseInConvexHull(intercept_pose_in_world, biped.reachable_vertices.find(swing_foot)->second);
      InterceptPlan intercept_plan;
      intercept_plan.tf = t_min_to_xprime;
      intercept_plan.tswitch = it_min->tswitch;
      intercept_plan.pose_next = intercept_pose_in_world;
      intercept_plan.icp_plus_offset_next = intercept_pose_in_world;
      intercept_plan.cop = T_world_to_local.inverse();
      intercept_plan.swing_foot = swing_foot;
      intercept_plan.stance_foot = otherFoot[swing_foot];
      intercept_plan.error = 0; // to be filled in later
      intercept_plans.push_back(intercept_plan);
    }
  } else {
    std::vector<double> us = {biped.u_max, -biped.u_max};
    for (std::vector<double>::iterator u = us.begin(); u != us.end(); ++u) {
      std::vector<double> t_int = QPReactiveRecoveryPlan::expIntercept(icp_traj_in_local + this->desired_icp_offset, x0, xd0, *u, 7);
      std::vector<double> t_int_feasible;
      for (std::vector<double>::iterator it = t_int.begin(); it != t_int.end(); ++it) {
        if (*it >= t_min_to_xprime && *it >= std::abs(xd0 / (*u))) {
          t_int_feasible.push_back(*it);
        }
      }
      std::vector<Isometry3d, aligned_allocator<Isometry3d>> reachable_poses;
      // If there are no intercepts, get as close to our desired capture as possible within the reachable set. 
      // note: this might be off the xcop->xic line
      if (t_int_feasible.size() == 0) {
        Isometry3d reachable_pose_in_world = Isometry3d::Identity();
        reachable_pose_in_world.translate(Vector3d(x_ic + this->desired_icp_offset, 0, 0));
        reachable_pose_in_world = T_world_to_local.inverse() * reachable_pose_in_world;
        reachable_pose_in_world = QPReactiveRecoveryPlan::closestPoseInConvexHull(reachable_pose_in_world, biped.reachable_vertices.find(swing_foot)->second);
        reachable_poses.push_back(reachable_pose_in_world);
      } else {
        for (std::vector<double>::iterator t = t_int_feasible.begin(); t != t_int_feasible.end(); ++t) {
          Isometry3d reachable_pose_in_world = Isometry3d::Identity();
          reachable_pose_in_world.translate(Vector3d(icp_traj_in_local.value(*t) + this->desired_icp_offset, 0, 0));
          reachable_pose_in_world = T_world_to_local.inverse() * reachable_pose_in_world;
          reachable_pose_in_world = QPReactiveRecoveryPlan::closestPoseInConvexHull(reachable_pose_in_world, biped.reachable_vertices.find(swing_foot)->second);
          reachable_poses.push_back(reachable_pose_in_world);
        }
      }
      for (std::vector<Isometry3d, aligned_allocator<Isometry3d>>::iterator reachable_pose = reachable_poses.begin(); reachable_pose != reachable_poses.end(); ++ reachable_pose) {
        Isometry3d reachable_pose_in_local = T_world_to_local * (*reachable_pose);

        std::vector<BangBangIntercept> intercepts = QPReactiveRecoveryPlan::bangBangIntercept(x0, xd0, reachable_pose_in_local.translation().x(), biped.u_max);
        if (intercepts.size() > 0) {
          // if there are multiple options, take the one that switches sooner
          std::vector<BangBangIntercept>::iterator it_min = std::min_element(intercepts.begin(), intercepts.end(), tswitchComp);

          InterceptPlan intercept_plan;
          intercept_plan.tf = it_min->tf;
          intercept_plan.tswitch = it_min->tswitch;
          intercept_plan.pose_next = *reachable_pose;
          Isometry3d icp_plus_offset_next = Isometry3d::Identity();
          icp_plus_offset_next.translate(Vector3d(icp_traj_in_local.value(it_min->tf) + this->desired_icp_offset, 0, 0));
          icp_plus_offset_next = T_world_to_local.inverse() * icp_plus_offset_next;
          intercept_plan.icp_plus_offset_next = icp_plus_offset_next;
          intercept_plan.swing_foot = swing_foot;
          intercept_plan.stance_foot = otherFoot[swing_foot];
          intercept_plan.error = 0; // to be filled in later
        }
      }
    }
  }

  for (std::vector<InterceptPlan>::iterator it = intercept_plans.begin(); it != intercept_plans.end(); ++it) {
    it->error = (it->pose_next.translation().head(2) - it->icp_plus_offset_next.translation().head(2)).norm();
  }

  return intercept_plans;
}




