#!/usr/bin/python
# todo: add the pelvis pitch to the yaw from the gui

import os,sys
import lcm
import time
from lcm import LCM
import math
from threading import Thread

home_dir =os.getenv("DRC_BASE")
#print home_dir
sys.path.append(home_dir + "/software/build/lib/python2.7/site-packages")
sys.path.append(home_dir + "/software/build/lib/python2.7/dist-packages")
from drc.robot_state_t import robot_state_t
from drc.position_3d_t import position_3d_t
from drc.vector_3d_t import vector_3d_t
from drc.quaternion_t import quaternion_t
from drc.twist_t import twist_t
from drc.force_torque_t import force_torque_t
from drc.walking_goal_t import walking_goal_t
from drc.atlas_behavior_manipulate_params_t import atlas_behavior_manipulate_params_t
########################################################################################

def timestamp_now (): return int (time.time () * 1000000)

def quat_to_euler(q) :
  roll_a = 2.0 * (q[0]*q[1] + q[2]*q[3]);
  roll_b = 1.0 - 2.0 * (q[1]*q[1] + q[2]*q[2]);
  roll = math.atan2 (roll_a, roll_b);

  pitch_sin = 2.0 * (q[0]*q[2] - q[3]*q[1]);
  pitch = math.asin (pitch_sin);

  yaw_a = 2.0 * (q[0]*q[3] + q[1]*q[2]);
  yaw_b = 1.0 - 2.0 * (q[2]*q[2] + q[3]*q[3]);  
  yaw = math.atan2 (yaw_a, yaw_b);
  return [roll,pitch,yaw]

def euler_to_quat(roll, pitch, yaw):
  sy = math.sin(yaw*0.5);
  cy = math.cos(yaw*0.5);
  sp = math.sin(pitch*0.5);
  cp = math.cos(pitch*0.5);
  sr = math.sin(roll*0.5);
  cr = math.cos(roll*0.5);
  w = cr*cp*cy + sr*sp*sy;
  x = sr*cp*cy - cr*sp*sy;
  y = cr*sp*cy + sr*cp*sy;
  z = cr*cp*sy - sr*sp*cy;
  return [w,x,y,z]


def setStateAtHeight85BackAndPelvisPitched(msg):
  msg.pose.translation.z = 0.855
  # convert input rotation to euler
  quat_in= [msg.pose.rotation.w, msg.pose.rotation.x, msg.pose.rotation.y, msg.pose.rotation.z]
  [roll,pitch,yaw]=quat_to_euler(quat_in)
  nominal_rpy = [0.01269, 0.2714, yaw] # combine yaw with nominal leaning of robot:
  quat_out = euler_to_quat(nominal_rpy[0], nominal_rpy[1], nominal_rpy[2])
  msg.pose.rotation.w = quat_out[0]
  msg.pose.rotation.x = quat_out[1]
  msg.pose.rotation.y = quat_out[2]
  msg.pose.rotation.z = quat_out[3]
  msg.joint_position = [0.0255525112152, 0.441202163696, 0.00035834312439, 0.0319782495499, 0.0157958865166, 0.0383446216583, -0.91215968132,\
                        0.951368033886, -0.315932631493, -0.0551896877587, -0.0183086395264, -0.0554901361465, -0.92648422718, 0.953379929066,\
                        -0.280878514051, 0.0627506822348, 0.27154108882, -1.361130476, 2.00148749352, 0.515956759453, 0.0401229038835, \
                        0.000248298572842, 0.269899457693, 1.37451326847, 2.04671573639, -0.454142481089, 0.00692522525787, 0.00274819415063]
  return msg

def setStateAtHeight86(msg):
  msg.pose.translation.z = 0.858534753323
  # orientation, 0.00342719585229, 0.00504788873763, 0.103853363782,
  msg.joint_position = [0.00432538986206, 0.000792384147644, -0.0010027885437, 1.14905309677, 0.000830829143524, 0.0304319858551, -0.495565026999, \
                        1.01119089127, -0.54089897871, -0.0268067382276, -0.00381422042847, -0.0367293357849, -0.494196861982, 0.994021117687, \
                        -0.524335980415, 0.0539663769305, 0.280060052872, -1.30356013775, 2.00733232498, 0.483818888664, 0.0282346289605, \
                        -0.00199056160636, 0.309303581715, 1.35600960255, 2.05246806145, -0.442733466625, 0.00692522525787, 0.145126670599 ]
  return msg

