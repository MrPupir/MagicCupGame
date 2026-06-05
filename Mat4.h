#pragma once
#include <cmath>
#include <cstring>

struct Mat4 {
    float m[16];

    Mat4() { Identity(); }

    void Identity() {
        memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Mat4 Multiply(const Mat4& A, const Mat4& B) {
        Mat4 R;
        for (int c = 0; c < 4; c++) {
            for (int r = 0; r < 4; r++) {
                float s = 0.0f;
                for (int k = 0; k < 4; k++) {
                    s += A.m[k * 4 + r] * B.m[c * 4 + k];
                }
                R.m[c * 4 + r] = s;
            }
        }
        return R;
    }

    static Mat4 Perspective(float fovYDeg, float aspect, float zNear, float zFar) {
        Mat4 R;
        float fovY = fovYDeg * 3.14159265358979323846f / 180.0f;
        float f = 1.0f / std::tan(fovY * 0.5f);
        memset(R.m, 0, sizeof(R.m));
        R.m[0] = f / aspect;
        R.m[5] = f;
        R.m[10] = (zFar + zNear) / (zNear - zFar);
        R.m[11] = -1.0f;
        R.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
        return R;
    }

    static Mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
        Mat4 R;
        memset(R.m, 0, sizeof(R.m));
        R.m[0] = 2.0f / (right - left);
        R.m[5] = 2.0f / (top - bottom);
        R.m[10] = -2.0f / (zFar - zNear);
        R.m[12] = -(right + left) / (right - left);
        R.m[13] = -(top + bottom) / (top - bottom);
        R.m[14] = -(zFar + zNear) / (zFar - zNear);
        R.m[15] = 1.0f;
        return R;
    }

    static Mat4 LookAt(float ex, float ey, float ez,
                       float cx, float cy, float cz,
                       float ux, float uy, float uz) {
        float fx = cx - ex, fy = cy - ey, fz = cz - ez;
        float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
        if (fl > 1e-8f) { fx /= fl; fy /= fl; fz /= fl; }

        float sx = fy * uz - fz * uy;
        float sy = fz * ux - fx * uz;
        float sz = fx * uy - fy * ux;
        float sl = std::sqrt(sx*sx + sy*sy + sz*sz);
        if (sl > 1e-8f) { sx /= sl; sy /= sl; sz /= sl; }

        float uxn = sy * fz - sz * fy;
        float uyn = sz * fx - sx * fz;
        float uzn = sx * fy - sy * fx;

        Mat4 R;
        R.m[0] = sx;  R.m[4] = sy;   R.m[8]  = sz;   R.m[12] = -(sx*ex + sy*ey + sz*ez);
        R.m[1] = uxn; R.m[5] = uyn;  R.m[9]  = uzn;  R.m[13] = -(uxn*ex + uyn*ey + uzn*ez);
        R.m[2] = -fx; R.m[6] = -fy;  R.m[10] = -fz;  R.m[14] = (fx*ex + fy*ey + fz*ez);
        R.m[3] = 0;   R.m[7] = 0;    R.m[11] = 0;    R.m[15] = 1;
        return R;
    }

    static Mat4 Invert(const Mat4& in) {
        const float* a = in.m;
        float inv[16];

        inv[0]  =  a[5]*a[10]*a[15] - a[5]*a[11]*a[14] - a[9]*a[6]*a[15] + a[9]*a[7]*a[14] + a[13]*a[6]*a[11] - a[13]*a[7]*a[10];
        inv[4]  = -a[4]*a[10]*a[15] + a[4]*a[11]*a[14] + a[8]*a[6]*a[15] - a[8]*a[7]*a[14] - a[12]*a[6]*a[11] + a[12]*a[7]*a[10];
        inv[8]  =  a[4]*a[9]*a[15]  - a[4]*a[11]*a[13] - a[8]*a[5]*a[15] + a[8]*a[7]*a[13] + a[12]*a[5]*a[11] - a[12]*a[7]*a[9];
        inv[12] = -a[4]*a[9]*a[14]  + a[4]*a[10]*a[13] + a[8]*a[5]*a[14] - a[8]*a[6]*a[13] - a[12]*a[5]*a[10] + a[12]*a[6]*a[9];
        inv[1]  = -a[1]*a[10]*a[15] + a[1]*a[11]*a[14] + a[9]*a[2]*a[15] - a[9]*a[3]*a[14] - a[13]*a[2]*a[11] + a[13]*a[3]*a[10];
        inv[5]  =  a[0]*a[10]*a[15] - a[0]*a[11]*a[14] - a[8]*a[2]*a[15] + a[8]*a[3]*a[14] + a[12]*a[2]*a[11] - a[12]*a[3]*a[10];
        inv[9]  = -a[0]*a[9]*a[15]  + a[0]*a[11]*a[13] + a[8]*a[1]*a[15] - a[8]*a[3]*a[13] - a[12]*a[1]*a[11] + a[12]*a[3]*a[9];
        inv[13] =  a[0]*a[9]*a[14]  - a[0]*a[10]*a[13] - a[8]*a[1]*a[14] + a[8]*a[2]*a[13] + a[12]*a[1]*a[10] - a[12]*a[2]*a[9];
        inv[2]  =  a[1]*a[6]*a[15]  - a[1]*a[7]*a[14]  - a[5]*a[2]*a[15] + a[5]*a[3]*a[14] + a[13]*a[2]*a[7]  - a[13]*a[3]*a[6];
        inv[6]  = -a[0]*a[6]*a[15]  + a[0]*a[7]*a[14]  + a[4]*a[2]*a[15] - a[4]*a[3]*a[14] - a[12]*a[2]*a[7]  + a[12]*a[3]*a[6];
        inv[10] =  a[0]*a[5]*a[15]  - a[0]*a[7]*a[13]  - a[4]*a[1]*a[15] + a[4]*a[3]*a[13] + a[12]*a[1]*a[7]  - a[12]*a[3]*a[5];
        inv[14] = -a[0]*a[5]*a[14]  + a[0]*a[6]*a[13]  + a[4]*a[1]*a[14] - a[4]*a[2]*a[13] - a[12]*a[1]*a[6]  + a[12]*a[2]*a[5];
        inv[3]  = -a[1]*a[6]*a[11]  + a[1]*a[7]*a[10]  + a[5]*a[2]*a[11] - a[5]*a[3]*a[10] - a[9]*a[2]*a[7]   + a[9]*a[3]*a[6];
        inv[7]  =  a[0]*a[6]*a[11]  - a[0]*a[7]*a[10]  - a[4]*a[2]*a[11] + a[4]*a[3]*a[10] + a[8]*a[2]*a[7]   - a[8]*a[3]*a[6];
        inv[11] = -a[0]*a[5]*a[11]  + a[0]*a[7]*a[9]   + a[4]*a[1]*a[11] - a[4]*a[3]*a[9]  - a[8]*a[1]*a[7]   + a[8]*a[3]*a[5];
        inv[15] =  a[0]*a[5]*a[10]  - a[0]*a[6]*a[9]   - a[4]*a[1]*a[10] + a[4]*a[2]*a[9]  + a[8]*a[1]*a[6]   - a[8]*a[2]*a[5];

        float det = a[0]*inv[0] + a[1]*inv[4] + a[2]*inv[8] + a[3]*inv[12];
        Mat4 R;
        if (det == 0.0f) return R;
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) R.m[i] = inv[i] * det;
        return R;
    }
};
