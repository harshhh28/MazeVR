#ifndef KERO_2D_COLLISION_H
#define KERO_2D_COLLISION_H

#include <stdbool.h>
#include "kero_vec2.h"

static inline vec2_t Vec2Normal(vec2_t v, bool s) {
    vec2_t n;
    if(s) {
        n.x = -v.y;
        n.y = v.x;
    }
    else {
        n.x = v.y;
        n.y = -v.x;
    }
    return Vec2Normalized(n);
}

vec2_t ProjectPolygonToVectorMinMax(vec2_t *vertices, int vertices_count, vec2_t v) {
    float projected = Vec2Dot(vertices[0], v), min = projected, max = projected;
    int i = 0;
    while(++i < vertices_count) {
        projected = Vec2Dot(vertices[i], v);
        if(projected < min) {
            min = projected;
        }
        if(projected > max) {
            max = projected;
        }
    }
    return Vec2Make(min, max);;
}

bool PolygonsIntersect(vec2_t *verticesa, int verticesa_count, vec2_t *verticesb, int verticesb_count) {
    if(verticesa_count > verticesb_count) {
        Swap(int, verticesa_count, verticesb_count);
        Swap(vec2_t *, verticesa, verticesb);
    }
    vec2_t axis, aminmax, bminmax;
    for(int s = 0; s < verticesa_count; ++s) {
        axis = Vec2Normal(Vec2AToB(verticesa[s], verticesa[(s + 1) % verticesa_count]), true);
        aminmax = ProjectPolygonToVectorMinMax(verticesa, verticesa_count, axis);
        bminmax = ProjectPolygonToVectorMinMax(verticesb, verticesb_count, axis);
        if(!(aminmax.x >= bminmax.x && aminmax.x <= bminmax.y ||
             aminmax.y >= bminmax.x && aminmax.y <= bminmax.y ||
             bminmax.x >= aminmax.x && bminmax.x <= aminmax.y ||
             bminmax.y >= aminmax.x && bminmax.y <= aminmax.y)) {
            return false;
        }
    }
    for(int s = 0; s < verticesb_count; ++s) {
        axis = Vec2Normal(Vec2AToB(verticesb[s], verticesb[(s + 1) % verticesb_count]), true);
        aminmax = ProjectPolygonToVectorMinMax(verticesa, verticesa_count, axis);
        bminmax = ProjectPolygonToVectorMinMax(verticesb, verticesb_count, axis);
        if(!(aminmax.x >= bminmax.x && aminmax.x <= bminmax.y ||
             aminmax.y >= bminmax.x && aminmax.y <= bminmax.y ||
             bminmax.x >= aminmax.x && bminmax.x <= aminmax.y ||
             bminmax.y >= aminmax.x && bminmax.y <= aminmax.y)) {
            return false;
        }
    }
    return true;
}

bool PolygonsIntersectMTV(vec2_t *verticesa, int verticesa_count, vec2_t *verticesb, int verticesb_count, vec2_t *mtv) {
    vec2_t axis, aminmax, bminmax;
    float translation_magnitude = HUGE_VAL;
    float magnitude[4];
    for(int s = 0; s < verticesa_count; ++s) {
        axis = Vec2Normal(Vec2AToB(verticesa[s], verticesa[(s + 1) % verticesa_count]), true);
        aminmax = ProjectPolygonToVectorMinMax(verticesa, verticesa_count, axis);
        bminmax = ProjectPolygonToVectorMinMax(verticesb, verticesb_count, axis);
        if(aminmax.x >= bminmax.x && aminmax.x <= bminmax.y ||
           aminmax.y >= bminmax.x && aminmax.y <= bminmax.y ||
           bminmax.x >= aminmax.x && bminmax.x <= aminmax.y ||
           bminmax.y >= aminmax.x && bminmax.y <= aminmax.y) {
            magnitude[0] = bminmax.y - aminmax.x;
            magnitude[1] = bminmax.x - aminmax.y;
            magnitude[2] = aminmax.y - bminmax.x;
            magnitude[3] = aminmax.x - bminmax.y;
            for(int i = 1; i < 4; ++i) {
                if(Absolute(magnitude[i]) < Absolute(magnitude[0])) {
                    magnitude[0] = magnitude[i];
                }
            }
            if(Absolute(magnitude[0]) < Absolute(translation_magnitude)) {
                translation_magnitude = magnitude[0];
                *mtv = axis;
            }
        }
        else {
            mtv->x = mtv->y = 0;
            return false;
        }
    }
    for(int s = 0; s < verticesb_count; ++s) {
        axis = Vec2Normal(Vec2AToB(verticesb[s], verticesb[(s + 1) % verticesb_count]), true);
        aminmax = ProjectPolygonToVectorMinMax(verticesa, verticesa_count, axis);
        bminmax = ProjectPolygonToVectorMinMax(verticesb, verticesb_count, axis);
        if(aminmax.x >= bminmax.x && aminmax.x <= bminmax.y ||
           aminmax.y >= bminmax.x && aminmax.y <= bminmax.y ||
           bminmax.x >= aminmax.x && bminmax.x <= aminmax.y ||
           bminmax.y >= aminmax.x && bminmax.y <= aminmax.y) {
            magnitude[0] = bminmax.y - aminmax.x;
            magnitude[1] = bminmax.x - aminmax.y;
            magnitude[2] = aminmax.y - bminmax.x;
            magnitude[3] = aminmax.x - bminmax.y;
            for(int i = 1; i < 4; ++i) {
                if(Absolute(magnitude[i]) < Absolute(magnitude[0])) {
                    magnitude[0] = magnitude[i];
                }
            }
            if(Absolute(magnitude[0]) < Absolute(translation_magnitude)) {
                translation_magnitude = magnitude[0];
                *mtv = axis;
            }
        }
        else {
            mtv->x = mtv->y = 0;
            return false;
        }
    }
    *mtv = Vec2MulScalar(Vec2Normalized(*mtv), translation_magnitude);
    return true;
}

#endif
