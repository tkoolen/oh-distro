function runPinnedReachingLCM
%NOTEST

options.floating = false;
options.dt = 0.001;
r = Atlas('../models/mit_gazebo_models/mit_robot_drake/model_minimal_contact.urdf',options);

% set initial state to fixed point
load('data/atlas_pinned_config.mat');
r = r.setInitialState(xstar);

% set initial conditions in gazebo
% NOTE: gazebo must be running for this to work
state_frame = r.getStateFrame();
state_frame.publish(0,xstar,'SET_ROBOT_CONFIG');

[Kp,Kd] = getPDGains(r);
sys = pdcontrol(r,Kp,Kd);

sys = PinnedEndEffectorControl(sys,r);

% create end effectors
joint_names = r.getJointNames();
joint_names = joint_names(2:end); % get rid of null string at beginning..
right_ee = EndEffector(r,'atlas','r_hand',[0;0;0],'R_HAND_GOAL');
right_ee = right_ee.setMask(~cellfun(@isempty,strfind(joint_names,'r_arm')));
left_ee = EndEffector(r,'atlas','l_hand',[0;0;0],'L_HAND_GOAL');
left_ee = left_ee.setMask(~cellfun(@isempty,strfind(joint_names,'l_arm')));
head_ee = EndEffector(r,'atlas','head',[0.1;0;0],'GAZE_GOAL');
headmask = ~cellfun(@isempty,strfind(joint_names,'neck')) + ...
          ~cellfun(@isempty,strfind(joint_names,'back_lbz'));
head_ee = head_ee.setMask(headmask);
head_ee = head_ee.setGain(0.25);

% add end effectors
sys = sys.addEndEffector(right_ee);
sys = sys.addEndEffector(left_ee);
sys = sys.addEndEffector(head_ee);

% give fixed nominal position. Note this can be removed and nominal configs
% can be sent over LCM too.
x0 = r.getInitialState();
qgen = ConstOrPassthroughSystem(x0(1:r.getNumStates()/2));
qgen = qgen.setOutputFrame(sys.getInputFrame.frame{4});

% set up MIMO connections
connection(1).from_output = 1;
connection(1).to_input = 4;
ins(1).system = 2;
ins(1).input = 1;
ins(2).system = 2;
ins(2).input = 2;
ins(3).system = 2;
ins(3).input = 3;
ins(4).system = 2;
ins(4).input = 5;
outs(1).system = 2;
outs(1).output = 1;
sys = mimoCascade(qgen,sys,connection,ins,outs);

options.timekeeper = 'drake/lcmTimeKeeper'; 
runLCM(sys,[],options);

end