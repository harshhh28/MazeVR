#ifndef KERO_SOFTWARE_3D_H

#ifdef __cplusplus
extern "C"{
#endif
    
#include "kero_platform.h"
#include "kero_vec2.h"
#include "kero_vec3.h"
#include "kero_image.h"
#include "kero_sprite.h"
    
    typedef struct {
        union {
            struct { vec3_t v0, v1, v2; };
            vec3_t v[3];
        };
        uint32_t c;
        struct {
            uint32_t double_sided: 1;
        } flags;
        vec2_t uv[3];
        uint8_t texture_index;
    } face_t;
    
    static inline void K3D_SetPixel(ksprite_t* target, float* depth_buffer, int x, int y, float depth, uint32_t color){
        if(depth > depth_buffer[x + y*target->w]){
            KS_SetPixel(target, x, y, color);
            depth_buffer[x + y*target->w] = depth;
        }
    }
    
    static inline void K3D_SetPixelAlpha10(ksprite_t* target, float* depth_buffer, int x, int y, float depth, uint32_t color){
        if((color>>24) > 0 && depth > depth_buffer[x + y*target->w]){
            KS_SetPixel(target, x, y, color);
            depth_buffer[x + y*target->w] = depth;
        }
    }
    
    static inline void K3D_ScanLine(ksprite_t* dest, float* depth_buffer, int y, int x0, float z0, int x1, float z1, uint32_t pixel){
        if(x0 > x1) {
            KS_Swap(int, x0, x1);
            KS_Swap(float, z0, z1);
        }
        if(x1 < 0 || x0 > dest->w-1)return;
        int left = Max(0, x0);
        int right = Min(dest->w-1, x1);
        for(int x = left; x <= right; ++x){
            float z = z0 + (z1-z0) * (((float)x-x0) / (x1-x0+0.0001f));
            K3D_SetPixel( dest, depth_buffer, x, y, z, pixel );
        }
    }
    
    static inline void K3D_ScanLineTextured(ksprite_t* dest, float* depth_buffer, ksprite_t* texture, int y, int x0, float z0, int x1, float z1, float u0, float u1, float v0, float v1){
        if(y < 0 || y > dest->h-1) return;
        if(x0 > x1) {
            KS_Swap(int, x0, x1);
            if(x1 < 0 || x0 > dest->w-1) return;
            KS_Swap(float, z0, z1);
            KS_Swap(float, u0, u1);
            KS_Swap(float, v0, v1);
        }
        int left = Max(0, x0);
        int right = Min(dest->w-1, x1);
        uint32_t pixel;
        float skip = left - x0;
        float xfrac = 1.f/(x1-x0+0.0001f);
        float zstep = (z1-z0)*xfrac;
        float ustep = (u1-u0)*xfrac;
        float vstep = (v1-v0)*xfrac;
        float z = z0 + zstep*skip;
        float u = u0 + ustep*skip;
        float v = v0 + vstep*skip;
        for(int x = left; x <= right; ++x){
            K3D_SetPixelAlpha10( dest, depth_buffer, x, y, z, KS_SampleWrapped(texture, u/z, v/z) );
            z += zstep;
            u += ustep;
            v += vstep;
        }
    }
    
    static inline void K3D_ScanLineSafe(ksprite_t* dest, float* depth_buffer, int y, int x0, float z0, int x1, float z1, uint32_t pixel){
        if(y < 0 || y > dest->h-1)return;
        int left = Min(x0, x1);
        int right = Max(x0, x1);
        if(right < 0 || left > dest->w-1)return;
        left = Max(0, left);
        right = Min(dest->w-1, right);
        for(int x = left; x <= right; ++x){
            K3D_SetPixel(dest, depth_buffer, x, y, z0, pixel);
        }
    }
    
    void K3D_DrawTriangle(ksprite_t* dest, float* depth_buffer, vec3_t a, vec3_t b, vec3_t c, uint32_t color){
        // Sort vertices vertically so a.y <= b.y <= c.y
        if(a.y > b.y){
            KS_Swap(vec3_t, a, b);
        }
        if(b.y > c.y){
            KS_Swap(vec3_t, b, c);
        }
        if(a.y > b.y){
            KS_Swap(vec3_t, a, b);
        }
        if(b.y>a.y){
            // Scan lines between the edge of a->b and the edge of a->c
            for(int y = KS_Max(a.y, 0); y <= KS_Min(b.y, dest->h-1); ++y){
                float fracb = ((KS_Max((float)y,a.y)-a.y)/(b.y-a.y));
                float fracc = ((KS_Max((float)y,a.y)-a.y)/(c.y-a.y));
                float x01 = a.x+(b.x-a.x)*fracb;
                float x02 = a.x+(c.x-a.x)*fracc;
                float z0 =  a.z+(b.z-a.z)*fracb;
                float z1 =  a.z+(c.z-a.z)*fracc;
                K3D_ScanLine(dest, depth_buffer, y, x01, z0, x02, z1, color);
            }
        }
        if(c.y>b.y){
            // Scan lines between the edge of a->c and the edge of b->c
            for(int y = KS_Max(b.y, 0); y <= KS_Min(c.y, dest->h-1); ++y){
                float fraca = ((KS_Max((float)y,b.y)-a.y)/(c.y-a.y));
                float fracb = ((KS_Max((float)y,b.y)-b.y)/(c.y-b.y));
                float x02 = a.x+(c.x-a.x)*fraca;
                float x12 = b.x+(c.x-b.x)*fracb;
                float z0 =  a.z+(c.z-a.z)*fraca;
                float z1 =  b.z+(c.z-b.z)*fracb;
                K3D_ScanLine(dest, depth_buffer, y, x02, z0, x12, z1, color);
            }
        }
    }
    
    void K3D_DrawTriangleTextured(ksprite_t* dest, float* depth_buffer, ksprite_t* texture, vec3_t a, vec3_t b, vec3_t c, vec2_t uv[3]){
        // Sort vertices vertically so a.y <= b.y <= c.y
        if(a.y > b.y){
            KS_Swap(vec3_t, a, b);
            KS_Swap(float, uv[0].u, uv[1].u);
            KS_Swap(float, uv[0].v, uv[1].v);
        }
        if(b.y > c.y){
            KS_Swap(vec3_t, b, c);
            KS_Swap(float, uv[1].u, uv[2].u);
            KS_Swap(float, uv[1].v, uv[2].v);
        }
        if(a.y > b.y){
            KS_Swap(vec3_t, a, b);
            KS_Swap(float, uv[0].u, uv[1].u);
            KS_Swap(float, uv[0].v, uv[1].v);
        }
        if(b.y>a.y){
            // Scan lines between the edge of a->b and the edge of a->c
            int y2 = KS_Min(b.y, dest->h-1);
            float x0, x1, z0, z1, u0, u1, v0, v1;
            float fracab, fracac;
            for(int y = KS_Max(a.y, 0); y <= y2; ++y) {
                fracab = (KS_Max(y, a.y)-a.y)/(b.y-a.y), fracac = (KS_Max(y, a.y)-a.y)/(c.y-a.y);
                x0 = a.x+(b.x-a.x)*fracab, x1 = a.x+(c.x-a.x)*fracac;
                z0 = a.z+(b.z-a.z)*fracab, z1 = a.z+(c.z-a.z)*fracac;
                u0 = uv[0].u+(uv[1].u-uv[0].u)*fracab, u1 = uv[0].u+(uv[2].u-uv[0].u)*fracac;
                v0 = uv[0].v+(uv[1].v-uv[0].v)*fracab, v1 = uv[0].v+(uv[2].v-uv[0].v)*fracac;
                K3D_ScanLineTextured(dest, depth_buffer, texture, y, x0, z0, x1, z1, u0, u1, v0, v1);
            }
            /* Old and glitchy
            float y = KS_Max(a.y, 0), y2 = KS_Min(b.y-1, dest->h-1);
            float skip = (float)y-a.y;
            float fracb = 1.f / (b.y-a.y+0.0001f), fracc = 1.f / (c.y-a.y+0.0001f);
            float x0s = (b.x-a.x)*fracb, x1s = (c.x-a.x)*fracc;
            float z0s = (b.z-a.z)*fracb, z1s = (c.z-a.z)*fracc;
            float u0s = (uv[1].u-uv[0].u)*fracb, u1s = (uv[2].u-uv[0].u)*fracc;
            float v0s = (uv[1].v-uv[0].v)*fracb, v1s = (uv[2].v-uv[0].v)*fracc;
            float x0  = a.x+skip*x0s, x1  = a.x+skip*x1s;
            float z0  = a.z+skip*z0s, z1  = a.z+skip*z1s;
            float u0  = uv[0].u+skip*u0s, u1  = uv[0].u+skip*u1s;
            float v0  = uv[0].v+skip*v0s, v1  = uv[0].v+skip*v1s;
            for(; y <= y2; ++y){
                K3D_ScanLineTextured(dest, depth_buffer, texture, y, x0, z0, x1, z1, u0, u1, v0, v1);
                x0 += x0s; x1 += x1s; z0 += z0s; z1 += z1s; u0 += u0s; u1 += u1s; v0 += v0s; v1 += v1s;
            }*/
        }
        if(c.y>b.y){
            // Scan lines between the edge of a->c and the edge of b->c
            int y2 = KS_Min(c.y, dest->h);
            float fracac, fracbc;
            float x0, x1, z0, z1, u0, u1, v0, v1;
            for(int y = KS_Max(b.y, 0); y < y2; ++y){
                fracac = (KS_Max(y, b.y)-a.y)/(c.y-a.y), fracbc = (KS_Max(y, b.y)-b.y)/(c.y-b.y);
                x0 = a.x+(c.x-a.x)*fracac, x1 = b.x+(c.x-b.x)*fracbc;
                z0 = a.z+(c.z-a.z)*fracac, z1 = b.z+(c.z-b.z)*fracbc;
                u0 = uv[0].u+(uv[2].u-uv[0].u)*fracac, u1 = uv[1].u+(uv[2].u-uv[1].u)*fracbc;
                v0 = uv[0].v+(uv[2].v-uv[0].v)*fracac, v1 = uv[1].v+(uv[2].v-uv[1].v)*fracbc;
                K3D_ScanLineTextured(dest, depth_buffer, texture, y, x0, z0, x1, z1, u0, u1, v0, v1);
            }
            /* Old and glitchy
            int y = KS_Max(b.y, 0);
            int y2 = KS_Min(c.y, dest->h-1);
            float skipa = (float)y-a.y;
            float skipb = (float)y-b.y;
            float fraca = 1.f / (c.y-a.y+0.0001f), fracb = 1.f / (c.y-b.y+0.0001f);
            float x0s = (c.x-a.x)*fraca, x1s = (c.x-b.x)*fracb;
            float z0s = (c.z-a.z)*fraca, z1s = (c.z-b.z)*fracb;
            float u0s = (uv[2].u-uv[0].u)*fraca, u1s = (uv[2].u-uv[1].u)*fracb;
            float v0s = (uv[2].v-uv[0].v)*fraca, v1s = (uv[2].v-uv[1].v)*fracb;
            float x0  = a.x+skipa*x0s, x1 = b.x+skipb*x1s;
            float z0  = a.z+skipa*z0s, z1 = b.z+skipb*z1s;
            float u0  = uv[0].u+skipa*u0s, u1 = uv[1].u+skipb*u1s;
            float v0  = uv[0].v+skipa*v0s, v1 = uv[1].v+skipb*v1s;
            for(; y <= y2; ++y){
                K3D_ScanLineTextured(dest, depth_buffer, texture, y, x0, z0, x1, z1, u0, u1, v0, v1);
                x0 += x0s; x1 += x1s; z0 += z0s; z1 += z1s; u0 += u0s; u1 += u1s; v0 += v0s; v1 += v1s;
            }*/
        }
    }
    
    static inline void K3D_DrawTriangleWire(ksprite_t* target, vec3_t a, vec3_t b, vec3_t c, uint32_t color){
        KS_DrawLine(target, a.x, a.y, b.x, b.y, color);
        KS_DrawLine(target, a.x, a.y, c.x, c.y, color);
        KS_DrawLine(target, b.x, b.y, c.x, c.y, color);
    }
    
    static inline void K3D_RenderQuad(vec3_t a, vec3_t b, vec3_t c, vec3_t d, uint32_t color, ksprite_t* target, float* depth_buffer){
        K3D_DrawTriangle(target, depth_buffer, a, b, c, color);
        K3D_DrawTriangle(target, depth_buffer, a, c, d, color);
    }
    
    void K3D_CreateDodecahedron(face_t faces[36], float r) {
        float s = 1.f;
        float t2 = PI / 10.f;
        float t3 = 3.f * PI / 10.f;
        float t4 = PI / 5.f;
        float d1 = s / 2.f / sinf(t4);
        float d2 = d1 * cosf(t4);
        float d3 = d1 * cosf(t2);
        float d4 = d1 * sinf(t2);
        float Fx =
            (s * s - (2.f * d3) * (2.f * d3) -
             (d1 * d1 - d3 * d3 - d4 * d4)) /
            (2.f * (d4 - d1));
        float d5 = SquareRoot(0.5f *
                              (s * s + (2.f * d3) * (2.f * d3) -
                               (d1 - Fx) * (d1 - Fx) -
                               (d4 - Fx) * (d4 - Fx) - d3 * d3));
        float Fy = (Fx * Fx - d1 * d1 - d5 * d5) / (2.f * d5);
        float Ay = d5 + Fy;
        vec3_t verts[20] = {
            Vec3MulScalar(Vec3Make(d1, Ay, 0), r),
            Vec3MulScalar(Vec3Make(d4, Ay, d3), r),
            Vec3MulScalar(Vec3Make(-d2, Ay, s / 2), r),
            Vec3MulScalar(Vec3Make(-d2, Ay, -s / 2), r),
            Vec3MulScalar(Vec3Make(d4, Ay, -d3), r),
            Vec3MulScalar(Vec3Make(Fx, Fy, 0), r),
            Vec3MulScalar(Vec3Make(Fx * sinf(t2), Fy, Fx * cosf(t2)), r),
            Vec3MulScalar(Vec3Make(-Fx * sinf(t3), Fy, Fx * cosf(t3)), r),
            Vec3MulScalar(Vec3Make(-Fx * sinf(t3), Fy, -Fx * cosf(t3)), r),
            Vec3MulScalar(Vec3Make(Fx * sinf(t2), Fy, -Fx * cosf(t2)), r),
            Vec3MulScalar(Vec3Make(Fx * sinf(t3), -Fy, Fx * cosf(t3)), r),
            Vec3MulScalar(Vec3Make(-Fx * sinf(t2), -Fy, Fx * cosf(t2)), r),
            Vec3MulScalar(Vec3Make(-Fx, -Fy, 0), r),
            Vec3MulScalar(Vec3Make(-Fx * sinf(t2), -Fy, -Fx * cosf(t2)), r),
            Vec3MulScalar(Vec3Make(Fx * sinf(t3), -Fy, -Fx * cosf(t3)), r),
            Vec3MulScalar(Vec3Make(d2, -Ay, s / 2), r),
            Vec3MulScalar(Vec3Make(-d4, -Ay, d3), r),
            Vec3MulScalar(Vec3Make(-d1, -Ay, 0), r),
            Vec3MulScalar(Vec3Make(-d4, -Ay, -d3), r),
            Vec3MulScalar(Vec3Make(d2, -Ay, -s / 2), r),
        };
        for(int i = 0; i < 12; ++i) {
            faces[i*3].c = (rand()%(255));
            faces[i*3].c = (faces[i*3].c<<16) + (faces[i*3].c<<8) + (faces[i*3].c) + (255<<24);
            faces[i*3].flags.double_sided = false;
            faces[i*3].texture_index = -1;
            for(int j = i*3+1; j < i*3+3; ++j) {
                faces[j].c = faces[i*3].c;
                faces[j].flags = faces[i].flags;
                faces[j].texture_index = faces[i].texture_index;
            }
        }
        faces[ 0].v0 = verts[ 0], faces[ 0].v1 = verts[ 2], faces[ 0].v2 = verts[ 1];
        faces[ 1].v0 = verts[ 0], faces[ 1].v1 = verts[ 3], faces[ 1].v2 = verts[ 2];
        faces[ 2].v0 = verts[ 0], faces[ 2].v1 = verts[ 4], faces[ 2].v2 = verts[ 3];
        faces[ 3].v0 = verts[ 0], faces[ 3].v1 = verts[ 1], faces[ 3].v2 = verts[ 5];
        faces[ 4].v0 = verts[ 1], faces[ 4].v1 = verts[ 6], faces[ 4].v2 = verts[ 5];
        faces[ 5].v0 = verts[ 5], faces[ 5].v1 = verts[ 6], faces[ 5].v2 = verts[10];
        faces[ 6].v0 = verts[ 0], faces[ 6].v1 = verts[ 5], faces[ 6].v2 = verts[ 9];
        faces[ 7].v0 = verts[ 0], faces[ 7].v1 = verts[ 9], faces[ 7].v2 = verts[ 4];
        faces[ 8].v0 = verts[ 9], faces[ 8].v1 = verts[ 5], faces[ 8].v2 = verts[14];
        faces[ 9].v0 = verts[ 1], faces[ 9].v1 = verts[ 2], faces[ 9].v2 = verts[ 7];
        faces[10].v0 = verts[ 1], faces[10].v1 = verts[ 7], faces[10].v2 = verts[ 6];
        faces[11].v0 = verts[ 7], faces[11].v1 = verts[11], faces[11].v2 = verts[ 6];
        faces[12].v0 = verts[ 2], faces[12].v1 = verts[ 3], faces[12].v2 = verts[ 8];
        faces[13].v0 = verts[ 2], faces[13].v1 = verts[ 8], faces[13].v2 = verts[ 7];
        faces[14].v0 = verts[ 8], faces[14].v1 = verts[12], faces[14].v2 = verts[ 7];
        faces[15].v0 = verts[ 3], faces[15].v1 = verts[ 4], faces[15].v2 = verts[ 9];
        faces[16].v0 = verts[ 3], faces[16].v1 = verts[ 9], faces[16].v2 = verts[ 8];
        faces[17].v0 = verts[ 9], faces[17].v1 = verts[13], faces[17].v2 = verts[ 8];
        faces[18].v0 = verts[ 5], faces[18].v1 = verts[10], faces[18].v2 = verts[15];
        faces[19].v0 = verts[ 5], faces[19].v1 = verts[15], faces[19].v2 = verts[19];
        faces[20].v0 = verts[ 5], faces[20].v1 = verts[19], faces[20].v2 = verts[14];
        faces[21].v0 = verts[ 9], faces[21].v1 = verts[14], faces[21].v2 = verts[19];
        faces[22].v0 = verts[ 9], faces[22].v1 = verts[19], faces[22].v2 = verts[18];
        faces[23].v0 = verts[ 9], faces[23].v1 = verts[18], faces[23].v2 = verts[13];
        faces[24].v0 = verts[ 8], faces[24].v1 = verts[13], faces[24].v2 = verts[18];
        faces[25].v0 = verts[ 8], faces[25].v1 = verts[18], faces[25].v2 = verts[17];
        faces[26].v0 = verts[ 8], faces[26].v1 = verts[17], faces[26].v2 = verts[12];
        faces[27].v0 = verts[ 7], faces[27].v1 = verts[12], faces[27].v2 = verts[17];
        faces[28].v0 = verts[ 7], faces[28].v1 = verts[17], faces[28].v2 = verts[16];
        faces[29].v0 = verts[ 7], faces[29].v1 = verts[16], faces[29].v2 = verts[11];
        faces[30].v0 = verts[ 6], faces[30].v1 = verts[11], faces[30].v2 = verts[16];
        faces[31].v0 = verts[ 6], faces[31].v1 = verts[16], faces[31].v2 = verts[15];
        faces[32].v0 = verts[ 6], faces[32].v1 = verts[15], faces[32].v2 = verts[10];
        faces[33].v0 = verts[15], faces[33].v1 = verts[16], faces[33].v2 = verts[17];
        faces[34].v0 = verts[15], faces[34].v1 = verts[17], faces[34].v2 = verts[18];
        faces[35].v0 = verts[15], faces[35].v1 = verts[18], faces[35].v2 = verts[19];
    }
    
    face_t K3D_TranslateRotate(face_t before, vec3_t translation, vec3_t rotation) {
        face_t temp1, temp2;
        for(int v = 0; v < 3; ++v) {
            // Roll
            temp2.v[v].z = before.v[v].z;
            temp2.v[v].x = cos(rotation.z)*before.v[v].x - sin(rotation.z)*before.v[v].y;
            temp2.v[v].y = sin(rotation.z)*before.v[v].x + cos(rotation.z)*before.v[v].y;
            // Pitch
            temp1.v[v].x = temp2.v[v].x;
            temp1.v[v].y = sin(rotation.x)*temp2.v[v].y - cos(rotation.x)*temp2.v[v].z;
            temp1.v[v].z = cos(rotation.x)*temp2.v[v].y + sin(rotation.x)*temp2.v[v].z;
            // Yaw
            temp2.v[v].x = sin(rotation.y)*temp1.v[v].x - cos(rotation.y)*temp1.v[v].z;
            temp2.v[v].z = cos(rotation.y)*temp1.v[v].x + sin(rotation.y)*temp1.v[v].z;
            temp2.v[v].y = temp1.v[v].y;
            // Translate 
            temp1.v[v] = Vec3Add(temp2.v[v], translation);
        }
        temp1.texture_index = before.texture_index;
        temp1.c = before.c;
        temp1.flags = before.flags;
        temp1.uv[0].u = before.uv[0].u;
        temp1.uv[1].u = before.uv[1].u;
        temp1.uv[2].u = before.uv[2].u;
        temp1.uv[0].v = before.uv[0].v;
        temp1.uv[1].v = before.uv[1].v;
        temp1.uv[2].v = before.uv[2].v;
        return temp1;
    }
    
    face_t K3D_CameraTranslateRotate(face_t before, vec3_t translation, vec3_t rotation) {
        face_t temp1, temp2;
        for(int v = 0; v < 3; ++v) {
            // Translate 
            temp1.v[v] = Vec3Sub(before.v[v], translation);
            // Yaw
            temp2.v[v].x = sin(-rotation.y)*temp1.v[v].x - cos(-rotation.y)*temp1.v[v].z;
            temp2.v[v].z = cos(-rotation.y)*temp1.v[v].x + sin(-rotation.y)*temp1.v[v].z;
            temp2.v[v].y = temp1.v[v].y;
            // Pitch
            temp1.v[v].x = temp2.v[v].x;
            temp1.v[v].y = sin(rotation.x)*temp2.v[v].y - cos(rotation.x)*temp2.v[v].z;
            temp1.v[v].z = cos(rotation.x)*temp2.v[v].y + sin(rotation.x)*temp2.v[v].z;
            // Roll
            temp2.v[v].z = temp1.v[v].z;
            temp2.v[v].x = cos(rotation.z)*temp1.v[v].x - sin(rotation.z)*temp1.v[v].y;
            temp2.v[v].y = sin(rotation.z)*temp1.v[v].x + cos(rotation.z)*temp1.v[v].y;
        }
        temp2.texture_index = before.texture_index;
        temp2.c = before.c;
        temp2.flags = before.flags;
        temp2.uv[0].u = before.uv[0].u;
        temp2.uv[1].u = before.uv[1].u;
        temp2.uv[2].u = before.uv[2].u;
        temp2.uv[0].v = before.uv[0].v;
        temp2.uv[1].v = before.uv[1].v;
        temp2.uv[2].v = before.uv[2].v;
        return temp2;
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_SOFTWARE_3D_H
#endif
