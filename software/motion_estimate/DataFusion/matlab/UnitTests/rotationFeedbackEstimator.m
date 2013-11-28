
clc
clear
% close all

% create biased rotations

% Assume in truth we are not rotating

% We propagate the true and biased rotation rates

% Estimator with orientation measurements


iterations = 10000;
dt = 0.001;

gn = [0,0,9.81]';

wbias = 0.00;

% Setup the datafusion variables
posterior.x = zeros(6,1);
posterior.P = 999*eye(6);
DFRESULTS.STATEX = zeros(iterations/20,6);
DFRESULTS.STATECOV = zeros(iterations/20,6);
index = 0;


% Prep data
traj.true.w_b = zeros(iterations,3);
traj.true.w_b(5001:5100,2) = +pi/4/100*1000;
traj.true.f_l = [1*ones(iterations,1) zeros(iterations,2)]; % forward left up frame

traj.true.bQl = [ones(iterations,1), zeros(iterations,3)];
traj.true.E = zeros(iterations,3);

traj.measured.w_b = traj.true.w_b;
traj.measured.w_b(:,2) = traj.true.w_b(:,2) + wbias; % this is gyro bias
traj.measured.a_b = traj.true.f_l; % this just initializes memory

traj.INS.bQl = [ones(iterations,1), zeros(iterations,3)];
traj.INS.E = zeros(iterations,3);

traj.INS.a_l = zeros(iterations,3);
traj.INS.f_l = zeros(iterations,3);


% Now we compute the true trajectory
for k = 2:iterations
    
    % Propagation of the true orientation
    tbRl = q2R(traj.true.bQl(k-1,:)');
    tbRl = closed_form_DCM_farrell(-dt*traj.true.w_b(k,:)', tbRl); % use negative rotations because here we are looking at the body to world rotations
    tlRb = tbRl';
    traj.true.bQl(k,:) = R2q(tbRl)';
    %disp(['true quaternion -- ' num2str(traj.true.bQl(k,:))]);
    traj.true.E(k,:) = q2e(traj.true.bQl(k,:))';
    traj.true.a_b(k,:) = (tlRb*(traj.true.f_l(k,:)' + gn))';
    traj.true.f_b(k,:) = (tlRb*(traj.true.f_l(k,:)'))';
    
    traj.measured.a_b(k,:) = traj.true.a_b(k,:);
    
    % this is the propagation of the observed orientation
    bRl = q2R(traj.INS.bQl(k-1,:)');
    bRl = closed_form_DCM_farrell(-dt*traj.measured.w_b(k,:)' , bRl);
    lRb = bRl';
    
    % Quaternion is Local to body
    traj.INS.bQl(k,:) = R2q(bRl);
%     disp(['true quaternion -- ' num2str(traj.INS.bQl(k,:))]);
    traj.INS.E(k,:) = q2e(traj.INS.bQl(k,:))';
    traj.INS.a_l(k,:) = (lRb'*traj.measured.a_b(k,:)')';
    traj.INS.f_l(k,:) = traj.INS.a_l(k,:) - gn';
    
    % rate change for the data fusion process
    if (mod(k, 20)==0)
        % gaan voort en pleeg vermenging
        index = index + 1;
        
        Sys.T = 0.02;% this should be taken from the utime stamps when ported to real data
        Sys.posterior = posterior;

        Measurement.quaternionManifoldResidual = R2q(  q2R(traj.true.bQl(k,:)')' * q2R(traj.INS.bQl(k,:)') );
        Measurement.INS.pose.bQl = traj.INS.bQl(k,:)';
        
        disp(['rotationFeedbackEstimator -- quaternion manifold residual: norm=' num2str(norm(Measurement.quaternionManifoldResidual)) ', q = ' num2str(Measurement.quaternionManifoldResidual)])
        [Result, dfSys] = iterate_rot_only([], Sys, Measurement);

        % Store stuff for later plotting
        DFRESULTS.STATEX(index,:) = dfSys.posterior.x';
        DFRESULTS.STATECOV(index,:) = diag(posterior.P);
        
    end
    
end

%%
figure(1), clf
subplot(321)
plot(traj.INS.E)
title('Estimated orientation')
grid on
subplot(323)
plot(traj.INS.a_l(:,1:2))
title('Estimated acceleration in local frame')
grid on
subplot(325)
plot(traj.INS.f_l)
title('Estimated force in the local frame')
grid on



subplot(322)
plot(traj.measured.a_b)
title('Measured Acceleration')
grid on

subplot(324)
plot(DFRESULTS.STATEX(:,1:3))
title('Estimated misalignment dE')
grid on

subplot(326)
plot(DFRESULTS.STATEX(:,4:6))
title('Bias estimate')
grid on

% True values

figure(2), clf
subplot(321), plot(traj.true.E)
title('True Euler angles')
grid on

subplot(322), plot(traj.true.f_l)
title('True local frame accelerations')
grid on

subplot(323), plot(traj.true.a_b)
title('True body accelerations')
grid on

subplot(324), plot(traj.true.f_b)
title('True body forces')
grid on