def setStateAtHeight80(msg):
  msg.pose.translation.z = 0.794386198521
  # -0.00225271177662, 0.00527569419577, 0.094212812089, 

  msg.joint_position = [0.004677157402, 0.00342667102814, -0.00474905967712, 1.14958667755, 0.00408941507339, 0.0368480682373, \
                        -0.681962370872, 1.34392225742, -0.680823981762, -0.0329969972372, -0.00800085067749, -0.0247997045517, \
                        -0.650174021721, 1.32475161552, -0.687993586063, 0.0449704453349, 0.256810456514, -1.30653166771, 2.03570413589, \
                        0.458308488131, 0.0489153526723, -0.0120663093403, 0.30805721879, 1.36914455891, 2.0452775, -0.40361696, 0.0068293809, -1.0575143]
  return msg

def setStateAtHeight75(msg):
  msg.pose.translation.z = 0.732506911755
  #, -0.000808249484968, -0.00102079036455, 0.0933028354976, 
  msg.joint_position = [0.004677157402, 0.00316667556763, -0.00521075725555, 1.14958667755, 0.00421768426895, 0.0511291027069, \
                        -0.804708003998, 1.56003177166, -0.770609498024, -0.0488710962236, -0.0078706741333, -0.0163278579712, \
                        -0.775656223297, 1.54737138748, -0.783533096313, 0.0399675630033, 0.255834490061, -1.30554103851, 2.036921978, \
                        0.455057352781, 0.053745046258, -0.0125637603924, 0.30805721879, 1.37250006199, 2.04537343979, -0.403616964, 0.00682938, -1.05751430]
  return msg

def setStateAtHeight70(msg):
  msg.pose.translation.z = 0.700085043907
  #-0.00262847501533, 0.00501577123118, 0.0927118574385,
  msg.joint_position = [0.004677157402, 0.00349199771881, -0.00572419166565, 1.14974021912, 0.00549167394638, 0.0557951927185, \
                        -0.906586408615, 1.75183320045, -0.869903743267, -0.0556065551937, -0.00715255737305, -0.0153464078903, \
                        -0.873906850815, 1.73374772072, -0.883218050003, 0.0490799583495, 0.255567997694, -1.30838859081, 2.03509545326, \
                        0.451305687428, 0.050525251776, -0.0110711865127, 0.307961344, 1.37230837345, 2.04546928, -0.404479831, 0.0068293, -1.05751430988]
  return msg

def setStateAtHeight66(msg):
  msg.pose.translation.z = 0.655467739105
  # -0.00185047180292, 0.00574357871503, 0.0934642327473,
  msg.joint_position = [0.004677157402, 0.00485932826996, -0.00669956207275, 1.14996910095, 0.00459951162338, 0.0668022632599, \
                        -0.965375542641, 1.86640143394, -0.928911805153, -0.0676346570253, -0.00748014450073, -0.0108672380447, \
                        -0.932114124298, 1.85122585297, -0.945250093937, 0.0485419183969, 0.248025298119, -1.30591237545, 2.04970741272, \
                        0.446678996086, 0.0600606650114, -0.0167930833995, 0.30805721879, 1.37700617313, 2.04546928406, -0.38981112, 0.006733536, -1.032395]
  return msg



def appendSandiaJoints(msg):
  msg.num_joints = msg.num_joints + 24
  msg.joint_position.extend ( [0]*24 )
  msg.joint_velocity.extend ( [0]*24 )
  msg.joint_effort.extend ( [0]*24 )

  msg.joint_name.extend( ["left_f0_j0","left_f0_j1","left_f0_j2",   "left_f1_j0","left_f1_j1","left_f1_j2",\
    "left_f2_j0","left_f2_j1","left_f2_j2",   "left_f3_j0","left_f3_j1","left_f3_j2" ] )
  msg.joint_name.extend( ["right_f0_j0","right_f0_j1","right_f0_j2",  "right_f1_j0","right_f1_j1","right_f1_j2", \
    "right_f2_j0","right_f2_j1","right_f2_j2",  "right_f3_j0","right_f3_j1","right_f3_j2" ] )
  return msg

