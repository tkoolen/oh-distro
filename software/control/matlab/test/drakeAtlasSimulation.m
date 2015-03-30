function drakeAtlasSimulation(atlas_version, visualize, add_hokuyo, right_hand, left_hand, world_name,box_height)
%NOTEST
if nargin < 1, atlas_version = 4; end
if nargin < 2, visualize = false; end
if nargin < 3, add_hokuyo = true; end
if nargin < 4, right_hand = 0; end
if nargin < 5, left_hand = 0; end
if nargin < 6, world_name = ''; end
if nargin < 7, box_height = 1.2; end

% IF YOU WANT MASS EST LOOK HERE
% (when this is more fleshed out this will become
% an argument)
use_mass_est = false;

% And if you want external wrench input on pelvis
use_external_wrench = true;

% silence some warnings
warning('off','Drake:RigidBodyManipulator:UnsupportedContactPoints')
warning('off','Drake:RigidBodyManipulator:UnsupportedJointLimits')
warning('off','Drake:RigidBodyManipulator:UnsupportedVelocityLimits')

% construct a pure atlas model for control
options.visualize = false; %don't use lcmgl lidar visualization... it's very slow
options.atlas_version = atlas_version;
options.floating = true;
options.dt = 0.00333;
options.hokuyo = false;
options.foot_force_sensors = false; % This works (you'll have to change
% LCMBroadcastBlock to broadcast them)
% but is slow right now.
options.obstacles = false; % Replaced by step terrain, though the option still works...
if (right_hand)
  options.hand_right = 'robotiq_weight_only';
end
if (left_hand)
  options.hand_left = 'robotiq_weight_only';
end
if (strcmp(world_name,'steps'))
  boxes = [1.0, 0.0, 1.2, 1, 0.15;
    1.2, 0.0, 0.8, 1, 0.30;];
  options.terrain = RigidBodyStepTerrain(boxes);
else
  options.terrain = RigidBodyFlatTerrain();
end
r_pure = DRCAtlas([],options);
% And construct a complete one
if (add_hokuyo)
  options.hokuyo = true;
  options.hokuyo_spin_rate = 4;
end
options.foot_force_sensors = false; % This works (you'll have to change
% LCMBroadcastBlock to broadcast them)
% but is slow right now.
sdfDir = fullfile(getDrakePath, 'examples', 'Atlas', 'sdf');
terrainSDF = fullfile(sdfDir,'drc_practice_task_2.world');

options.hand_right = getHandString(right_hand);
options.hand_left = getHandString(left_hand);
options.pelvis_wrench = use_external_wrench;

if (strcmp(world_name,'steps'))
  boxes = [1.0, 0.0, 1.2, 1, 0.15;
    1.2, 0.0, 0.8, 1, 0.30;];
  options.terrain = RigidBodyStepTerrain(boxes);
elseif (strcmp(world_name, 'terrain'))
  clear gazeboModelPath;
  setenv('GAZEBO_MODEL_PATH',sdfDir);
  height_map = RigidBodyHeightMapTerrain.constructHeightMapFromRaycast(RigidBodyManipulator(terrainSDF),[],-3:.015:10,-2:.015:2,10);
  options.terrain = height_map;
  options.use_bullet = false;
elseif (strcmp(world_name, 'stairs'))
  stairsRBM = RigidBodyManipulator();
  stairsRBM = stairsRBM.setTerrain(RigidBodyFlatTerrain());
  stairsRBM = stairsRBM.addRobotFromURDF('stairs.urdf', [1.5 ; 0.0; 0.0], [0 ; 0 ; pi]);
  stairsRBM = stairsRBM.compile();
  height_map = RigidBodyHeightMapTerrain.constructHeightMapFromRaycast(stairsRBM,[],-3:.015:3, -3:.015:3, 10);
  options.terrain = height_map;
  options.use_bullet = false;
else
  options.terrain = RigidBodyFlatTerrain();
end
options.replace_cylinders_with_capsules = false;
r_complete = DRCAtlas([],options);

