#ifndef KERO_3DMODEL_H
#define KERO_3DMODEL_H

#ifdef __cplusplus
extern "c"{
#endif
    
#include "kero_std.h"
#include "kero_math.h"
#include "kero_vec3.h"
#include "kero_vec2.h"
#include "kero_3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>
    
    typedef struct {
        union {
            struct { float r, g, b, a; };
            float rgb[4];
        };
    } color_t;
    
    typedef struct {
        char name[64];
        float stuff;
        float specular_exponent;
        float opacity;
        uint32_t illumination_model;
        color_t ambient;
        color_t diffuse;
        color_t specular;
        color_t emission;
        uint32_t texture;
    } material_t;
    
    typedef struct {
        bool use_vertex_normals;
        union {
            uint32_t v[3];
            struct { uint32_t v0, v1, v2; };
        };
        union {
            uint32_t texture_coordinates[3];
            uint32_t t[3];
            uint32_t uv[3];
        };
        union {
            uint32_t material;
            uint32_t m;
        };
        union {
            uint32_t normals[3];
            uint32_t normal[3];
            uint32_t n[3];
        };
    } face_t;
    
    typedef struct {
        union {
            vec3_t *vertices;
            vec3_t *v;
        };
        union {
            vec3_t *vertex_normals;
            vec3_t *vn;
        };
        union {
            face_t *faces;
            face_t *face;
            face_t *f;
        };
        union {
            vec2_t *texture_coordinates;
            vec2_t *tex_coords;
            vec2_t *uvs;
            vec2_t *uv;
        };
        union {
            material_t *materials;
            material_t *material;
            material_t *m;
        };
    } k3d_model_t;
    
    bool K3DM_Load(k3d_model_t *model, const char *const filepath) {
        typedef enum {
            OBJ, 
        } file_type_t;
        file_type_t file_type;
        if(strstr(filepath, "obj") != NULL) {
            file_type = OBJ;
        }
        switch(file_type) {
            case OBJ:{
                // Associated MTL file
                {
                    char *mtl_filename = malloc(64);
                    mtl_filename = strcat(strncpy(mtl_filename, filepath, strlen(filepath) - 3), "mtl");
                    char *data;
                    uint32_t byte_count;
                    kstd_error_t error;
                    if((error = KStd_ReadFile(mtl_filename, (uint8_t**)&data, &byte_count)) != KSTD_ERROR_SUCCESS) {
                        switch(error) {
                            case KSTD_ERROR_FILENOTFOUND:{
                                fprintf(stderr, "K3DM_Load() error: File not found: %s\n", mtl_filename);
                            }break;
                            case KSTD_ERROR_FILESEEK:{
                                fprintf(stderr, "K3DM_Load() error: fseek() failed: %s\n", mtl_filename);
                            }break;
                            case KSTD_ERROR_FILESIZE:{
                                fprintf(stderr, "K3DM_Load() error: File size invalid: %s\n", mtl_filename);
                            }break;
                            case KSTD_ERROR_MALLOC:{
                                fprintf(stderr, "K3DM_Load() error: Failed to allocate memory: %s\n", mtl_filename);
                            }break;
                            case KSTD_ERROR_FILEREAD:{
                                fprintf(stderr, "K3DM_Load() error: fread() failed: %s\n", mtl_filename);
                            }break;
                            default:{
                                fprintf(stderr, "K3DM_Load() error: Unknown error: %s\n", mtl_filename);
                            }break;
                        }
                        free(mtl_filename);
                        return false;
                    }
                    free(mtl_filename);
                    KDA_Init(model->material, 1);
                    char c;
                    char *line;
                    char *line_next = data;
                    bool last_line = false;
                    bool line_found = false;
                    material_t material_temp = {0};
                    char *float_next;
                    long l;
                    for(uint32_t i = 0; i < byte_count;) {
                        // New line
                        while(!line_found) {
                            c = data[i];
                            switch(c) {
                                case EOF:
                                case '\n':{
                                    data[i] = '\0';
                                    line = line_next;
                                    line_next = &data[i + 1];
                                    line_found = true;
                                }break;
                            }
                            ++i;
                        }
                        line_found = false;
                        switch(line[0]) {
                            case '#':{
#ifdef KERO_3D_VERBOSE
                                fprintf(stderr, "Comment: %s\n", &line[0]);
#endif
                            }break;
                            case 'n':{
#ifdef KERO_3D_VERBOSE
                                fprintf(stderr, "New material: %s\n", &line[0]);
#endif
                                if(line[1] == 'e' && line[2] == 'w' && line[3] == 'm' && line[4] == 't' && line[5] == 'l') {
                                    if(material_temp.name[0] != '\0') {
                                        KDA_Push(model->material, material_temp);
                                    }
                                    memset(&material_temp, 0, sizeof(material_temp));
                                    material_temp.diffuse.a = material_temp.specular.a = material_temp.ambient.a = 1.f;
                                    material_temp.opacity = 1;
                                    material_temp.texture = 0;
                                    if(strlen(&line[7]) > 64) {
#ifdef KERO_3D_VERBOSE
                                        fprintf(stderr, "K3DM_Load(): Loading: %s. . . Material name longer than 64 characters: %s\n", filepath, &line[7]);
#endif
                                        free(data);
                                        KDA_Free(model->material);
                                        return false;
                                    }
                                    strcpy(&material_temp.name[0], &line[7]);
                                }
                            }break;
                            case 'K':{
                                switch(line[1]) {
                                    case 'a':{ // Ambient colour
                                        float_next = &line[2];
                                        for(int c = 0; c < 3; ++c) {
                                            if((material_temp.ambient.rgb[c] = strtod(float_next, &float_next)) == HUGE_VAL || float_next == NULL) {
#ifdef KERO_3D_VERBOSE
                                                fprintf(stderr, "K3DM_Load(): Loading: %s. . . Ambient colour format error: %s\n", filepath, &line[0]);
#endif
                                                free(data);
                                                KDA_Free(model->material);
                                                return false;
                                            }
                                        }
                                    }break;
                                    case 'd':{ // Diffuse colour
                                        float_next = &line[2];
                                        for(int c = 0; c < 3; ++c) {
                                            if((material_temp.diffuse.rgb[c] = strtod(float_next, &float_next)) == HUGE_VAL || float_next == NULL) {
#ifdef KERO_3D_VERBOSE
                                                fprintf(stderr, "K3DM_Load(): Loading: %s. . . Diffuse colour format error: %s\n", filepath, &line[0]);
#endif
                                                free(data);
                                                KDA_Free(model->material);
                                                return false;
                                            }
                                        }
                                    }break;
                                    case 's':{ // Specular colour
                                        float_next = &line[2];
                                        for(int c = 0; c < 3; ++c) {
                                            if((material_temp.specular.rgb[c] = strtod(float_next, &float_next)) == HUGE_VAL || float_next == NULL) {
#ifdef KERO_3D_VERBOSE
                                                fprintf(stderr, "K3DM_Load(): Loading: %s. . . Specular colour format error: %s\n", filepath, &line[0]);
#endif
                                                free(data);
                                                KDA_Free(model->material);
                                                return false;
                                            }
                                        }
                                    }break;
                                }
                            }break;
                            case 'N': {
                                if(line[1] == 's') { // Specular exponent
                                    if((material_temp.specular_exponent = strtod(&line[2], NULL)) == HUGE_VAL) {
#ifdef KERO_3D_VERBOSE
                                        fprintf(stderr, "K3DM_Load(): Loading: %s. . . Specular exponent format error: %s\n", filepath, &line[0]);
#endif
                                        free(data);
                                        KDA_Free(model->material);
                                        return false;
                                    }
                                    material_temp.specular_exponent /= 7.8125f;
                                }
                            }break;
                            case 'd':{ // Opacity 0 = invisible, 1 = opaque
                                if((material_temp.opacity = strtod(&line[1], NULL)) == HUGE_VAL) {
#ifdef KERO_3D_VERBOSE
                                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . Opacity format error: %s\n", filepath, &line[0]);
#endif
                                    free(data);
                                    KDA_Free(model->material);
                                    return false;
                                }
                            }break;
                            case 'T':{ // Inverse opacity 0 = opaque, 1 = invisible
                                if(line[1] == 'r') {
                                    if((material_temp.opacity = strtod(&line[1], NULL)) == HUGE_VAL) {
#ifdef KERO_3D_VERBOSE
                                        fprintf(stderr, "K3DM_Load(): Loading: %s. . . Opacity format error: %s\n", filepath, &line[0]);
#endif
                                        free(data);
                                        KDA_Free(model->material);
                                        return false;
                                    }
                                    material_temp.opacity = 1.f - material_temp.opacity;
                                }
                            }break;
                            case 'i':{
                                if(line[1] == 'l' && line[2] == 'l' && line[3] == 'u' && line[4] == 'm') { // Illumination model
                                    if((l = strtol(&line[5], NULL, 10)) == LONG_MAX || l == LONG_MIN) {
#ifdef KERO_3D_VERBOSE
                                        fprintf(stderr, "K3DM_Load(): Loading: %s. . . Illumination model format error: %s\n", filepath, &line[0]);
#endif
                                        free(data);
                                        KDA_Free(model->material);
                                        return false;
                                    }
                                    // For now, expect illumination model 2 but eh who knows?
                                    material_temp.illumination_model = (uint32_t)l;
                                }
                            }break;
                            case 'm':{
                                if(line[1] == 'a' && line[2] == 'p' && line[3] == '_') {
                                    if(line[4] == 'K' && line[5] == 'd') { // Diffuse map
                                        if(!K3D_LoadTexture(&material_temp.texture, &line[7])) {
#ifdef KERO_3D_VERBOSE
                                            fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to load texture: %s\n", filepath, &line[0]);
#endif
                                        }
                                    }
                                    else if(line[4] == 'd') { // Opacity map
                                    }
                                }
                            }break;
                        }
                    }
                    if(material_temp.name[0] != '\0') {
                        KDA_Push(model->material, material_temp);
                    }
                    free(data);
                    KDA_Shrink(model->material);
                }
                
                // OBJ file itself
                char *data;
                uint32_t byte_count;
                kstd_error_t error;
                if((error = KStd_ReadFile(filepath, (uint8_t**)&data, &byte_count)) != KSTD_ERROR_SUCCESS) {
#ifdef KERO_3D_VERBOSE
                    switch(error) {
                        case KSTD_ERROR_FILENOTFOUND:{
                            fprintf(stderr, "K3DM_Load() error: File not found: %s\n", filepath);
                        }break;
                        case KSTD_ERROR_FILESEEK:{
                            fprintf(stderr, "K3DM_Load() error: fseek() failed: %s\n", filepath);
                        }break;
                        case KSTD_ERROR_FILESIZE:{
                            fprintf(stderr, "K3DM_Load() error: File size invalid: %s\n", filepath);
                        }break;
                        case KSTD_ERROR_MALLOC:{
                            fprintf(stderr, "K3DM_Load() error: Failed to allocate memory: %s\n", filepath);
                        }break;
                        case KSTD_ERROR_FILEREAD:{
                            fprintf(stderr, "K3DM_Load() error: fread() failed: %s\n", filepath);
                        }break;
                        default:{
                            fprintf(stderr, "K3DM_Load() error: Unknown error: %s\n", filepath);
                        }break;
                    }
#endif
                    return false;
                }
                char c;
                char *line;
                char *line_next = data;
                bool last_line = false;
                if(KDA_Init(model->vertices, 128) == NULL) {
#ifdef KERO_3D_VERBOSE
                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to allocate memory for vertices.\n", filepath);
#endif
                    KDA_Free(model->materials);
                    return false;
                }
                if(KDA_Init(model->faces, 128) == NULL) {
#ifdef KERO_3D_VERBOSE
                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to allocate memory for faces.\n", filepath);
#endif
                    KDA_Free(model->vertices);
                    KDA_Free(model->materials);
                    return false;
                }
                if(KDA_Init(model->texture_coordinates, 128) == NULL) {
#ifdef KERO_3D_VERBOSE
                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to allocate memory for texture coordinates.\n", filepath);
#endif
                    KDA_Free(model->vertices);
                    KDA_Free(model->faces);
                    KDA_Free(model->materials);
                    return false;
                }
                if(KDA_Init(model->vertex_normals, 128) == NULL) {
#ifdef KERO_3D_VERBOSE
                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to allocate memory for vertex normals.\n", filepath);
#endif
                    KDA_Free(model->vertices);
                    KDA_Free(model->faces);
                    KDA_Free(model->materials);
                    KDA_Free(model->texture_coordinates);
                    return false;
                }
                char *float_next;
                bool line_found = false;
                long l;
                vec3_t vertex_temp;
                vec2_t tex_coord_temp;
                face_t face_temp = {0};
                for(int i = 0; i < byte_count && !last_line;) {
                    // New line
                    while(!line_found) {
                        c = data[i];
                        switch(c) {
                            case EOF:
                            case '\n':{
                                data[i] = '\0';
                                line = line_next;
                                line_next = &data[i + 1];
                                line_found = true;
                            }break;
                        }
                        ++i;
                    }
                    line_found = false;
                    switch(line[0]) {
                        case '#':{
#ifdef KERO_3D_VERBOSE
                            fprintf(stderr, "Comment: %s\n", &line[0]);
#endif
                        }break;
                        case 'v':{
#ifdef KERO_3D_VERBOSE
                            fprintf(stderr, "Vertex: %s\n", &line[0]);
#endif
                            switch(line[1]) {
                                case ' ':{
                                    float_next = &line[2];
                                    for(int i = 0; i < 3; ++i) {
                                        if((vertex_temp.V[i] = strtod(float_next, &float_next)) == HUGE_VAL || float_next == NULL ) {
#ifdef KERO_3D_VERBOSE
                                            fprintf(stderr, "K3DM_Load(): Loading: %s. . . File format error: Failed to read vertex.\n", filepath);
#endif
                                            free(data);
                                            KDA_Free(model->vertices);
                                            KDA_Free(model->faces);
                                            KDA_Free(model->materials);
                                            KDA_Free(model->texture_coordinates);
                                            return false;
                                        }
                                    }
                                    if(KDA_Push(model->vertices, vertex_temp) == NULL) {
#ifdef KERO_3D_VERBOSE
                                        fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to realloc vertices\n", filepath);
#endif
                                        free(data);
                                        KDA_Free(model->faces);
                                        KDA_Free(model->materials);
                                        KDA_Free(model->texture_coordinates);
                                        return false;
                                    }
                                }break;
                                case 't':{
                                    float_next = &line[2];
                                    for(int t = 0; t < 2; ++t) {
                                        if((tex_coord_temp.V[t] = strtod(float_next, &float_next)) == HUGE_VAL || float_next == NULL) {
#ifdef KERO_3D_VERBOSE
                                            fprintf(stderr, "K3DM_Load(): Loading: %s. . . File format error: Failed to read texture coordinates.\n", filepath);
#endif
                                            free(data);
                                            KDA_Free(model->vertices);
                                            KDA_Free(model->faces);
                                            KDA_Free(model->materials);
                                            KDA_Free(model->texture_coordinates);
                                            return false;
                                        }
                                    }
                                    if(KDA_Push(model->texture_coordinates, tex_coord_temp) == NULL) {
#ifdef KERO_3D_VERBOSE
                                        fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to realloc texture coordinates\n", filepath);
#endif
                                        free(data);
                                        KDA_Free(model->faces);
                                        KDA_Free(model->materials);
                                        KDA_Free(model->vertices);
                                        return false;
                                    }
                                }break;
                                case 'p':{
                                }break;
                                case 'n': { // Vertex normal
                                    float_next = &line[3];
                                    for(int i = 0; i < 3; ++i) {
                                        if((vertex_temp.V[i] = strtod(float_next, &float_next)) == HUGE_VAL || float_next == NULL ) {
#ifdef KERO_3D_VERBOSE
                                            fprintf(stderr, "K3DM_Load(): Loading: %s. . . File format error: Failed to read vertex normal.\n", filepath);
#endif
                                            free(data);
                                            KDA_Free(model->vertices);
                                            KDA_Free(model->faces);
                                            KDA_Free(model->materials);
                                            KDA_Free(model->texture_coordinates);
                                            return false;
                                        }
                                    }
                                    if(KDA_Push(model->vn, vertex_temp) == NULL) {
#ifdef KERO_3D_VERBOSE
                                        fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to realloc vertex normals\n", filepath);
#endif
                                        free(data);
                                        KDA_Free(model->faces);
                                        KDA_Free(model->materials);
                                        KDA_Free(model->texture_coordinates);
                                        return false;
                                    }
                                }break;
                            }
                        }break;
                        case 'f':{
#ifdef KERO_3D_VERBOSE
                            fprintf(stderr, "Face: %s\n", &line[0]);
#endif
                            float_next = &line[1];
                            for(int i = 0; i < 3; ++i) {
                                face_temp.uv[i] = face_temp.n[i] = 0;
                                if((l = strtol(float_next, &float_next, 10)) == LONG_MAX || l == LONG_MIN || float_next == NULL) {
#ifdef KERO_3D_VERBOSE
                                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . File format error: Failed to read face.\n", filepath);
#endif
                                    free(data);
                                    KDA_Free(model->vertices);
                                    KDA_Free(model->faces);
                                    KDA_Free(model->materials);
                                    KDA_Free(model->texture_coordinates);
                                    return false;
                                }
                                if(l < 0) {
                                    face_temp.v[i] = KDA_Top(model->vertices) - (int)l;
                                }
                                else {
                                    face_temp.v[i] = (int)l - 1;
                                }
                                if(*float_next == '/') {
                                    ++float_next;
                                    // Texture coordinate index
                                    if(*float_next != '/') {
                                        if((l = strtol(float_next, &float_next, 10)) == LONG_MAX || l == LONG_MIN || float_next == NULL) {
#ifdef KERO_3D_VERBOSE
                                            fprintf(stderr, "K3DM_Load(): Loading: %s. . . File format error: Failed to read face.\n", filepath);
#endif
                                            free(data);
                                            KDA_Free(model->vertices);
                                            KDA_Free(model->faces);
                                            KDA_Free(model->materials);
                                            KDA_Free(model->texture_coordinates);
                                            return false;
                                        }
                                        face_temp.uv[i] = (uint32_t)(l - 1);
                                    }
                                }
                                if(*float_next == '/') {
                                    // Normal index
                                    if((l = strtol(float_next + 1, &float_next, 10)) == LONG_MAX || l == LONG_MIN) {
#ifdef KERO_3D_VERBOSE
                                        fprintf(stderr, "K3DM_Load(): Loading: %s. . . File format error: Failed to read face.\n", filepath);
#endif
                                        free(data);
                                        KDA_Free(model->vertices);
                                        KDA_Free(model->faces);
                                        KDA_Free(model->materials);
                                        KDA_Free(model->texture_coordinates);
                                        return false;
                                    }
                                    face_temp.n[i] = (uint32_t)(l - 1);
                                }
                            }
                            if(KDA_Push(model->faces, face_temp) == NULL) {
#ifdef KERO_3D_VERBOSE
                                fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to realloc faces\n", filepath);
#endif
                                free(data);
                                KDA_Free(model->texture_coordinates);
                                KDA_Free(model->materials);
                                KDA_Free(model->vertices);
                                return false;
                            }
                        }break;
                        case 'u':{
                            if(line[1] == 's' && line[2] == 'e' && line[3] == 'm' && line[4] == 't' && line[5] == 'l') {
#ifdef KERO_3D_VERBOSE
                                fprintf(stderr, "Material: %s\n", line);
#endif
                                int found = -1;
                                for(int i = 0; i < KDA_Top(model->material); ++i) {
                                    if(strstr(model->material[i].name, &line[7])) {
                                        found = i;
                                        break;
                                    }
                                }
                                if(found != -1) {
                                    face_temp.m = found;
                                }
                                else {
                                    face_temp.m = 0;
                                }
                            }
                        }break;
                        case 'm':{
                            if(line[1] == 't' && line[2] == 'l' && line[3] == 'l' && line[4] == 'i' && line[5] == 'b') {
                            }
                        }break;
                    }
                }
                free(data);
                if(KDA_Shrink(model->vertices) == NULL) {
#ifdef KERO_3D_VERBOSE
                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to allocate memory for vertices.\n", filepath);
#endif
                    KDA_Free(model->faces);
                    KDA_Free(model->materials);
                    KDA_Free(model->texture_coordinates);
                    KDA_Free(model->vertex_normals);
                    return false;
                }
                if(KDA_Shrink(model->faces) == NULL) {
#ifdef KERO_3D_VERBOSE
                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to allocate memory for faces.\n", filepath);
#endif
                    KDA_Free(model->vertices);
                    KDA_Free(model->materials);
                    KDA_Free(model->texture_coordinates);
                    KDA_Free(model->vertex_normals);
                    return false;
                }
                if(KDA_Shrink(model->vertex_normals) == NULL) {
#ifdef KERO_3D_VERBOSE
                    fprintf(stderr, "K3DM_Load(): Loading: %s. . . Failed to allocate memory for vertex normals.\n", filepath);
#endif
                    KDA_Free(model->vertices);
                    KDA_Free(model->materials);
                    KDA_Free(model->texture_coordinates);
                    KDA_Free(model->faces);
                    return false;
                }
                // Calculate face and vertex normals
                /*for(int i = 0; i < KDA_Top(model->faces); ++i) {
                    model->faces[i].normal = Vec3Cross(Vec3AToB(model->vertices[model->faces[i].v0], model->vertices[model->faces[i].v1]), Vec3AToB(model->vertices[model->faces[i].v0], model->vertices[model->faces[i].v2]));
                }
                uint32_t *vertex_faces;
                KDA_Init(vertex_faces, 64);
                float total_area;
                float *face_area;
                KDA_Init(face_area, 64);
                KDA_Init(model->vn, KDA_Top(model->vertices));
                for(int v = 0; v < KDA_Top(model->vertices); ++v) {
                    total_area = 0;
                    for(int f = 0; f < KDA_Top(model->faces); ++f) {
                        //fprintf(stderr, "%d ", f);
                        if(model->faces[f].v0 == v || model->faces[f].v1 == v || model->faces[f].v2 == v) {
                            KDA_Push(vertex_faces, f);
                            KDA_Push(face_area, Vec3Length(model->faces[f].normal));
                            total_area += face_area[KDA_Top(face_area) - 1];
                        }
                    }
                    model->vn[v] = Vec3Make(0, 0, 0);
                    for(int f = 0; f < KDA_Top(vertex_faces); ++f) {
                        model->vn[v].x += model->faces[vertex_faces[f]].n.x * face_area[f] / total_area;
                        model->vn[v].y += model->faces[vertex_faces[f]].n.y * face_area[f] / total_area;
                        model->vn[v].z += model->faces[vertex_faces[f]].n.z * face_area[f] / total_area;
                    }
                    model->vn[v] = Vec3Norm(model->vn[v]);
                    KDA_Top(vertex_faces) = 0;
                    KDA_Top(face_area) = 0;
                }
                KDA_Free(vertex_faces);
                for(int i = 0; i < KDA_Top(model->faces); ++i) {
                    model->faces[i].normal = Vec3Norm(model->faces[i].normal);
                }*/
            }break;
        }
#ifdef KERO_3D_VERBOSE
        fprintf(stderr, "K3DM_Load(): Successfully loaded: %s\n", filepath);
#endif
        return true;
    }
    
    void K3DM_Draw(k3d_model_t *model) {
        //glBegin(GL_TRIANGLES);
        uint32_t last_material = -1;
        for(int i = 0; i < 2; ++i) {
            for(uint32_t f = 0; f < KDA_Top(model->faces); ++f) {
                if((i == 0 && model->material[model->faces[f].m].opacity == 1.f) || (i == 1 && model->material[model->faces[f].m].opacity < 1.f)) {
                    if(model->faces[f].m != last_material) {
                        last_material = model->faces[f].m;
                        glMaterialfv(GL_FRONT, GL_AMBIENT, model->material[model->faces[f].m].ambient.rgb);
                        glMaterialfv(GL_FRONT, GL_DIFFUSE, model->material[model->faces[f].m].diffuse.rgb);
                        glMaterialfv(GL_FRONT, GL_SPECULAR, model->material[model->faces[f].m].specular.rgb);
                        glMaterialfv(GL_FRONT, GL_EMISSION, model->material[model->faces[f].m].emission.rgb);
                        glMaterialf(GL_FRONT, GL_SHININESS, model->material[model->faces[f].m].specular_exponent);
                        if(model->material[model->faces[f].m].texture) {
                            glEnd();
                            glBindTexture(GL_TEXTURE_2D, model->material[model->faces[f].m].texture);
                            glBegin(GL_TRIANGLES);
                        }
                        else {
                            glEnd();
                            glBindTexture(GL_TEXTURE_2D, 0);
                            glBegin(GL_TRIANGLES);
                        }
                    }
                    glColor4f(model->material[model->faces[f].m].diffuse.r, model->material[model->faces[f].m].diffuse.g, model->material[model->faces[f].m].diffuse.b, model->material[model->faces[f].m].opacity);
                    // Face normal
                    /*if(!model->faces[f].use_vertex_normals) {
                        glNormal3f(model->faces[f].n.x, model->faces[f].n.y, model->faces[f].n.z);
                        glVertex3f(model->vertices[model->faces[f].v0].x, model->vertices[model->faces[f].v0].y, model->vertices[model->faces[f].v0].z);
                        glVertex3f(model->vertices[model->faces[f].v1].x, model->vertices[model->faces[f].v1].y, model->vertices[model->faces[f].v1].z);
                        glVertex3f(model->vertices[model->faces[f].v2].x, model->vertices[model->faces[f].v2].y, model->vertices[model->faces[f].v2].z);
                    }
                    else {
                    glNormal3f(model->vertex_normals[model->faces[f].v0].x, model->vertex_normals[model->faces[f].v0].y, model->vertex_normals[model->faces[f].v0].z);
                    glVertex3f(model->vertices[model->faces[f].v0].x, model->vertices[model->faces[f].v0].y, model->vertices[model->faces[f].v0].z);
                    glNormal3f(model->vertex_normals[model->faces[f].v1].x, model->vertex_normals[model->faces[f].v1].y, model->vertex_normals[model->faces[f].v1].z);
                    glVertex3f(model->vertices[model->faces[f].v1].x, model->vertices[model->faces[f].v1].y, model->vertices[model->faces[f].v1].z);
                    glNormal3f(model->vertex_normals[model->faces[f].v2].x, model->vertex_normals[model->faces[f].v2].y, model->vertex_normals[model->faces[f].v2].z);
                    glVertex3f(model->vertices[model->faces[f].v2].x, model->vertices[model->faces[f].v2].y, model->vertices[model->faces[f].v2].z);
                    }*/
                    if(model->material[model->faces[f].m].texture) {
                        glTexCoord2d(model->uv[model->faces[f].uv[0]].x, model->uv[model->faces[f].uv[0]].y);
                    }
                    glNormal3f(model->vertex_normals[model->faces[f].n[0]].x, model->vertex_normals[model->faces[f].n[0]].y, model->vertex_normals[model->faces[f].n[0]].z);
                    glVertex3f(model->vertices[model->faces[f].v0].x, model->vertices[model->faces[f].v0].y, model->vertices[model->faces[f].v0].z);
                    if(model->material[model->faces[f].m].texture) {
                        glTexCoord2d(model->uv[model->faces[f].uv[1]].x, model->uv[model->faces[f].uv[1]].y);
                    }
                    glNormal3f(model->vertex_normals[model->faces[f].n[1]].x, model->vertex_normals[model->faces[f].n[1]].y, model->vertex_normals[model->faces[f].n[1]].z);
                    glVertex3f(model->vertices[model->faces[f].v1].x, model->vertices[model->faces[f].v1].y, model->vertices[model->faces[f].v1].z);
                    if(model->material[model->faces[f].m].texture) {
                        glTexCoord2d(model->uv[model->faces[f].uv[2]].x, model->uv[model->faces[f].uv[2]].y);
                    }
                    glNormal3f(model->vertex_normals[model->faces[f].n[2]].x, model->vertex_normals[model->faces[f].n[2]].y, model->vertex_normals[model->faces[f].n[2]].z);
                    glVertex3f(model->vertices[model->faces[f].v2].x, model->vertices[model->faces[f].v2].y, model->vertices[model->faces[f].v2].z);
                }
            }
        }
        //glEnd();
    }
    
#ifdef __cplusplus
}
#endif

#endif
