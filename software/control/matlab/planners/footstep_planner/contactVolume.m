function [contact_length, contact_width, contact_height] = contactVolume(biped, step1, step2, options)

if nargin < 4; options = struct(); end
if ~isfield(options, 'planar_clearance'); options.planar_clearance = 0.05; end
if ~isfield(options, 'nom_z_clearance'); options.nom_z_clearance = 0.05; end

step2.pos(6) = step1.pos(6) + angleDiff(step1.pos(6), step2.pos(6));

swing_angle = atan2(step2.pos(2) - step1.pos(2), step2.pos(1) - step1.pos(1));
phi.last = step1.pos(6) - swing_angle;
phi.next = step2.pos(6) - swing_angle;

foot_bodies = struct('right', biped.getBody(biped.getFrame(biped.foot_frame_id.right).body_ind),...
                       'left', biped.getBody(biped.getFrame(biped.foot_frame_id.left).body_ind));
contact_pts.last = quat2rotmat(axis2quat([0;0;1;phi.last])) * foot_bodies.right.getTerrainContactPoints();
contact_pts.next = quat2rotmat(axis2quat([0;0;1;phi.next])) * foot_bodies.right.getTerrainContactPoints();
effective_width = max([max(contact_pts.last(2,:)) - min(contact_pts.last(2,:)),...
                       max(contact_pts.next(2,:)) - min(contact_pts.next(2,:))]);
effective_length = max([max(contact_pts.last(1,:)) - min(contact_pts.last(1,:)),...
                        max(contact_pts.next(1,:)) - min(contact_pts.next(1,:))]);
effective_height = (max([effective_length, effective_width])/2) / sqrt(2); % assumes the foot never rotates past 45 degrees in the world frame

% % We'll expand all of our obstacles in the plane by this distance, which is the maximum allowed distance from the center of the foot to the edge of an obstacle
contact_length = effective_length / 2 + options.planar_clearance;
contact_width = effective_width / 2 + options.planar_clearance;
contact_height = effective_height + options.nom_z_clearance;

end