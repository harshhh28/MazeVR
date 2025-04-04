#ifndef KERO_MATH_H
#define KERO_MATH_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdlib.h>
#include <stdbool.h>
    
#define ROOT2 1.414213562f
#define PI 3.14159265359f
#define TWOPI 6.28318530718
#define TAU TWOPI
#define HALFPI 1.570796327f
    
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Absolute(a) ( (a) > 0 ? (a) : ( -(a) ) )
#define Square(a) ( (a) * (a) )
#define Sign(a) ( (a) < 0 ? -1 : 1)
#define Randf(a) ((float)rand() / (float)RAND_MAX * a)
#define FMod(numerator, denominator) ((numerator) - (int)((numerator)/(denominator))*(denominator))
#define Swap(type, a, b) { type t = (a); (a) = (b); (b) = (t); }
#define Floor(a) (Sign(a) * (int)Absolute(a))
#define Ceiling(a) (Sign(a) * ((int)Absolute(a) + 1))
    
    static inline int Wrap(int a, int min, int max) {
        if(min > max) {
            Swap(int, min, max);
        }
        int range = max - min;
        a = ((a - min) % range + range) % range + min;
    }
    
    static inline float Power(float a, int p) {
        if(p == 0) return 1.f;
        float orig = a;
        for(int i = 1; i < p; ++i) {
            a *= orig;
        }
        return a;
    }
    
    static inline int Poweri(float a, int p) {
        if(p == 0) return 1;
        float orig = a;
        for(int i = 1; i < p; ++i) {
            a *= orig;
        }
        return a;
    }
    
    static inline float SquareRoot(float a)
    {
        float precision = a*0.0001f;
        if(precision < 0.0001f) {
            precision = 0.0001f;
        }
        float root = a;
        while (Absolute(root - a / root) > precision)
        {
            root = (root + a / root) / 2.f;
        }
        return root;
    }
    
    static inline double SquareRootd(double a)
    {
        double precision = a*0.00001;
        double root = a;
        while ((root - a / root) > precision)
        {
            root= (root + a / root) / 2.0;
        }
        return root;
    }
    
#define LineLength(ax,ay,bx,by) SquareRoot(Square((float)(bx)-(float)(ax))+Square((float)(by)-(float)(ay)))
    
    static inline int Roundf(float a){ return (int)Sign(a)*(Absolute(a)+0.5f); }
    static inline int Roundd(double a){ return (int)Sign(a)*(Absolute(a)+0.5); }
    
    static inline float Lerp(float a, float b, float t) {
        return (a + (b-a)*t);
    }
    static inline float LerpSnap(float a, float b, float t, float minimum_difference) {
        return Absolute(b-a)<minimum_difference?b:(a + (b-a)*t);
    }
    
    static inline bool PointIsWithinRect(float x, float y, float left, float top, float right, float bottom) {
        return x >= left && x <= right && y <= top && y >= bottom;
    }
    
    static inline bool PointIsWithinRecti(int x, int y, int left, int top, int right, int bottom) {
        return x >= left && x <= right && y <= top && y >= bottom;
    }
    
#ifdef __cplusplus
}
#endif

#endif