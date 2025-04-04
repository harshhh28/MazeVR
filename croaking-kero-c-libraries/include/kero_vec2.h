#if !defined(KERO_VEC2_H)

#ifdef __cplusplus
extern "C"{
#endif
    
#include "kero_math.h"
#include <math.h>
    
    typedef struct{
        int x, y;
    } vec2i_t;
    
    vec2i_t VEC2IZERO = {0};
    
    vec2i_t Vec2iAdd(vec2i_t a, vec2i_t b){
        vec2i_t c;
        c.x = a.x + b.x;
        c.y = a.y + b.y;
        return c;
    }
    
    vec2i_t Vec2iSub(vec2i_t a, vec2i_t b){
        vec2i_t c;
        c.x = a.x - b.x;
        c.y = a.y - b.y;
        return c;
    }
    
    bool Vec2iAreEqual(vec2i_t a, vec2i_t b){
        return (a.x == b.x && a.y == b.y);
    }
    
    bool Vec2iIntersectLineSegments(vec2i_t a0, vec2i_t a1, vec2i_t b0, vec2i_t b1, vec2i_t* intersection){
        float s1x, s1y, s2x, s2y;
        s1x = a1.x - a0.x; s1y = a1.y - a0.y;
        s2x = b1.x - b0.x; s2y = b1.y - b0.y;
        float s, t;
        s = (-s1y * (float)(a0.x - b0.x) + s1x * (float)(a0.y - b0.y)) / (-s2x * s1y + s1x * s2y);
        t = ( s2x * (float)(a0.y - b0.y) - s2y * (float)(a0.x - b0.x)) / (-s2x * s1y + s1x * s2y);
        if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
        {
            // Collision detected
            if (intersection != NULL){
                intersection->x = a0.x + (t * s1x);
                intersection->y = a0.y + (t * s1y);
            }
            return true;
        }
        return false;
    }
    
    vec2i_t Vec2iRotated(vec2i_t vec, float angle){
        vec2i_t new_vec;
        float s = sinf(angle);
        float c = cosf(angle);
        new_vec.x = vec.x*c - vec.y*s;
        new_vec.y = vec.x*s + vec.y*c;
        return new_vec;
    }
    
    float Vec2iAngle(vec2i_t vec){
        return atan2f(vec.y, vec.x);
    }
    
    vec2i_t Vec2iMake(int x, int y){
        vec2i_t new_vec;
        new_vec.x = x;
        new_vec.y = y;
        return new_vec;
    }
    
    typedef struct{
        union {
            struct { float x, y; };
            float V[2];
            struct { float u, v; };
        };
    } vec2_t;
    
    
    const vec2_t VEC2ZERO = {0,0};
    
    static inline vec2_t Vec2Add(vec2_t a, vec2_t b){
        vec2_t c;
        c.x = a.x + b.x;
        c.y = a.y + b.y;
        return c;
    }
    
    static inline vec2_t Vec2Sub(vec2_t a, vec2_t b){
        a.x -= b.x;
        a.y -= b.y;
        return a;
    }
    
    static inline bool Vec2AreEqual(vec2_t a, vec2_t b){
        return (a.x == b.x && a.y == b.y);
    }
    
    bool Vec2IntersectLineSegments(vec2_t a0, vec2_t a1, vec2_t b0, vec2_t b1, vec2_t* intersection){
        float s1x, s1y, s2x, s2y;
        s1x = a1.x - a0.x; s1y = a1.y - a0.y;
        s2x = b1.x - b0.x; s2y = b1.y - b0.y;
        float s, t;
        s = (-s1y * (float)(a0.x - b0.x) + s1x * (float)(a0.y - b0.y)) / (-s2x * s1y + s1x * s2y);
        t = ( s2x * (float)(a0.y - b0.y) - s2y * (float)(a0.x - b0.x)) / (-s2x * s1y + s1x * s2y);
        if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
        {
            // Collision detected
            if (intersection != NULL){
                intersection->x = a0.x + (t * s1x);
                intersection->y = a0.y + (t * s1y);
            }
            return true;
        }
        return false;
    }
    
    static inline vec2_t Vec2Rotated(vec2_t vec, float angle){
        vec2_t new_vec;
        float s = sinf(angle);
        float c = cosf(angle);
        new_vec.x = vec.x*c - vec.y*s;
        new_vec.y = vec.x*s + vec.y*c;
        return new_vec;
    }
    
    static inline float Vec2Angle(vec2_t vec){
        return atan2f(vec.y, vec.x);
    }
    
    static inline vec2_t Vec2Make(float x, float y){
        vec2_t new_vec;
        new_vec.x = x;
        new_vec.y = y;
        return new_vec;
    }
    
    static inline bool Vec2Equals(vec2_t a, vec2_t b){
        return (Absolute(a.x-b.x) < 0.001f && Absolute(a.y-b.y) < 0.001f);
    }
    
    static inline float Vec2Length(vec2_t a){
        return SquareRoot(Square(a.x) + Square(a.y));
    }
    
    static inline float Vec2DistanceBetween(vec2_t a, vec2_t b){
        return SquareRoot(Square(b.x-a.x) + Square(b.y-a.y));
    }
    
    static inline vec2_t Vec2Normalized(vec2_t a) {
        float length = Vec2Length(a);
        if(length == 0) {
            return a;
        }
        a.x /= length, a.y /= length;
        return a;
    }
    
    static inline vec2_t Vec2MulScalar(vec2_t a, float s) {
        a.x *= s, a.y *= s;
        return a;
    }
    
    static inline vec2_t Vec2AToB(vec2_t a, vec2_t b) {
        b.x -= a.x, b.y -= a.y;
        return b;
    }
    
    static inline vec2_t Vec2Lerp(vec2_t a, vec2_t b, float t) {
        return Vec2Add(a, Vec2MulScalar(Vec2AToB(a, b), t));
    }
    
    static inline vec2_t Vec2LerpToDistance(vec2_t a, vec2_t b, float t, float distance) {
        if(Vec2DistanceBetween(a, b) > distance) {
            return Vec2Add(a, Vec2MulScalar(Vec2Normalized(Vec2AToB(a, b)), t));
        }
    }
    
    static inline float Vec2Dot(vec2_t a, vec2_t b) {
        return a.x * b.x + a.y * b.y;
    }
    
    static inline float Vec2DistanceFromPointToLine(vec2_t p, vec2_t la, vec2_t lb) {
        float a = la.y - lb.y;
        float b = lb.x - la.x;
        float c = la.x * lb.y - lb.x * la.y;
        return Absolute(a * p.x + b * p.y + c) / SquareRoot(Square(a) + Square(b));
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_VEC2_H
#endif