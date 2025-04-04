#ifndef KERO_3D_H
#define KERO_3D_H

/*
// Standard initial code:



void InitializeView();

int main(int argc, char* argv[]) {
KPGLInit(1920, 1080, "Notemon");

glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_TEXTURE_2D);
    
    InitializeView();
    
    koglposrot_t camera = {
    .position = { 0, 0, -10 },
    .rotation = { 0, 0, 0 },
    };
    
    bool continue_loop = true;
    while(continue_loop) {
    
    while(KPEventsQueued()) {
            kp_event_t* e = KPNextEvent();
            switch(e->type) {
                case KPEVENT_KEY_PRESS:{
                    switch(e->key) {
                        case KEY_ESCAPE:{
                            continue_loop = false;
                        }break;
                    }
                }break;
                case KPEVENT_MOUSE_BUTTON_PRESS:{
                    switch(e->button) {
                        case MOUSE_LEFT: {
                        }break;
                    }
                }break;
                case KPEVENT_RESIZE:{
InitializeView();
}break;
case KPEVENT_QUIT:{
continue_loop = false;
}break;
}
KPFreeEvent(e);
}

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        glRotatef(camera.rotation.pitch, 1, 0, 0);
        glRotatef(camera.rotation.yaw, 0, 1, 0);
        glRotatef(camera.rotation.roll, 0, 0, 1);
        glTranslatef(-camera.position.x, -camera.position.y, -camera.position.z);
        
        glPushMatrix();
        {
        }
        glPopMatrix();
        
KPGLFlip();
    }
    
    }
    
void InitializeView() {
    glViewport(0, 0, kp_window.w, kp_window.h);
    gluLookAt(0, 0, 0, 0, 0, 0, 0, 1, 0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glOrtho(0, kp_window.w, 0, kp_window.h, 0, 100);
    gluPerspective(40, (float)kp_window.w / (float)kp_window.h, 1, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

*/



#define KERO_PLATFORM_GL
#include "kero_platform.h"
#include <stdint.h>
#include <stdbool.h>


bool K3D_LoadTexture(GLuint *texture_id, const char *const filepath);
void K3D_FreeTexture(GLuint texture_id);

#include "kero_image.h"
#include "kero_vec3.h"
#include "kero_3dmodel.h"

typedef struct {
    union {
        vec3_t position;
        vec3_t pos;
        vec3_t p;
    };
    union {
        vec3_t rotation;
        vec3_t rot;
        vec3_t r;
    };
} k3d_posrot_t;

typedef struct {
    union {
        int width;
        int w;
    };
    union {
        int height;
        int h;
    };
    float u0, v0, u1, v1;
    float originx, originy;
    union {
        GLuint texture;
        GLuint tex;
        GLuint t;
    };
} k3d_sprite_t;

typedef struct {
    union {
        struct { uint32_t width, depth; };
        struct { uint32_t w, d; };
        struct { uint32_t x, z; };
    };
    union {
        float **heights;
        float **h;
        float **y;
    };
} k3d_heightmap_t;

bool K3D_LoadTexture(GLuint *texture_id, const char *const filepath) {
    kimage_t image;
    if(KI_LoadHardwareDefaults(&image, filepath) != KI_LOAD_SUCCESS) {
        fprintf(stderr, "Failed to load texture: %s\n", filepath);
        return false;
    }
    
    glGenTextures(1, texture_id);
    glBindTexture(GL_TEXTURE_2D, *texture_id);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.w, image.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
    
    KI_Free(&image);
    
    return true;
}

void K3D_FreeTexture(GLuint texture_id) {
    glDeleteTextures(1, &texture_id);
}

void K3D_ResetLightMaterial() {
    float ambient[4] = {.2, .2, .2, 1};
    //float diffuse[4] = {.8, .8, .8, 1};
    float ones[4] = {1, 1, 1, 1};
    float zeroes[4] = {0};
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ones);
    glMaterialfv(GL_FRONT, GL_SPECULAR, ones);
    glMaterialfv(GL_FRONT, GL_EMISSION, zeroes);
    glMaterialf(GL_FRONT, GL_SHININESS, 64);
}