% simplify collision to parts we care about
r_pure = r_pure.removeCollisionGroupsExcept({'heel','toe'});
r_complete = r_complete.removeCollisionGroupsExcept({'heel','toe', 'palm', 'knuckle', 'default'});
r_pure = compile(r_pure);

% we need to add a FT sensor frame
ft_link_ind = r_complete.getManipulator.findLinkId('r_hand');
ftframe = RigidBodyFrame(ft_link_ind,[0;0;0],[0;0;0],'force_torque_sensor');
r_complete = r_complete.addFrame(ftframe);
r_complete = r_complete.compile();

extra_height = 0;
% Add world if relevant
if (strcmp(world_name, 'valve_wall'))
  % Add valve DRC environment
  r_complete = r_complete.addRobotFromURDF('single_valve_wall.urdf', [1.0; 0.0; 0.0], [0;0;pi]);
elseif (strcmp(world_name, 'drill_frame'))
  r_complete = r_complete.addRobotFromURDF([getDrakePath(), '/examples/Atlas/urdf/drill_frame.urdf'], [1.0; 0.0; 0], [0;0;pi]);
  options_cyl.floating = true;
  r_complete = r_complete.addRobotFromURDF('table.urdf', [0.225; -1.5; 0.5]);
  r_complete = r_complete.addRobotFromURDF('drill_box.urdf', [0.225; -1.2; 1.2], [], options_cyl);
elseif (strcmp(world_name, 'door'))
  r_complete = r_complete.addRobotFromURDF([getDrakePath(), '/examples/Atlas/urdf/door.urdf'], [1.0; 0.0; 0], [0;0;pi]);
elseif (strcmp(world_name, 'manip_ex'))
  options_cyl.floating = true;
  r_complete = r_complete.addRobotFromURDF('table.urdf', [1.225; 0.0; 0.5]);
  r_complete = r_complete.addRobotFromURDF('drill_box.urdf', [0.775; -0.2; 1.2], [], options_cyl);
elseif(strcmp(world_name, 'terrain'))
  r_complete = r_complete.addRobotFromSDF(terrainSDF);
elseif(strcmp(world_name, 'plug'))
  options_cyl.floating = true;
  r_complete = r_complete.addRobotFromURDF('plug_frame.urdf', [1.0 ; 0.0 ; 0.0]);
  r_complete = r_complete.addRobotFromURDF('table.urdf', [0; -1.5; 0.5]);
  r_complete = r_complete.addRobotFromURDF('big_plug.urdf', [-0.2; -1.2; 1.2], [], options_cyl);
  r_complete = r_complete.addRobotFromURDF('small_plug.urdf', [0.2; -1.2; 1.2], [], options_cyl);
elseif(strcmp(world_name, 'stairs'))
  r_complete = r_complete.addRobotFromURDF('stairs.urdf', [1.5 ; 0.0; 0.0], [0 ; 0 ; pi]);
elseif(strcmp(world_name, 'runningboard'))  
  r_complete = r_complete.addRobotFromURDF('springboard.urdf', [0.0 ; 0.0; 0.1], [0 ; 0 ; pi]);
  extra_height = 0.12;
elseif(strcmp(world_name,'box'))
  box = RigidBodyBox([1;2;box_height;]);
  r_complete = r_complete.addCollisionGeometryToBody(1,box);
  r_complete = r_complete.addVisualGeometryToBody(1,box);
  handle = addpathTemporary([getenv('DRC_BASE'),'/software/control/matlab/planners/prone']);
  kpt = KinematicPoseTrajectory(r_complete,{});
  r_complete = kpt.addSpecifiedCollisionGeometryToRobot({'l_fpelvis','r_fpelvis'},r_complete);  
end
r_complete = compile(r_complete);

% set initial state to fixed point
S = load(r_pure.fixed_point_file);
xstar = S.xstar;
xstar(1) = 0;
xstar(2) = 0;
xstar(3) = xstar(3) + 0.08 + extra_height ;
xstar(6) = 0;
x0 = zeros(r_pure.getNumStates, 1);
x0(1:length(xstar)) = xstar;
r_pure = r_pure.setInitialState(x0);

