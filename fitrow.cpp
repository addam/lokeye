#include <fstream>
#include <iostream>
#include <vector>
#include <tuple>
#include <Eigen/Dense>
using namespace Eigen;
using namespace std;

struct Point
{
    float px, py, pz, qx, qy, qz;
};

inline float pow2(float x) { return x*x; }

int main(int argc, char *argv[])
{
    vector<Point> points;
    vector<float> correct_z;
    ifstream file1((argc > 1) ? argv[1] : "rot1.xyz"), file2((argc > 2) ? argv[2] : "rot2.xyz");
    Point center({0, 0, 0, 0, 0, 0});
    while (1) {
        float px, py, pz, qx, qy, qz;
        file1 >> px >> py >> pz;
        file2 >> qx >> qy >> qz;
        if (file1.eof() or file2.eof())
            break;
        points.push_back({px, py, 0, qx, qy, 0});
        correct_z.push_back(pz);
        center.px += px; center.py += py; center.pz += pz; center.qx += qx; center.qy += qy;
    }
    const int n = points.size();
    center.px /= n; center.py /= n; center.pz /= n; center.qx /= n; center.qy /= n;
    for (Point &p: points) {
        p.px -= center.px;
        p.py -= center.py;
        p.qx -= center.qx;
        p.qy -= center.qy;
    }
    
    float rxx, rxy, rxz, ryx, ryy, ryz, rzx, rzy, rzz;
    
    {
        MatrixX4f m;
        m.resize(n, NoChange);
        for (int i=0; i<n; i++) {
            m(i, 0) = points[i].px;
            m(i, 1) = points[i].py;
            m(i, 2) = points[i].qx;
            m(i, 3) = points[i].qy;
        }
        JacobiSVD<MatrixXf> svd(m, ComputeThinV);
        cout << "Singular value:" << endl << svd.singularValues()[3] << endl;
        cout << "Null vector:" << endl << svd.matrixV().col(3) << endl;
        rzx = -svd.matrixV()(3, 3);
        rzy = svd.matrixV()(2, 3);
    }
    
    {
        MatrixXf A(2 * n, n + 4);
        for (int i=0; i<n; i++) {
            for (int j=0; j<n+4; j++) {
                A(2*i+0, j) = 0; A(2*i+1, j) = 0;
            }
            A(2*i+0, 0) = points[i].px; A(2*i+1, 2) = points[i].px;
            A(2*i+0, 1) = points[i].py; A(2*i+1, 3) = points[i].py;
            A(2*i+0, i+4) = rzx;
            A(2*i+1, i+4) = rzy;
        }
        VectorXf b(2*n);
        for (int i=0; i<n; i++) {
            b(2*i+0) = points[i].qx;
            b(2*i+1) = points[i].qy;
        }
        auto svd = A.jacobiSvd(ComputeThinU | ComputeThinV);
        cout << "Last two singular values:" << endl << svd.singularValues().tail<2>() << endl;
        VectorXf sol = svd.solve(b);
        rxx = sol[0];
        ryx = sol[1];
        rxy = sol[2];
        ryy = sol[3];
        {
            double diff = 0;
            for (int i=0; i<n; i++) {
                float ppx = points[i].px, ppy = points[i].py, ppz = sol[i+4];
                float x = ppx * rxx + ppy * ryx + ppz * rzx;
                float y = ppx * rxy + ppy * ryy + ppz * rzy;
                diff += pow2(x - points[i].qx) + pow2(y - points[i].qy);
            }
            printf("Intermediate xy error: %f\n", diff);
        }
        Matrix3f J;
        J(0,2) = rzx*rzx;
        J(1,2) = rzy*rzy;
        J(2,2) = -rzx*rzy;
        float cv = 0, cw = 0, q = 1/sqrt(rzx*rzy);
        const MatrixXf kernel = svd.matrixV().rightCols<2>();
        const float v1 = kernel(0, 0), v2 = kernel(1, 0), v3 = kernel(2, 0), v4 = kernel(3, 0),
            w1 = kernel(0, 1), w2 = kernel(1, 1), w3 = kernel(2, 1), w4 = kernel(3, 1);
        {
            double diff1 = 0, diff2 = 0;
            for (int i=0; i<n; i++) {
                float ppx = points[i].px, ppy = points[i].py, k1 = kernel(i+4, 0), k2 = kernel(i+4, 1);
                diff1 += abs(ppx * v1 + ppy * v2 + k1 * rzx);
                diff1 += abs(ppx * v3 + ppy * v4 + k1 * rzy);
                diff2 += abs(ppx * w1 + ppy * w2 + k2 * rzx);
                diff2 += abs(ppx * w3 + ppy * w4 + k2 * rzy);
            }
            printf("Kernel error: %f, %f\n", diff1, diff2);
        }
        for (int i=0; i<100; i++) {
            J(0,0) = 2*v2*(cw*w2+cv*v2+ryx) + 2*v1*(cw*w1+cv*v1+rxx);
            J(0,1) = 2*w2*(cw*w2+cv*v2+ryx) + 2*w1*(cw*w1+cv*v1+rxx);
            J(1,0) = 2*v4*(cw*w4+cv*v4+ryy) + 2*v3*(cw*w3+cv*v3+rxy);
            J(1,1) = 2*w4*(cw*w4+cv*v4+ryy) + 2*w3*(cw*w3+cv*v3+rxy);
            J(2,0) = v1*(cw*w3+cv*v3+rxy) + v2*(cw*w4+cv*v4+ryy) + v3*(cw*w1+cv*v1+rxx) + v4*(cw*w2+cv*v2+ryx);
            J(2,1) = w1*(cw*w3+cv*v3+rxy) + w2*(cw*w4+cv*v4+ryy) + w3*(cw*w1+cv*v1+rxx) + w4*(cw*w2+cv*v2+ryx);
            Vector3f value;
            value(0) = 1 - pow2(cw*w2 + cv*v2 + ryx) - pow2(cw*w1 + cv*v1 + rxx) - pow2(rzx)*q;
            value(1) = 1 - pow2(cw*w4 + cv*v4 + ryy) - pow2(cw*w3 + cv*v3 + rxy) - pow2(rzy)*q;
            value(2) = rzx*rzy*q - (cw*w2 + cv*v2 + ryx)*(cw*w4 + cv*v4 + ryy) - (cw*w1 + cv*v1 + rxx)*(cw*w3 + cv*v3 + rxy);
            Vector3f delta = J.jacobiSvd(ComputeFullU | ComputeFullV).solve(value);
            const float sor = 1;
            cv = (2 - sor) * cv + sor * delta(0);
            cw = (2 - sor) * cw + sor * delta(1);
            q = (2 - sor) * q + sor * delta(2);
        }
        rxx += cw*w1+cv*v1;
        ryx += cw*w2+cv*v2;
        rzx *= sqrt(q);
        rxy += cw*w3+cv*v3;
        ryy += cw*w4+cv*v4;
        rzy *= sqrt(q);
        rxz = ryx * rzy - rzx * ryy;
        ryz = rzx * rxy - rxx * rzy;
        rzz = rxx * ryy - ryx * rxy;
        printf("Solved cv = %f, cw = %f, q = %f\n", cv, cw, q);
        printf("Rotation matrix:\n%f %f %f\n %f %f %f\n %f %f %f\n", rxx, ryx, rzx, rxy, ryy, rzy, rxz, ryz, rzz);
        for (int i=0; i<n; i++) {
            points[i].pz = (sol[i+4] + cv * kernel(i+4, 0) + cw * kernel(i+4, 1))/sqrt(q);
        }
    }
    {
        float diff = 0;
        for (int i=0; i<n; i++) {
            float ppx = points[i].px, ppy = points[i].py, ppz = points[i].pz;
            float x = ppx * rxx + ppy * ryx + ppz * rzx;
            float y = ppx * rxy + ppy * ryy + ppz * rzy;
            diff += pow2(x - points[i].qx) + pow2(y - points[i].qy);
        }
        printf("Final xy error: %f\n", diff);
    }
    {
        float diff = 0;
        for (int i=0; i<n; i++) {
            diff += pow2(points[i].pz + center.pz - correct_z[i]);
        }
        printf("Total z error vs. reference: %f\n", diff);
    }
    
    ofstream out("out.obj");
    for (Point p: points) {
        out << "v " << p.px << ' ' << p.py << ' ' << p.pz << endl;
    }
}
