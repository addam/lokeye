canon = [eye(3), ones(3,1)]

perm = [0, 0, 1; -1, 0, 1; 0, -1, 1];

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
% it is linear wrt. u, d if only d(:, 4) is non-zero
function dH = diff(p1, p2, u, d)
 dH = (get_hom(p1, p2 + u * d) - get_hom(p1, p2)) / u;
end

function dPi = projection_derivative(H, v)
 w = H(3,:) * v;
 proj = H(1:2,:) * v / w;
 dPi = [1, 0, -proj(1); 0, 1, -proj(2)] / w;
end

function dPi = pd(proj, w)
 dPi = [1, 0, -proj(1); 0, 1, -proj(2)] / w;
end

function proj = projection(H, v)
 w = H(3,:) * v;
 proj = H(1:2,:) * v / w;
end

function p = npr(p)
 p = p ./ sum(abs(p), 1) .* sign(sum(p, 1));
end

function p = nmr(p)
 p = p ./ sum(sum(abs(p))) .* sign(sum(sum(p)));
end

%for j = [1:4];
 %for i = [1:3];
  %delta = zeros(3, 4);
  %p1 = rand(3, 4);
  %p2 = rand(3, 4);
  %p2 = log(p2 ./ (1 - p2));
  %delta(i, j) = 1;
  %[[42;j;i]diff(p1, p2, 0.01, delta), diff(p1, p2, 100, delta)]
 %end
%end

p1 = rand(3, 4);
p2 = rand(3, 4);
p2 = log(p2 ./ (1 - p2));
a = p2(:, 1); 
b = p2(:, 2); 
c = p2(:, 3); 
d = p2(:, 4); 
aa = p1(:, 1); 
bb = p1(:, 2); 
cc = p1(:, 3); 
dd = p1(:, 4); 
dx = [1;0;0]
dy = [0;1;0]

% diff: diff(p1, p2, anything, delta) = get_hom(p1, [p2(:,1:3), delta(:,4)])

% permutation: decanonize([d,a,b,c]) * perm * canonize(p1) = decanonize([a,b,c,d]) * canonize(p1), up to scale
% diff expanded: (decanonize([a,b,c,d+u*dx]) * canonize(p1) - decanonize([a,b,c,d]) * canonize(p1))/u = decanonize([a,b,c,dx]) * canonize(p1)
% diff perm: (decanonize([d,a,b,c+u*dx]) * perm * canonize(p1) - decanonize([d,a,b,c]) * perm * canonize(p1))/u = decanonize([d,a,b,dx]) * perm * canonize(p1)

R = perm
can = @canonize
dec = @decanonize
dec([d,a,b,c]) * R ~ dec([a,b,c,d])
dec([a,b,c,d+dx]) - dec([a,b,c,d]) = dec([a,b,c,dx])
dec([d,a,b,c+dx]) * R - dec([d,a,b,c]) * R = dec([d,a,b,dx]) * R
dec([d,a,b,c+dx]) * R - dec([d,a,b,c]) * R ~ dec([a,b,dx,d])
dec([b,c,d,a+dx]) - dec([b,c,d,a]) ~ dec([dx,b,c,d]) * R
% nojo, jenže já potřebuju tohle, třeba:
dec([a,b,c+dx,d]) - dec([a,b,c,d]) !~ (dec([d,a,b,c+dx]) - dec([d,a,b,c])) * R
% ty dvojice se rovnají až na škálu -- ale napříč dvojicema je ta škála různá, takže to nefunguje
% ta škála se mění výrazně, že ta rovnice nefunguje ani diferenciálně

% tohle je konstantní vzhledem k u, až na škálu:
dec([a,b+u*dx,c,d]) - dec([a,b,c,d])
% ta škála ale není úměrná přesně u
% navíc, tomuhle se to _nerovná_:
dec([a,dx,c,d])

tab = []
for i = [-60:60]
u = 2**(i / 2);
tab(i+61) = norm(dec([a,b+u*dx,c,d]) - dec([a,b,c,d]));
end

% to druhé konverguje k prvnímu, jak se patří:
projection_derivative(get_hom([aa,bb,cc,dd], [a,b,c,d]), p) * get_hom([aa,bb,cc,dd], [a,b,c,dy]) * p
u=1e-7; (projection(get_hom([aa,bb,cc,dd], [a,b,c,d+u*dy]), p) - projection(get_hom([aa,bb,cc,dd], [a,b,c,d]), p)) / u
% a pochopitelně smím udělat i tohle:
projection_derivative(get_hom([dd,aa,bb,cc], [d,a,b,c]), p) * get_hom([dd,aa,bb,cc], [d,a,b,dy]) * p
% ...rozepsáno:
Cc = canonize([dd,aa,bb,cc])  % const, 4 varianty
H = get_hom([aa,bb,cc,dd], [a,b,c,d])  % frame, 1 exemplář
Dc = decanonize([d,a,b,c])  % frame, 4 varianty
Dcy = decanonize([d,a,b,dy])  % frame, 8 variant
p = rand(3,1)  % pixel
cp = Cc * p % pixel, 4 varianty
pp = projection(H, p) % pixel, 1 exemplář
pd(pp, Dc(3,:)*cp) * Dcy * cp
% to znamená, řeším 12 soustav 3x3 na frame a násobím asi 5 matic na pixel