% load the correct initial state in the box world
if strcmp(world_name,'box')
  % load the correct fixed point file
  handle = addpathTemporary([getenv('DRC_BASE'),'/software/control/matlab/planners/chair_standup']);
  if atlas_version == 4
    chair_data = load('chair_standup_data.mat');
  else
    chair_data = load('chair_data_v5.mat');
  end
    
  qstar = chair_data.q_sol(:,3);
  xstar = [qstar;0*qstar];
  xstar(3) = xstar(3) + 0.08;
  x0 = zeros(r_pure.getNumStates, 1);
  x0(1:length(xstar)) = xstar;
  r_pure = r_pure.setInitialState(x0);
end

% and complete state to a feasible sol
% so lcp has an easier time
xstar_complete = zeros(r_complete.getNumStates(), 1);
xstar_complete(1:length(xstar)) = xstar;
xstar_complete = r_complete.resolveConstraints(xstar_complete);
r_complete = r_complete.setInitialState(xstar_complete);

done = 0;
while(~done)
  % mass est
  if (use_mass_est)
    sys = mimoCascade(r_complete, MassEstimationBlock(r_complete, r_pure, ftframe));
  else
    sys = r_complete;
  end
  
  % Pass through outputs from robot
  for i=1:r_complete.getOutputFrame.getNumFrames
    outs(i).system = 2;
    outs(i).output = i;
  end
  % LCM intrepret Atlas commands
  lcmInputBlock = LCMInputFromAtlasCommandBlock(r_complete,r_pure,options);
  sys = mimoFeedback(lcmInputBlock, sys, [], [], [], outs);
  % LCM interpret in for hand
  if (right_hand > 0)
    %lcmRobotiqInputBlock = LCMInputFromRobotiqCommandBlockTendons(r_complete, options);
    lcmRobotiqInputBlock_right = getHandDriver(right_hand, r_complete, 'right', options);
    sys = mimoFeedback(lcmRobotiqInputBlock_right, sys, [], [], [], outs);
  end
  if (left_hand > 0)
    %lcmRobotiqInputBlock = LCMInputFromRobotiqCommandBlockTendons(r_complete, options);
    lcmRobotiqInputBlock_left = getHandDriver(right_hand, r_complete, 'left', options);
    sys = mimoFeedback(lcmRobotiqInputBlock_left, sys, [], [], [], outs);
  end

  % LCM interpret in for the force/torque type
  if (use_external_wrench)
    lcmFTBlock = LCMInputFromForceTorqueBlock(r_complete);
    sys = mimoCascade(lcmFTBlock, sys);
  end

  % LCM broadcast out
  broadcast_opts = options;
  broadcast_opts.publish_truth = 0;
  broadcast_opts.publish_imu = 1;
  lcmBroadcastBlock = LCMBroadcastBlock(r_complete,r_pure, broadcast_opts);
  sys = mimoCascade(sys, lcmBroadcastBlock);
  
  % Visualize if desired
  if visualize
    v = r_complete.constructVisualizer;
    v.display_dt = 0.01;
    S=warning('off','Drake:DrakeSystem:UnsupportedSampleTime');
    output_select(1).system=1;
    output_select(1).output=1;
    sys = mimoCascade(sys,v,[],[],output_select);
    warning(S);
  end
  try
    simulate(sys,[0.0,Inf], xstar_complete, options);
  catch err
    disp('caught error in simulate(): restarting sim');
  end
end
end

function handString = getHandString(hand_id)
switch hand_id
  case 1
    handString = 'robotiq';
  case 2
    handString = 'robotiq_tendons';
  case 3
    handString = 'robotiq_simple';
  case -1
    handString = 'robotiq_weight_only';
  otherwise
    handString = 'none';
end
end

function handDriver = getHandDriver(hand_id, r_complete, handedness, options)
switch hand_id
  case 1
    handDriver = LCMInputFromRobotiqCommandBlock(r_complete, handedness, options);
  case 2
    handDriver = LCMInputFromRobotiqCommandBlockTendons(r_complete, handedness, options);
  case 3
    handDriver = LCMInputFromRobotiqCommandBlockSimplePD(r_complete, handedness, options);
  otherwise
    handDriver = [];
    disp('unexpected hand type, should be {1, 2, 3}')
end
end