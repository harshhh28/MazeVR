#ifndef KERO_COLOR_H

#ifdef __cplusplus
extern "C"{
#endif
    
#include <stdint.h>
    
    typedef struct {
        union {
            struct { float r, g, b, a; };
            float f[4];
        };
    } kero_colorf_t;
    typedef struct {
        union {
            struct { uint8_t b, g, r, a; };
            uint32_t abgr;
        };
    } kero_colorb_t;
    
    uint32_t KC_RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a){
        return b + (g<<8) + (r<<16) + (a<<24);
    }
    
    uint32_t KC_Darken(uint32_t color, float brightness){
        brightness = Min(1, Max(0, brightness));
        int b = (float)((uint8_t)color) * brightness;
        int g = (float)((uint8_t)(color>>8)) * brightness;
        int r = (float)((uint8_t)(color>>16)) * brightness;
        int a = (uint8_t)(color>>24);
        return b + (g<<8) + (r<<16) + (a<<24);
    }
    
    void KC_HSVToRGB(float h, float s, float v, int* r, int* g, int* b) {
        h = Absolute(h/60.f);
        float c = v * s;
        float x = c * (1.f - Absolute(FMod(h, 2) - 1.f));
        float m = v - c;
        int hue_prime = (int)(h)%6;
        switch(hue_prime) {
            case 0:{
                *r = (c + m) * 255, *g = (x + m) * 255, *b = m * 255;
            }break;
            case 1:{
                *r = (x + m) * 255, *g = (c + m) * 255, *b = m * 255;
            }break;
            case 2:{
                *r = m * 255, *g = (c + m) * 255, *b = (x + m) * 255;
            }break;
            case 3:{
                *r = m * 255, *g = (x + m) * 255, *b = (c + m) * 255;
            }break;
            case 4:{
                *r = (x + m) * 255, *g = m * 255, *b = (c + m) * 255;
            }break;
            case 5:{
                *r = (c + m) * 255, *g = m * 255, *b = (x + m) * 255;
            }break;
        }
        
    }
    
    uint32_t KC_HSVToRGBu32(float h, float s, float v) {
        h = Absolute(h/60.f);
        float c = v * s;
        float x = c * (1.f - Absolute(FMod(h, 2) - 1.f));
        float m = v - c;
        int hue_prime = (int)(h)%6;
        
        uint8_t r, g, b;
        switch(hue_prime) {
            case 0:{
                r = (c + m) * 255, g = (x + m) * 255, b = m * 255;
            }break;
            case 1:{
                r = (x + m) * 255, g = (c + m) * 255, b = m * 255;
            }break;
            case 2:{
                r = m * 255, g = (c + m) * 255, b = (x + m) * 255;
            }break;
            case 3:{
                r = m * 255, g = (x + m) * 255, b = (c + m) * 255;
            }break;
            case 4:{
                r = (x + m) * 255, g = m * 255, b = (c + m) * 255;
            }break;
            case 5:{
                r = (c + m) * 255, g = m * 255, b = (x + m) * 255;
            }break;
        }
        
        return (b) + (g << 8) + (r << 16) + (255 << 24);
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_COLOR_H
#endif