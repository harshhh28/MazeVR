#ifndef KERO_MATRIX_H

#include "kero_vec3.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct {
        union {
            float M[4][4];
        };
    } mat4x4_t;
    
    void MatMul_Vec3_4x4(const vec3_t* a, const mat4x4_t* b, vec3_t* out) {
        out->x = a->x*b->M[0][0] + a->y*b->M[1][0] + a->z*b->M[2][0];
        out->y = a->x*b->M[0][1] + a->y*b->M[1][1] + a->z*b->M[2][1];
        out->z = a->x*b->M[0][2] + a->y*b->M[1][2] + a->z*b->M[2][2];
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_MATRIX_H
#endif