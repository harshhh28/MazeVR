#ifndef KERO_VEC3_H

#ifdef __cplusplus
extern "C"{
#endif
    
#include "kero_math.h"
    
    typedef struct{
        union {
            struct { float x, y, z; };
            struct { float r, g, b; };
            struct { float pitch, yaw, roll; };
            float V[3];
            float M[1][3];
        };
    } vec3_t, mat1x3_t;
    
    static inline vec3_t Vec3Make(float x, float y, float z) {
        vec3_t v = { x,y,z };
        return v;
    }
    
    static inline vec3_t Vec3Add(vec3_t a, vec3_t b) {
        a.x += b.x, a.y += b.y, a.z += b.z;
        return a;
    }
    
    static inline vec3_t Vec3Sub(vec3_t a, vec3_t b) {
        a.x -= b.x, a.y -= b.y, a.z -= b.z;
        return a;
    }
    
    static inline vec3_t Vec3MulScalar(vec3_t v, float s) {
        v.x *= s, v.y *= s, v.z *= s;
        return v;
    }
    
    static inline float Vec3Length(vec3_t v) {
        return SquareRoot(Square(v.x) + Square(v.y) + Square(v.z));
    }
    
    static inline vec3_t Vec3Norm(vec3_t v) {
        float length = Vec3Length(v);
        v.x /= length, v.y /= length, v.z /= length;
        return v;
    }
    
    static inline float Vec3Dot(vec3_t a, vec3_t b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }
    
    static inline vec3_t Vec3Cross(vec3_t a, vec3_t b) {
        vec3_t x = {
            a.y*b.z - b.y*a.z,
            -a.x*b.z + b.x*a.z,
            a.x*b.y - b.x*a.y
        };
        return x;
    }
    
    static inline vec3_t Vec3AToB(vec3_t a, vec3_t b) {
        return Vec3Sub(b, a);
    }
    
    /*static inline float Vec3AngleFromTo(vec3_t a, vec3_t b) {
        float a = 0;
        return a;
    }*/
    
    static inline vec3_t Vec3ReflectByNormal(vec3_t v, vec3_t n) {
        return Vec3MulScalar(Vec3Sub(Vec3MulScalar(n, Vec3Dot(v, n)), v), -2.f);
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_VEC3_H
#endif