void K3D_DrawString(const char *const string, uint32_t length) {
    glBegin(GL_TRIANGLES); {
        float x = 0, y = 0;
        float tx, ty;
        for(unsigned int c = 0; c < length; ++c) {
            switch(string[c]) {
                case '\n':{
                    --y;
                    x = 0;
                }break;
                case ' ':{
                    ++x;
                }break;
                default:
                tx = (string[c] % 16) / 16.f;
                ty = 1.f - (string[c] / 16) / 16.f;
                glTexCoord2d(tx, ty - 1.f / 16.f);
                glVertex3f(x, y - 1, 0);
                glTexCoord2d(tx + 1.f / 16.f, ty - 1.f / 16.f);
                glVertex3f(x + 1, y - 1, 0);
                glTexCoord2d(tx + 1.f / 16.f, ty);
                glVertex3f(x + 1, y, 0);
                glTexCoord2d(tx, ty - 1.f / 16.f);
                glVertex3f(x, y - 1, 0);
                glTexCoord2d(tx + 1.f / 16.f, ty);
                glVertex3f(x + 1, y, 0);
                glTexCoord2d(tx, ty);
                glVertex3f(x, y, 0);
                ++x;
            }
        }
    } glEnd();
}

void K3D_SpriteInit(k3d_sprite_t *sprite, int width, int height, float u0, float u1, float v0, float v1, float originx, float originy, GLuint texture) {
    sprite->width = width;
    sprite->height = height;
    sprite->u0 = u0;
    sprite->u1 = u1;
    sprite->v0 = v0;
    sprite->v1 = v1;
    sprite->originx = originx;
    sprite->originy = originy;
    sprite->texture = texture;
}

void K3D_DrawSprite(k3d_sprite_t *sprite, float x, float y, float z, float rotation, int framex, int framey) {
    glPushMatrix(); {
        glTranslatef(x, y, z);
        glRotatef(rotation, 0, 0, 1);
        glBindTexture(GL_TEXTURE_2D, sprite->texture);
        float texw = sprite->u1 - sprite->u0;
        float texh = sprite->v1 - sprite->v0;
        glBegin(GL_TRIANGLE_FAN); {
            glTexCoord2d(sprite->u0 + framex * texw, sprite->v0 + framey * texh);
            glVertex3f(-sprite->originx, -sprite->originy, z);
            glTexCoord2d(sprite->u0 + framex * texw, sprite->v1 + framey * texh);
            glVertex3f(-sprite->originx, sprite->h - sprite->originy, z);
            glTexCoord2d(sprite->u1 + framex * texw, sprite->v1 + framey * texh);
            glVertex3f(sprite->w - sprite->originx, sprite->h - sprite->originy, z);
            glTexCoord2d(sprite->u1 + framex * texw, sprite->v0 + framey * texh);
            glVertex3f(sprite->w - sprite->originx, -sprite->originy, z);
        } glEnd();
    } glPopMatrix();
}

void K3D_DrawSpriteScale(k3d_sprite_t *sprite, float x, float y, float z, float rotation, int framex, int framey, float scalex, float scaley) {
    glPushMatrix(); {
        glTranslatef(x, y, z);
        glRotatef(rotation, 0, 0, 1);
        glScalef(scalex, scaley, 1);
        glBindTexture(GL_TEXTURE_2D, sprite->texture);
        float texw = sprite->u1 - sprite->u0;
        float texh = sprite->v1 - sprite->v0;
        glBegin(GL_TRIANGLE_FAN); {
            glTexCoord2d(sprite->u0 + framex * texw, sprite->v0 + framey * texh);
            glVertex3f(-sprite->originx, -sprite->originy, z);
            glTexCoord2d(sprite->u0 + framex * texw, sprite->v1 + framey * texh);
            glVertex3f(-sprite->originx, sprite->height - sprite->originy, z);
            glTexCoord2d(sprite->u1 + framex * texw, sprite->v1 + framey * texh);
            glVertex3f(sprite->width - sprite->originx, sprite->height - sprite->originy, z);
            glTexCoord2d(sprite->u1 + framex * texw, sprite->v0 + framey * texh);
            glVertex3f(sprite->width - sprite->originx, -sprite->originy, z);
        } glEnd();
    } glPopMatrix();
}

#endif
