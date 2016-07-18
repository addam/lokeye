canon = [eye(3), ones(3,1)]

% these function calculate homographies for sets of 4 homogeneous 2D points, expressed as a 3x4 matrix
% the sign ~ in comments means equality up to scale of each column (that is, equality of the points)

% decanonize(p) * canon ~ p
function T = decanonize(p)
 system = p(:, 1:3);
 alpha = system \ p(:, 4);
 T = system * diag(alpha);
end

% canonize(p) * p ~ canon
function T = canonize(p)
 system = [cross(p(:,2), p(:,3)), cross(p(:,3), p(:,1)), cross(p(:,1), p(:,2))]';
 T = system ./ (system * p(:, 4));
end

% get_hom(p1, p2) * p1 ~ p2
function H = get_hom(p1,p2)
 H = decanonize(p2) * canonize(p1);
end

% derivative of the homography along a given vector
% it is linear wrt. p2, u, d (but I can not yet prove why)
function dH = diff(p1, p2, u, d)
 dH = (get_hom(p1, p2 + u * d) - get_hom(p1, p2)) / u;
end

function p = npr(p)
 p = p ./ sum(abs(p), 1) .* sign(sum(p, 1));
end

function p = nmr(p)
 p = p ./ sum(sum(abs(p))) .* sign(sum(sum(p)));
end