def appendIrobotJoints(msg):
  msg.num_joints = msg.num_joints + 16
  msg.joint_position.extend ( [0]*16 )
  msg.joint_velocity.extend ( [0]*16 )
  msg.joint_effort.extend ( [0]*16 )

  msg.joint_name.extend( ["left_finger[0]/joint_base_rotation", "left_finger[0]/joint_base", "left_finger[0]/joint_flex", \
      "left_finger[1]/joint_base_rotation", "left_finger[1]/joint_base", "left_finger[1]/joint_flex", \
      "left_finger[2]/joint_base", "left_finger[2]/joint_flex" ] )
  msg.joint_name.extend( ["right_finger[0]/joint_base_rotation", "right_finger[0]/joint_base", "right_finger[0]/joint_flex",\
      "right_finger[1]/joint_base_rotation", "right_finger[1]/joint_base", "right_finger[1]/joint_flex",\
      "right_finger[2]/joint_base", "right_finger[2]/joint_flex" ] )
  return msg

def appendHeadJoints(msg):
  msg.num_joints = msg.num_joints + 13
  msg.joint_position.extend ( [0]*13 )
  msg.joint_velocity.extend ( [0]*13 )
  msg.joint_effort.extend ( [0]*13 )

  msg.joint_name.extend( ["pre_spindle_cal_x_joint", "pre_spindle_cal_y_joint", "pre_spindle_cal_z_joint", \
      "pre_spindle_cal_roll_joint", "pre_spindle_cal_pitch_joint", "pre_spindle_cal_yaw_joint", \
      "hokuyo_joint", \
      "post_spindle_cal_x_joint", "post_spindle_cal_y_joint", "post_spindle_cal_z_joint", \
      "post_spindle_cal_roll_joint", "post_spindle_cal_pitch_joint", "post_spindle_cal_yaw_joint" ])
  return msg

def quat_to_euler(q) :
  
  roll_a = 2.0 * (q[0]*q[1] + q[2]*q[3]);
  roll_b = 1.0 - 2.0 * (q[1]*q[1] + q[2]*q[2]);
  roll = math.atan2 (roll_a, roll_b);

  pitch_sin = 2.0 * (q[0]*q[2] - q[3]*q[1]);
  pitch = math.asin (pitch_sin);

  yaw_a = 2.0 * (q[0]*q[3] + q[1]*q[2]);
  yaw_b = 1.0 - 2.0 * (q[2]*q[2] + q[3]*q[3]);  
  yaw = math.atan2 (yaw_a, yaw_b);
  
  #a = q[0]
  #b = q[1]
  #c = q[2]  
  #d = q[3]  
  #roll = math.atan2(2.0*(c*d + b*a),a*a-b*b-c*c+d*d);
  #pitch = math.asin(-2.0*(b*d - a*c));
  #yaw = math.atan2(2.0*(b*c + d*a),a*a+b*b-c*c-d*d);
  
  #roll = math.atan2( 2*(q[0]*q[1]+q[2]*q[3]), 1-2*(q[1]*q[1]+q[2]*q[2]));
  #pitch = math.asin(2*(q[0]*q[2]-q[3]*q[1]));
  #yaw = math.atan2(2*(q[0]*q[3]+q[1]*q[2]), 1-2*(q[2]*q[2]+q[3]*q[3]));
  return [roll,pitch,yaw]

def getPoseMsg():
  pose = position_3d_t()
  translation = vector_3d_t()
  rotation = quaternion_t()
  rotation.w = 1
  pose.translation = translation
  pose.rotation = rotation
  return pose

def getTwistMsg():
  twist = twist_t()
  linv = vector_3d_t()
  angv = vector_3d_t()
  twist.linear_velocity = linv
  twist.angular_velocity = angv
  return twist

def getRobotStateMsg():
  msg = robot_state_t()
  msg.utime = timestamp_now ()
  
  ft = force_torque_t()
  msg.pose = getPoseMsg()
  msg.twist = getTwistMsg()
  msg.force_torque = ft

  msg.num_joints = 28
  msg.joint_position = [0]*28
  msg.joint_velocity = [0]*28
  msg.joint_effort = [0]*28

  msg.joint_name = ['back_bkz', 'back_bky', 'back_bkx', 'neck_ay', 'l_leg_hpz', 'l_leg_hpx',\
                    'l_leg_hpy', 'l_leg_kny', 'l_leg_aky', 'l_leg_akx', 'r_leg_hpz', 'r_leg_hpx',\
                    'r_leg_hpy', 'r_leg_kny', 'r_leg_aky', 'r_leg_akx', 'l_arm_usy', 'l_arm_shx',\
                    'l_arm_ely',      'l_arm_elx', 'l_arm_uwy', 'l_arm_mwx', 'r_arm_usy', 'r_arm_shx',\
                    'r_arm_ely', 'r_arm_elx', 'r_arm_uwy', 'r_arm_mwx']
  return msg  
  
def sendRobotStateMsg():
  global goal_pelvis_height, goal_pelvis_pitch, goal_pos, goal_xy
  msg = getRobotStateMsg()
  quat_out = euler_to_quat(0,0, goal_yaw)
  msg.pose.rotation.w = quat_out[0]
  msg.pose.rotation.x = quat_out[1]
  msg.pose.rotation.y = quat_out[2]
  msg.pose.rotation.z = quat_out[3]
  msg.pose.translation.x = goal_xy[0]
  msg.pose.translation.y = goal_xy[1]
  print "Pelvis Height: " , goal_pelvis_height
  if (goal_pelvis_pitch > 0.1):
    print "Using Leaning Pitch"
    msg = setStateAtHeight85BackAndPelvisPitched(msg)
  elif (goal_pelvis_height >  0.83 ):
    msg = setStateAtHeight86(msg)
  elif (goal_pelvis_height >  0.775 ):
    msg = setStateAtHeight80(msg)
  elif (goal_pelvis_height >  0.725 ):
    msg = setStateAtHeight75(msg)
  elif (goal_pelvis_height >  0.68 ):
    msg = setStateAtHeight70(msg)
  else:
    msg = setStateAtHeight66(msg)

  msg = appendHeadJoints(msg)
  if (hand_type == "sandia"):
    msg = appendSandiaJoints(msg)
  else:
    msg = appendIrobotJoints(msg)
  lc.publish("EST_ROBOT_STATE", msg.encode())


def on_manip_params(channel, data):
  global goal_pelvis_height,goal_pelvis_pitch, goal_yaw, goal_xy
  m = atlas_behavior_manipulate_params_t.decode(data)
  goal_pelvis_height = m.desired.pelvis_height
  goal_pelvis_pitch = m.desired.pelvis_pitch
  print "Changing height and pitch"
  sendRobotStateMsg()
  
def on_walking_goal(channel, data):
  global goal_pelvis_height, goal_yaw, goal_xy
  m = walking_goal_t.decode(data)
  quat_in= [m.goal_pos.rotation.w, m.goal_pos.rotation.x, m.goal_pos.rotation.y, m.goal_pos.rotation.z]
  [roll,pitch,goal_yaw]=quat_to_euler(quat_in)
  goal_xy = [m.goal_pos.translation.x, m.goal_pos.translation.y]
  sendRobotStateMsg()

  

#################################################################################

print 'drc-fake-robot-state [irobot|sandia]'
#print 'Number of arguments:', len(sys.argv), 'arguments.'
#print 'Argument List:', str(sys.argv)

hand_type = "sandia"

if (len(sys.argv)>=2):
  hand_type = sys.argv[1]
  print "hand_type:", hand_type
else:
  print "hand_type defaulting to ", hand_type


lc = lcm.LCM()
print "started"

goal_xy = [0,0]
goal_yaw = 0
goal_pelvis_height = 0
goal_pelvis_pitch = 0




def lcm_thread():
  sub1 = lc.subscribe("ATLAS_MANIPULATE_PARAMS", on_manip_params) 
  sub2 = lc.subscribe("WALKING_GOAL", on_walking_goal) 
  while True:
    ## Handle LCM if new messages have arrived.
    lc.handle()

  lc.unsubscribe(sub1)
  lc.unsubscribe(sub2)

t2 = Thread(target=lcm_thread)
t2.start()

sleep_timing=0.5 # time between updates of the plots - in wall time
while (1==1):
  time.sleep(sleep_timing)
  sendRobotStateMsg()

