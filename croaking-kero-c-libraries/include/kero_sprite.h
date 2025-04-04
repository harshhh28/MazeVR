#if !defined(KERO_SPRITE_H)

#ifdef __cplusplus
extern "C"{
#endif
    
#include <string.h>
#include <stdint.h>
#include <math.h>
    
#define KS_Max(a, b) ((a)>(b)?(a):(b))
#define KS_Min(a, b) ((a)<(b)?(a):(b))
#define KS_Absolute(a) ((a)>0?(a):(-(a)))
#define KS_Square(a) ((a)*(a))
    float KS_SquareRootf(float a)
    {
        float precision = a*0.00001f;
        float root = a;
        while ((root - a/root) > precision)
        {
            root= (root + a/root) / 2;
        }
        return root;
    }
#define KS_LineLength(ax,ay,bx,by) KS_SquareRootf(KS_Square((float)(bx)-(float)(ax))+KS_Square((float)(by)-(float)(ay)))
    int KS_Roundf(float a){ return (int)(a+0.5f); }
#define KS_Swap(type, a, b) {type t = a; a = b; b = t;}
    
#define KS_BLACK 0xff000000
#define KS_WHITE 0xffffffff
    
    typedef struct{
        int w, h;
        uint32_t* pixels;
    } ksprite_t;
    
#ifdef KERO_IMAGE_H
    bool KS_Load(ksprite_t* sprite, char* filepath){
        if(KI_LoadSoftwareDefaults((kimage_t*)sprite, filepath) != KI_LOAD_SUCCESS){
            return false;
        }
        return true;
    }
#endif
    
    static inline void KS_Create(ksprite_t* sprite, int w, int h){
        sprite->w = w;
        sprite->h = h;
        sprite->pixels = (uint32_t*)malloc(w*h*sizeof(uint32_t));
    }
    
    static inline void KS_Free(ksprite_t* sprite) {
        free(sprite->pixels);
    }
    
    void KS_CreateFromSprite(ksprite_t* sprite, ksprite_t* source, int left, int top, int right, int bottom) {
        KS_Create(sprite, right-left, bottom-top);
        for(int y = 0; y < sprite->h; ++y){
            for(int x = 0; x < sprite->w; ++x){
                sprite->pixels[y*sprite->w+x] = source->pixels[(y+top)*source->w + x+left];
            }
        }
    }
    
    static inline uint32_t KS_GetPixel(ksprite_t* source, int x, int y){
        return source->pixels[y*source->w + x];
    }
    
    static inline uint32_t KS_SampleWrapped(ksprite_t* source, float x, float y){
        int ix = Absolute(((int)(x*source->w))%source->w);
        int iy = Absolute(((int)(y*source->h))%source->h);
        return source->pixels[ix + iy*source->w];
    }
    
    static inline uint32_t KS_GetPixelSafe(ksprite_t* source, int x, int y){
        if(x < 0 || x > source->w-1 || y < 0 || y > source->h-1) return 0;
        return source->pixels[y*source->w + x];
    }
    
    static inline void KS_SetPixel(ksprite_t* dest, int x, int y, uint32_t pixel){
        *(dest->pixels + y*dest->w + x) = pixel;
    }
    
    static inline void KS_SetPixelSafe(ksprite_t* dest, int x, int y, uint32_t pixel){
        if(x < 0 || x > dest->w-1 || y < 0 || y > dest->h-1) return;
        *(dest->pixels + y*dest->w + x) = pixel;
    }
    
    static inline void KS_SetPixelAlpha10(ksprite_t* dest, int x, int y, uint32_t pixel) {
        if(pixel>>24 == 0) return;
        dest->pixels[y*dest->w + x] = pixel;
    }
    
    static inline void KS_SetPixelAlpha10Safe(ksprite_t* dest, int x, int y, uint32_t pixel) {
        if(pixel>>24 == 0 || x < 0 || x > dest->w-1 || y < 0 || y > dest->h-1) return;
        dest->pixels[y*dest->w + x] = pixel;
    }
    
    void KS_SetPixelWithAlpha(ksprite_t* dest, int x, int y, uint32_t pixel){
        if(pixel>>24 == 0 || x < 0 || x > dest->w-1 || y < 0 || y > dest->h-1) return;
        *(dest->pixels + y*dest->w + x) = pixel;
    }
    
    void KS_ColorToTransparent(ksprite_t* sprite, uint32_t color) {
        for(int y = 0; y < sprite->h; ++y) {
            for(int x = 0; x < sprite->w; ++x) {
                if(sprite->pixels[y*sprite->w+x] == color) {
                    sprite->pixels[y*sprite->w+x] = 0;
                }
            }
        }
    }
    
    void KS_ColorKey(ksprite_t* sprite, uint32_t color_before, uint32_t color_after) {
        for(int y = 0; y < sprite->h; ++y) {
            for(int x = 0; x < sprite->w; ++x) {
                if(sprite->pixels[y*sprite->w+x] == color_before) {
                    sprite->pixels[y*sprite->w+x] = color_after;
                }
            }
        }
    }
    
    void KS_BlendPixel(ksprite_t* dest, int x, int y, uint32_t pixel){
        uint8_t* dest_pixel = (uint8_t*)(dest->pixels + y*dest->w + x);
        float alpha = (float)(pixel>>24) / 255.f;
        float one_minus_alpha = 1.f - alpha;
        *dest_pixel = (uint8_t)(pixel) * alpha + (*dest_pixel) * one_minus_alpha;
        ++dest_pixel;
        *dest_pixel = (uint8_t)(pixel>>8) * alpha + (*dest_pixel) * one_minus_alpha;
        ++dest_pixel;
        *dest_pixel = (uint8_t)(pixel>>16) * alpha + (*dest_pixel) * one_minus_alpha;
        ++dest_pixel;
        *dest_pixel += (255 - *dest_pixel) * alpha;
    }
    
    void KS_BlendPixelSafe(ksprite_t* dest, int x, int y, uint32_t pixel){
        if(x < 0 || x > dest->w-1 || y < 0 || y > dest->h-1) return;
        uint8_t* dest_pixel = (uint8_t*)(dest->pixels + y*dest->w + x);
        float alpha = (float)(pixel>>24) / 255.f;
        float one_minus_alpha = 1.f - alpha;
        *dest_pixel = (uint8_t)(pixel) * alpha + (*dest_pixel) * one_minus_alpha;
        ++dest_pixel;
        *dest_pixel = (uint8_t)(pixel>>8) * alpha + (*dest_pixel) * one_minus_alpha;
        ++dest_pixel;
        *dest_pixel = (uint8_t)(pixel>>16) * alpha + (*dest_pixel) * one_minus_alpha;
        ++dest_pixel;
        *dest_pixel += (255 - *dest_pixel) * alpha;
    }
    
    static inline void KS_Blit(ksprite_t* source, ksprite_t* dest, int x, int y){
        int left = x;
        int top = y;
        int left_clip = KS_Max(0, -left);
        int right_clip = KS_Max(0, left + source->w - dest->w);
        int top_clip = KS_Max(0, -top);
        int bottom_clip = KS_Max(0, top + source->h - dest->h);
        for(int sourcey = top_clip; sourcey < source->h - bottom_clip; ++sourcey){
            for(int sourcex = left_clip; sourcex < source->w - right_clip; ++sourcex){
                KS_SetPixel(dest, left+sourcex, top+sourcey, *(source->pixels + sourcey*source->w + sourcex));
            }
        }
    }
    
    static inline void KS_BlitBlend(ksprite_t* source, ksprite_t* dest, int x, int y){
        int left = x;
        int top = y;
        int left_clip = KS_Max(0, -left);
        int right_clip = KS_Max(0, left + source->w - dest->w);
        int top_clip = KS_Max(0, -top);
        int bottom_clip = KS_Max(0, top + source->h - dest->h);
        for(int sourcey = top_clip; sourcey < source->h - bottom_clip; ++sourcey){
            for(int sourcex = left_clip; sourcex < source->w - right_clip; ++sourcex){
                KS_BlendPixel(dest, left+sourcex, top+sourcey, *(source->pixels + sourcey*source->w + sourcex));
            }
        }
    }
    
    static inline void KS_BlitScaled(ksprite_t* sprite, ksprite_t* target, int x, int y, float scalex, float scaley, int originx, int originy){
        int left = x - originx*scalex;
        int top = y - originy*scaley;
        for(int sy = 0; sy < sprite->h; sy++){
            for(int sx = 0; sx < sprite->w; sx++){
                uint32_t source_pixel = KS_GetPixel(sprite, sx, sy);
                int scaled_sourcex = sx*scalex;
                int scaled_sourcey = sy*scaley;
                for(int oy = KS_Min(0,scaley); oy < KS_Max(scaley,0); oy++){
                    for(int ox = KS_Min(0,scalex); ox < KS_Max(scalex,0); ox++){
                        KS_SetPixel(target, left + scaled_sourcex + ox, top + scaled_sourcey + oy, source_pixel);
                    }
                }
            }
        }
    }
    
    static inline void KS_BlitScaledAlpha10(ksprite_t* sprite, ksprite_t* target, int x, int y, float scalex, float scaley, int originx, int originy){
        int left = x - originx*scalex;
        int top = y - originy*scaley;
        for(int sy = 0; sy < sprite->h; sy++){
            for(int sx = 0; sx < sprite->w; sx++){
                uint32_t source_pixel = KS_GetPixel(sprite, sx, sy);
                int scaled_sourcex = sx*scalex;
                int scaled_sourcey = sy*scaley;
                for(int oy = KS_Min(0,scaley); oy < KS_Max(scaley,0); oy++){
                    for(int ox = KS_Min(0,scalex); ox < KS_Max(scalex,0); ox++){
                        KS_SetPixelAlpha10(target, left + scaled_sourcex + ox, top + scaled_sourcey + oy, source_pixel);
                    }
                }
            }
        }
    }
    
    static inline void KS_BlitScaledBlend(ksprite_t* sprite, ksprite_t* target, int x, int y, float scalex, float scaley, int originx, int originy){
        int left = x - originx*scalex;
        int top = y - originy*scaley;
        for(int sy = 0; sy < sprite->h; sy++){
            for(int sx = 0; sx < sprite->w; sx++){
                uint32_t source_pixel = KS_GetPixel(sprite, sx, sy);
                int scaled_sourcex = sx*scalex;
                int scaled_sourcey = sy*scaley;
                for(int oy = KS_Min(0,scaley); oy < KS_Max(scaley,0); oy++){
                    for(int ox = KS_Min(0,scalex); ox < KS_Max(scalex,0); ox++){
                        KS_BlendPixel(target, left + scaled_sourcex + ox, top + scaled_sourcey + oy, source_pixel);
                    }
                }
            }
        }
    }
    
    static inline void KS_BlitScaledBlendSafe(ksprite_t* sprite, ksprite_t* target, int x, int y, float scalex, float scaley, int originx, int originy){
        int left = x - originx*scalex;
        int top = y - originy*scaley;
        for(int sy = 0; sy < sprite->h; sy++){
            for(int sx = 0; sx < sprite->w; sx++){
                uint32_t source_pixel = KS_GetPixel(sprite, sx, sy);
                int scaled_sourcex = sx*scalex;
                int scaled_sourcey = sy*scaley;
                for(int oy = KS_Min(0,scaley); oy < KS_Max(scaley,0); oy++){
                    for(int ox = KS_Min(0,scalex); ox < KS_Max(scalex,0); ox++){
                        KS_BlendPixelSafe(target, left + scaled_sourcex + ox, top + scaled_sourcey + oy, source_pixel);
                    }
                }
            }
        }
    }
    
    void KS_BlitScaledSafe(ksprite_t* sprite, ksprite_t* target, int x, int y, float scalex, float scaley, int originx, int originy){
        int left = x - originx*scalex;
        int top = y - originy*scaley;
        for(int sy = 0; sy < sprite->h; sy++){
            for(int sx = 0; sx < sprite->w; sx++){
                uint32_t source_pixel = KS_GetPixel(sprite, sx, sy);
                int scaled_sourcex = sx*scalex;
                int scaled_sourcey = sy*scaley;
                for(int oy = KS_Min(0,scaley); oy < KS_Max(scaley,0); oy++){
                    for(int ox = KS_Min(0,scalex); ox < KS_Max(scalex,0); ox++){
                        KS_SetPixelSafe(target, left + scaled_sourcex + ox, top + scaled_sourcey + oy, source_pixel);
                    }
                }
            }
        }
    }
    
    void KS_BlitRotatedOriginBlend(ksprite_t* sprite, ksprite_t* target, int x, int y, float angle, int originx, int originy){
        // Restrict to 90 degree angles
        angle = ((int)(angle/(PI/2.f))%4) * (PI/2.f);
        int s = sinf(angle);
        int c = cosf(angle);
        
        for(int sy = 0; sy < sprite->h; sy++){
            for(int sx = 0; sx < sprite->w; sx++){
                int centerx = sx - originx + 0.5f;
                int centery = sy - originy + 0.5f;
                KS_BlendPixel(target, x + centerx*c - centery*s + originx, y + centerx*s + centery*c + originy, KS_GetPixel(sprite, sx, sy));
            }
        }
    }
    void KS_BlitRotatedBlend(ksprite_t* sprite, ksprite_t* target, int x, int y, float angle){
        KS_BlitRotatedOriginBlend(sprite, target, x, y, angle, sprite->w/2, sprite->h/2);
    }
    
    void KS_BlitRotatedOriginBlendSafe(ksprite_t* sprite, ksprite_t* target, int x, int y, float angle, int originx, int originy){
        // Restrict to 90 degree angles
        angle = ((int)(angle/(PI/2.f))%4) * (PI/2.f);
        int s = sinf(angle);
        int c = cosf(angle);
        
        for(int sy = 0; sy < sprite->h; sy++){
            for(int sx = 0; sx < sprite->w; sx++){
                int centerx = sx - originx + 0.5f;
                int centery = sy - originy + 0.5f;
                KS_BlendPixelSafe(target, x + centerx*c - centery*s + originx, y + centerx*s + centery*c + originy, KS_GetPixel(sprite, sx, sy));
            }
        }
    }
    void KS_BlitRotatedBlendSafe(ksprite_t* sprite, ksprite_t* target, int x, int y, float angle){
        KS_BlitRotatedOriginBlendSafe(sprite, target, x, y, angle, sprite->w/2, sprite->h/2);
    }
    
    void KS_BlitAlpha10(ksprite_t* source, ksprite_t* dest, int x, int y){
        int left = x;
        int top = y;
        int left_clip = KS_Max(0, -left);
        int right_clip = KS_Max(0, left + source->w - dest->w);
        int top_clip = KS_Max(0, -top);
        int bottom_clip = KS_Max(0, top + source->h - dest->h);
        for(int sourcey = top_clip; sourcey < source->h - bottom_clip; ++sourcey){
            for(int sourcex = left_clip; sourcex < source->w - right_clip; ++sourcex){
                KS_SetPixelAlpha10(dest, left+sourcex, top+sourcey, source->pixels[sourcey*source->w + sourcex]);
            }
        }
    }
    
    void KS_BlitAlpha10Safe(ksprite_t* source, ksprite_t* dest, int x, int y){
        int left = x;
        int top = y;
        int left_clip = KS_Max(0, -left);
        int right_clip = KS_Max(0, left + source->w - dest->w);
        int top_clip = KS_Max(0, -top);
        int bottom_clip = KS_Max(0, top + source->h - dest->h);
        for(int sourcey = top_clip; sourcey < source->h - bottom_clip; ++sourcey){
            for(int sourcex = left_clip; sourcex < source->w - right_clip; ++sourcex){
                KS_SetPixelAlpha10Safe(dest, left+sourcex, top+sourcey, source->pixels[sourcey*source->w + sourcex]);
            }
        }
    }
    
    void KS_BlitAlpha10Flip(ksprite_t* source, ksprite_t* dest, int x, int y){
        int left = x;
        int top = y;
        int left_clip = KS_Max(0, -left);
        int right_clip = KS_Max(0, left + source->w - dest->w);
        int top_clip = KS_Max(0, -top);
        int bottom_clip = KS_Max(0, top + source->h - dest->h);
        for(int sourcey = top_clip; sourcey < source->h - bottom_clip; ++sourcey){
            for(int sourcex = left_clip; sourcex < source->w - right_clip; ++sourcex){
                KS_SetPixelAlpha10(dest, left+sourcex, top+sourcey, source->pixels[sourcey*source->w + (source->w-right_clip-sourcex-1)]);
            }
        }
    }
    
    void KS_BlitColored(ksprite_t* source, ksprite_t* dest, int x, int y, int originx, int originy, uint32_t colour){
        int left = x - originx;
        int top = y - originy;
        int left_clip = KS_Max(0, -left);
        int right_clip = KS_Max(0, left + source->w - dest->w);
        int top_clip = KS_Max(0, -top);
        int bottom_clip = KS_Max(0, top + source->h - dest->h);
        for(int sourcey = top_clip; sourcey < source->h - bottom_clip; ++sourcey){
            for(int sourcex = left_clip; sourcex < source->w - right_clip; ++sourcex){
                uint32_t source_pixel = source->pixels[sourcey*source->w + sourcex];
                float bratio = ((uint8_t)source_pixel)/255.f;
                float gratio = ((uint8_t)(source_pixel>>8))/255.f;
                float rratio = ((uint8_t)(source_pixel>>16))/255.f;
                uint8_t a = (uint8_t)(source_pixel>>24);
                KS_SetPixel(dest, left+sourcex, top+sourcey, (uint32_t)(bratio*((uint8_t)colour)) | (uint32_t)(gratio*((uint8_t)(colour>>8)))<<8 | (uint32_t)(rratio*((uint8_t)(colour>>16)))<<16 | a<<24);
            }
        }
    }
    
    void KS_BlitColoredAlpha10(ksprite_t* source, ksprite_t* dest, int x, int y, int originx, int originy, uint32_t colour){
        int left = x - originx;
        int top = y - originy;
        int left_clip = KS_Max(0, -left);
        int right_clip = KS_Max(0, left + source->w - dest->w);
        int top_clip = KS_Max(0, -top);
        int bottom_clip = KS_Max(0, top + source->h - dest->h);
        for(int sourcey = top_clip; sourcey < source->h - bottom_clip; ++sourcey){
            for(int sourcex = left_clip; sourcex < source->w - right_clip; ++sourcex){
                uint32_t source_pixel = source->pixels[sourcey*source->w + sourcex];
                float bratio = ((uint8_t)source_pixel)/255.f;
                float gratio = ((uint8_t)(source_pixel>>8))/255.f;
                float rratio = ((uint8_t)(source_pixel>>16))/255.f;
                uint8_t a = (uint8_t)(source_pixel>>24);
                KS_SetPixelAlpha10(dest, left+sourcex, top+sourcey, (uint32_t)(bratio*((uint8_t)colour)) | (uint32_t)(gratio*((uint8_t)(colour>>8)))<<8 | (uint32_t)(rratio*((uint8_t)(colour>>16)))<<16 | a<<24);
            }
        }
    }
    
    static inline void KS_Clear(ksprite_t* s) {
        memset(s->pixels, 0, sizeof(s->pixels[0]) * s->w * s->h);
    }
    
    void KS_SetAllPixelComponents(ksprite_t* sprite, uint8_t component){
        memset(sprite->pixels, component, sprite->w*sprite->h*4);
    }
    
    void KS_SetAllPixels(ksprite_t* sprite, uint32_t pixel){
        for(int i = 0; i < sprite->h*sprite->w; ++i){
            *(sprite->pixels + i) = pixel;
        }
    }
    
    static inline void KS_ScanLine(ksprite_t* dest, int y, int x0, int x1, uint32_t pixel){
        int left = KS_Min(x0, x1);
        int right = KS_Max(x0, x1);
        left = KS_Max(0, left);
        right = KS_Min(dest->w-1, right);
        for(int x = left; x <= right; ++x){
            KS_SetPixel(dest, x, y, pixel);
        }
    }
    
    static inline void KS_ScanLineSafe(ksprite_t* dest, int y, int x0, int x1, uint32_t pixel){
        if(y < 0 || y > dest->h-1)return;
        int left = KS_Min(x0, x1);
        int right = KS_Max(x0, x1);
        if(right < 0 || left > dest->w-1)return;
        left = KS_Max(0, left);
        right = KS_Min(dest->w-1, right);
        for(int x = left; x <= right; ++x){
            KS_SetPixel(dest, x, y, pixel);
        }
    }
    
    static inline void KS_DrawLineVertical(ksprite_t* dest, int x, int y0, int y1, uint32_t pixel) {
        if(y0 > y1){
            KS_Swap(int, y0, y1);
        }
        for(; y0 <= y1; ++y0){
            KS_SetPixel(dest, x, y0, pixel);
        }
    }
    
    static inline void KS_DrawLineVerticalBlend(ksprite_t* dest, int x, int y0, int y1, uint32_t pixel) {
        if(y0 > y1){
            KS_Swap(int, y0, y1);
        }
        for(; y0 <= y1; ++y0){
            KS_BlendPixel(dest, x, y0, pixel);
        }
    }
    
    static inline void KS_DrawLineVerticalBlendSafe(ksprite_t* dest, int x, int y0, int y1, uint32_t pixel) {
        if(y0 > y1){
            KS_Swap(int, y0, y1);
        }
        for(; y0 <= y1; ++y0){
            KS_BlendPixelSafe(dest, x, y0, pixel);
        }
    }
    
    static inline void KS_DrawLineVerticalSafe(ksprite_t* dest, int x, int y0, int y1, uint32_t pixel) {
        if(y0 > y1){
            KS_Swap(int, y0, y1);
        }
        y0 = Min(dest->h-1, Max(0, y0));
        y1 = Min(dest->h-1, Max(0, y1));
        for(; y0 <= y1; ++y0){
            KS_SetPixel(dest, x, y0, pixel);
        }
    }
    
    void KS_DrawLine(ksprite_t* dest, int x0, int y0, int x1, int y1, uint32_t pixel){
        int dx = KS_Absolute(x1 - x0);
        int dy = KS_Absolute(y1 - y0);
        if(dx == 0){
            KS_DrawLineVertical(dest, x0, y0, y1, pixel);
        }
        int sx = x0<x1 ? 1:-1;
        int sy = y0<y1 ? 1:-1;
        int err = (dx>dy ? dx:-dy) / 2;
        while (KS_SetPixel(dest, x0, y0, pixel), x0 != x1 || y0 != y1) {
            int e2 = err;
            if (e2 > -dx) { err -= dy; x0 += sx; }
            if (e2 <  dy) { err += dx; y0 += sy; }
        }
    }
    
    void KS_DrawLineBlend(ksprite_t* dest, int x0, int y0, int x1, int y1, uint32_t pixel){
        int dx = KS_Absolute(x1 - x0);
        int dy = KS_Absolute(y1 - y0);
        if(dx == 0){
            KS_DrawLineVerticalBlend(dest, x0, y0, y1, pixel);
        }
        int sx = x0<x1 ? 1:-1;
        int sy = y0<y1 ? 1:-1;
        int err = (dx>dy ? dx:-dy) / 2;
        while (KS_BlendPixel(dest, x0, y0, pixel), x0 != x1 || y0 != y1) {
            int e2 = err;
            if (e2 > -dx) { err -= dy; x0 += sx; }
            if (e2 <  dy) { err += dx; y0 += sy; }
        }
    }
    
    void KS_DrawLineBlendSafe(ksprite_t* dest, int x0, int y0, int x1, int y1, uint32_t pixel){
        int dx = KS_Absolute(x1 - x0);
        int dy = KS_Absolute(y1 - y0);
        if(dx == 0){
            KS_DrawLineVerticalBlendSafe(dest, x0, y0, y1, pixel);
        }
        int sx = x0<x1 ? 1:-1;
        int sy = y0<y1 ? 1:-1;
        int err = (dx>dy ? dx:-dy) / 2;
        while (KS_BlendPixelSafe(dest, x0, y0, pixel), x0 != x1 || y0 != y1) {
            int e2 = err;
            if (e2 > -dx) { err -= dy; x0 += sx; }
            if (e2 <  dy) { err += dx; y0 += sy; }
        }
    }
    
    void KS_DrawLineSafe(ksprite_t* dest, int x0, int y0, int x1, int y1, uint32_t pixel){
        int dx = KS_Absolute(x1 - x0);
        int dy = KS_Absolute(y1 - y0);
        if(dx == 0){
            KS_DrawLineVerticalSafe(dest, x0, y0, y1, pixel);
        }
        int sx = x0<x1 ? 1:-1;
        int sy = y0<y1 ? 1:-1;
        int err = (dx>dy ? dx:-dy) / 2;
        while (KS_SetPixelSafe(dest, x0, y0, pixel), x0 != x1 || y0 != y1) {
            int e2 = err;
            if (e2 > -dx) { err -= dy; x0 += sx; }
            if (e2 <  dy) { err += dx; y0 += sy; }
        }
    }
    
    void KS_DrawLinef(ksprite_t* dest, float x0, float y0, float x1, float y1, uint32_t pixel){
        float length = KS_LineLength(x0, y0, x1, y1);
        float dx = (x1-x0)/length;
        float dy = (y1-y0)/length;
        for(float  d = 0; d < length; ++d){
            KS_SetPixel(dest, x0 + d * dx, y0 + d * dy, pixel);
        }
    }
    
    void KS_DrawRect(ksprite_t* dest, int x1, int y1, int x2, int y2, uint32_t pixel){
        int left = KS_Max(0, KS_Min(x1, x2));
        int right = KS_Min(dest->w-1, KS_Max(x1, x2));
        int top = KS_Max(0, KS_Min(y1, y2));
        int bottom = KS_Min(dest->h-1, KS_Max(y1, y2));
        if(left > dest->w-1 || right < 0 || top > dest->h-1 || bottom < 0) return;
        KS_DrawLine(dest, left, top, right, top, pixel);
        KS_DrawLine(dest, left, bottom, right, bottom, pixel);
        KS_DrawLine(dest, left, top, left, bottom, pixel);
        KS_DrawLine(dest, right, top, right, bottom, pixel);
    }
    
    void KS_DrawRectSafe(ksprite_t* dest, int x1, int y1, int x2, int y2, uint32_t pixel){
        int left = KS_Max(0, KS_Min(x1, x2));
        int right = KS_Min(dest->w-1, KS_Max(x1, x2));
        int top = KS_Max(0, KS_Min(y1, y2));
        int bottom = KS_Min(dest->h-1, KS_Max(y1, y2));
        if(left > dest->w-1 || right < 0 || top > dest->h-1 || bottom < 0) return;
        KS_DrawLineSafe(dest, left, top, right, top, pixel);
        KS_DrawLineSafe(dest, left, bottom, right, bottom, pixel);
        KS_DrawLineSafe(dest, left, top, left, bottom, pixel);
        KS_DrawLineSafe(dest, right, top, right, bottom, pixel);
    }
    
    void KS_DrawRectBlend(ksprite_t* dest, int x1, int y1, int x2, int y2, uint32_t pixel){
        int left = KS_Max(0, KS_Min(x1, x2));
        int right = KS_Min(dest->w-1, KS_Max(x1, x2));
        int top = KS_Max(0, KS_Min(y1, y2));
        int bottom = KS_Min(dest->h-1, KS_Max(y1, y2));
        if(left > dest->w-1 || right < 0 || top > dest->h-1 || bottom < 0) return;
        KS_DrawLineBlend(dest, left, top, right, top, pixel);
        KS_DrawLineBlend(dest, left, bottom, right, bottom, pixel);
        KS_DrawLineBlend(dest, left, top, left, bottom, pixel);
        KS_DrawLineBlend(dest, right, top, right, bottom, pixel);
    }
    
    void KS_DrawRectBlendSafe(ksprite_t* dest, int x1, int y1, int x2, int y2, uint32_t pixel){
        int left = KS_Max(0, KS_Min(x1, x2));
        int right = KS_Min(dest->w-1, KS_Max(x1, x2));
        int top = KS_Max(0, KS_Min(y1, y2));
        int bottom = KS_Min(dest->h-1, KS_Max(y1, y2));
        if(left > dest->w-1 || right < 0 || top > dest->h-1 || bottom < 0) return;
        KS_DrawLineBlendSafe(dest, left, top, right, top, pixel);
        KS_DrawLineBlendSafe(dest, left, bottom, right, bottom, pixel);
        KS_DrawLineBlendSafe(dest, left, top, left, bottom, pixel);
        KS_DrawLineBlendSafe(dest, right, top, right, bottom, pixel);
    }
    
    void KS_DrawCircle(ksprite_t* dest, int centerx, int centery, float radius, uint32_t pixel) {
        int x = 0;
        int y = radius;
        float d = 3.f - 2.f*radius;
        while(x <= y) {
            KS_SetPixel(dest, centerx+y, centery+x, pixel);
            KS_SetPixel(dest, centerx+x, centery+y, pixel);
            KS_SetPixel(dest, centerx+x, centery-y, pixel);
            KS_SetPixel(dest, centerx+y, centery-x, pixel);
            KS_SetPixel(dest, centerx-y, centery-x, pixel);
            KS_SetPixel(dest, centerx-x, centery-y, pixel);
            KS_SetPixel(dest, centerx-x, centery+y, pixel);
            KS_SetPixel(dest, centerx-y, centery+x, pixel);
            ++x;
            if(d < 0) {
                d += 6 + 4.f*x;
            }
            else {
                --y;
                d += 4.f*(x-y) + 10;
            }
        }
    }
    
    void KS_DrawCircleSafe(ksprite_t* dest, int centerx, int centery, float radius, uint32_t pixel) {
        int x = 0;
        int y = radius;
        float d = 3.f - 2.f*radius;
        while(x <= y) {
            KS_SetPixelSafe(dest, centerx+y, centery+x, pixel);
            KS_SetPixelSafe(dest, centerx+x, centery+y, pixel);
            KS_SetPixelSafe(dest, centerx+x, centery-y, pixel);
            KS_SetPixelSafe(dest, centerx+y, centery-x, pixel);
            KS_SetPixelSafe(dest, centerx-y, centery-x, pixel);
            KS_SetPixelSafe(dest, centerx-x, centery-y, pixel);
            KS_SetPixelSafe(dest, centerx-x, centery+y, pixel);
            KS_SetPixelSafe(dest, centerx-y, centery+x, pixel);
            ++x;
            if(d < 0) {
                d += 6 + 4.f*x;
            }
            else {
                --y;
                d += 4.f*(x-y) + 10;
            }
        }
    }
    
    void KS_DrawCircleFilledSafe(ksprite_t* dest, int centerx, int centery, float radius, uint32_t pixel) {
        int x = 0;
        int y = radius;
        float d = 3.f - 2.f*radius;
        while(x <= y) {
            KS_ScanLineSafe(dest, centery+x, centerx-y, centerx+y, pixel);
            KS_ScanLineSafe(dest, centery+y, centerx-x, centerx+x, pixel);
            KS_ScanLineSafe(dest, centery-y, centerx-x, centerx+x, pixel);
            KS_ScanLineSafe(dest, centery-x, centerx-y, centerx+y, pixel);
            ++x;
            if(d < 0) {
                d += 6 + 4.f*x;
            }
            else {
                --y;
                d += 4.f*(x-y) + 10;
            }
        }
    }
    
    static inline void KS_ScanLineBlend(ksprite_t* dest, int y, int x0, int x1, uint32_t pixel){
        int left = KS_Min(x0, x1);
        int right = KS_Max(x0, x1);
        if(right < 0 || left > dest->w-1)return;
        for(int x = left; x <= right; ++x){
            KS_BlendPixel(dest, x, y, pixel);
        }
    }
    
    static inline void KS_ScanLineBlendSafe(ksprite_t* dest, int y, int x0, int x1, uint32_t pixel){
        if(y < 0 || y > dest->h-1)return;
        int left = KS_Max(0, KS_Min(x0, x1));
        int right = KS_Min(dest->w-1, KS_Max(x0, x1));
        if(right < 0 || left > dest->w-1)return;
        left = KS_Max(0, left);
        right = KS_Min(dest->w-1, right);
        for(int x = left; x <= right; ++x){
            KS_BlendPixel(dest, x, y, pixel);
        }
    }
    
    static inline void KS_DrawRectFilled(ksprite_t* dest, int x1, int y1, int x2, int y2, uint32_t pixel){
        int left = KS_Max(0, KS_Min(x1, x2));
        int right = KS_Min(dest->w-1, KS_Max(x1, x2));
        int top = KS_Max(0, KS_Min(y1, y2));
        int bottom = KS_Min(dest->h-1, KS_Max(y1, y2));
        if(left > dest->w-1 || right < 0 || top > dest->h-1 || bottom < 0) return;
        for(int y = top; y < bottom+1; ++y){
            KS_ScanLine(dest, y, left, right, pixel);
        }
    }
    
    static inline void KS_DrawRectFilledSafe(ksprite_t* dest, int x1, int y1, int x2, int y2, uint32_t pixel){
        int left = KS_Max(0, KS_Min(x1, x2));
        int right = KS_Min(dest->w-1, KS_Max(x1, x2));
        int top = KS_Max(0, KS_Min(y1, y2));
        int bottom = KS_Min(dest->h-1, KS_Max(y1, y2));
        if(left > dest->w-1 || right < 0 || top > dest->h-1 || bottom < 0) return;
        for(int y = top; y <= bottom; ++y){
            KS_ScanLineSafe(dest, y, left, right, pixel);
        }
    }
    
    static inline void KS_DrawRectFilledBlend(ksprite_t* dest, int x1, int y1, int x2, int y2, uint32_t pixel){
        int left = KS_Max(0, KS_Min(x1, x2));
        int right = KS_Min(dest->w-1, KS_Max(x1, x2));
        int top = KS_Max(0, KS_Min(y1, y2));
        int bottom = KS_Min(dest->h-1, KS_Max(y1, y2));
        if(left > dest->w-1 || right < 0 || top > dest->h-1 || bottom < 0) return;
        for(int y = top; y < bottom+1; ++y){
            KS_ScanLineBlend(dest, y, left, right, pixel);
        }
    }
    
    static inline void KS_DrawRectFilledBlendSafe(ksprite_t* dest, int x1, int y1, int x2, int y2, uint32_t pixel){
        int left = KS_Max(0, KS_Min(x1, x2));
        int right = KS_Min(dest->w-1, KS_Max(x1, x2));
        int top = KS_Max(0, KS_Min(y1, y2));
        int bottom = KS_Min(dest->h-1, KS_Max(y1, y2));
        if(left > dest->w-1 || right < 0 || top > dest->h-1 || bottom < 0) return;
        for(int y = top; y < bottom+1; ++y){
            KS_ScanLineBlendSafe(dest, y, left, right, pixel);
        }
    }
    
    /*void KS_DrawTriangleFlatBottom(ksprite_t* dest, float x, float y, float left, float right, float bottom, uint32_t pixel){
        float dl = left-x;
        float dr = right-x;
        float height = bottom-y;
        for(float liney = y; liney <= bottom; ++liney){
            float height_ratio = (liney-y)/height;
            KS_ScanLine(dest, liney, x+dl*height_ratio, x+dr*height_ratio, pixel);
        }
    }
    
    void KS_DrawTriangleFlatTop(ksprite_t* dest, float x, float y, float left, float right, float top, uint32_t pixel){
        float dl = left-x;
        float dr = right-x;
        float height = top-y;
        for(float liney = top; liney <= y; ++liney){
            float height_ratio = (liney-y)/height;
            KS_ScanLine(dest, liney, x+dl*height_ratio, x+dr*height_ratio, pixel);
        }
    }
    
    void KS_DrawTriangle(ksprite_t* dest, float x0, float y0, float x1, float y1, float x2, float y2, uint32_t pixel){
        // Sort vertices vertically so v0y <= v1y <= v2y
        if(y0 > y1){
            float xt = x0, yt = y0;
            x0 = x1;y0 = y1;
            x1 = xt;y1 = yt;
        }
        if(y1 > y2){
            float xt = x1, yt = y1;
            x1 = x2;y1 = y2;
            x2 = xt;y2 = yt;
        }
        if(y0 > y1){
            float xt = x0, yt = y0;
            x0 = x1;y0 = y1;
            x1 = xt;y1 = yt;
        }
        if(y1 == y2){
            KS_DrawTriangleFlatBottom(dest, x0, y0, x1, x2, y1, pixel);
        }
        else if(y0 == y1){
            KS_DrawTriangleFlatTop(dest, x2, y2, x0, x1, y0, pixel);
        }
        else{
            float x3 = (x0 + ((y1 - y0) / (y2 - y0)) * (x2 - x0));
            KS_DrawTriangleFlatBottom(dest, x0, y0, x1, x3, y1, pixel);
            KS_DrawTriangleFlatTop(dest, x2, y2, x1, x3, y1+1, pixel);
            KS_DrawLinef(dest, x1, y1, x3, y1, pixel);
        }
        KS_DrawLinef(dest, x0, y0, x1, y1, pixel);
        KS_DrawLinef(dest, x0, y0, x2, y2, pixel);
        KS_DrawLinef(dest, x2, y2, x1, y1, pixel);
    }*/
    
    void KS_DrawTriangle(ksprite_t* dest, float x0, float y0, float x1, float y1, float x2, float y2, uint32_t pixel){
        // Sort vertices vertically so v0y <= v1y <= v2y
        if(y0 > y1){
            KS_Swap(float, x0, x1);
            KS_Swap(float, y0, y1);
        }
        if(y1 > y2){
            KS_Swap(float, x1, x2);
            KS_Swap(float, y1, y2);
        }
        if(y0 > y1){
            KS_Swap(float, x0, x1);
            KS_Swap(float, y0, y1);
        }
        if(y1>y0){
            for(int y = y0; y <= y1; ++y){
                float x01 = x0+(x1-x0)*((KS_Min(KS_Max((float)y,y0),y1)-y0)/(y1-y0));
                float x02 = x0+(x2-x0)*((KS_Min(KS_Max((float)y,y0),y1)-y0)/(y2-y0));
                KS_ScanLine(dest, y, x01, x02, pixel);
            }
        }
        if(y2>y1){
            for(int y = y1; y <= y2; ++y){
                float x02 = x0+(x2-x0)*((KS_Min(KS_Max((float)y,y1),y2)-y0)/(y2-y0));
                float x12 = x1+(x2-x1)*((KS_Min(KS_Max((float)y,y1),y2)-y1)/(y2-y1));
                KS_ScanLine(dest, y, x02, x12, pixel);
            }
        }
    }
    
    void KS_ToGreyScale(ksprite_t* sprite) {
        uint8_t *pixel_it = (uint8_t*)sprite->pixels;
        uint8_t* last_pixel = pixel_it + sprite->w*sprite->h*4;
        uint32_t r, g, b, grey;
        while(pixel_it < last_pixel) {
            b = *pixel_it;
            g = *(pixel_it+1);
            r = *(pixel_it+2);
            grey = (b+g+r)/3;
            *pixel_it = *(pixel_it+1) = *(pixel_it+2) = grey;
            pixel_it += 4;
        }
    }
    
    typedef struct{
        int w, h;
        uint8_t* pixels;
    } KMask;
    
    void KMCreate(KMask* mask, int w, int h, uint8_t pixel){
        mask->w = w;
        mask->h = h;
        mask->pixels = (uint8_t*)malloc(w*h);
        memset(mask->pixels, pixel, w*h);
    }
    
    void KMClear(KMask* mask){
        memset(mask->pixels, 0xff, mask->w*mask->h);
    }
    
    void KMSetPixel(KMask* mask, int x, int y, uint8_t pixel){
        if(x < 0 || x > mask->w-1 || y < 0 || y > mask->h-1) return;
        *(mask->pixels + y*mask->w + x) = pixel;
    }
    
    void KS_BlitMasked(ksprite_t* source, ksprite_t* dest, KMask* mask, int spritex, int spritey, int maskx, int masky, void(*PixelFunc)(ksprite_t*, int, int, uint32_t)){
        int left_clip = KS_Max(0, KS_Max(-spritex, -maskx));
        int right_clip = KS_Max(0, KS_Max(spritex+source->w-dest->w, maskx+mask->w-dest->w));
        int top_clip = KS_Max(0, KS_Max(-spritey, -masky));
        int bottom_clip = KS_Max(0, KS_Max(spritey+source->h-dest->h, masky+mask->h-dest->h));
        for(int sourcey = top_clip; sourcey < source->h - bottom_clip; ++sourcey){
            for(int sourcex = left_clip; sourcex < source->w - right_clip; ++sourcex){
                uint32_t pixel = KS_GetPixel(source, sourcex, sourcey);
                pixel = ((pixel<<8)>>8) + (mask->pixels[sourcey*mask->w+sourcex]<<24);
                PixelFunc(dest, spritex+sourcex, spritey+sourcey, pixel);
            }
        }
    }
    
    void KS_BlitMask(KMask* mask, ksprite_t* dest, int destx, int desty, void(*PixelFunc)(ksprite_t*, int, int, uint32_t)){
        int left = KS_Max(0, destx);
        int right = KS_Min(dest->w-1, destx + mask->w-1);
        int top = KS_Max(0, desty);
        int bottom = KS_Min(dest->h-1, desty + mask->h-1);
        for(int y = top; y < bottom; ++y){
            for(int x = left; x < right; ++x){
                uint32_t pixel  = mask->pixels[(y-top)*mask->w + (x-left)];
                PixelFunc(dest, x, y, pixel + (pixel<<8) + (pixel<<16) + (0xff<<24));
            }
        }
    }
    
    void KMScanLine(KMask* dest, int y, int x0, int x1, uint8_t pixel){
        if(y < 0 || y > dest->h-1)return;
        int left = KS_Max(0, KS_Min(dest->w-1, KS_Min(x0, x1)));
        int right = KS_Max(0, KS_Min(dest->w-1, KS_Max(x0, x1)));
        memset(dest->pixels+(y*dest->w+left), pixel, right-left);
    }
    
    void KMDrawLinef(KMask* dest, float x0, float y0, float x1, float y1, uint8_t pixel){
        float length = KS_LineLength(x0, y0, x1, y1);
        float dx = (x1-x0)/length;
        float dy = (y1-y0)/length;
        for(int d = 0; d < length; ++d){
            KMSetPixel(dest, x0 + (float)d * dx, y0 + (float)d * dy, pixel);
        }
    }
    
    void KMDrawTriangleFlatBottom(KMask* dest, float x, float y, float left, float right, float bottom, uint8_t pixel){
        float dl = left-x;
        float dr = right-x;
        float height = bottom-y;
        for(float liney = y; liney <= bottom; ++liney){
            float height_ratio = (liney-y)/height;
            KMScanLine(dest, liney, x+dl*height_ratio, x+dr*height_ratio, pixel);
        }
    }
    
    void KMDrawTriangleFlatTop(KMask* dest, float x, float y, float left, float right, float top, uint8_t pixel){
        float dl = left-x;
        float dr = right-x;
        float height = top-y;
        for(float liney = top; liney <= y; ++liney){
            float height_ratio = (liney-y)/height;
            KMScanLine(dest, liney, x+dl*height_ratio, x+dr*height_ratio, pixel);
        }
    }
    
    
    void KMDrawTriangle(KMask* dest, float x0, float y0, float x1, float y1, float x2, float y2, uint8_t pixel){
        // Sort vertices vertically so v0y <= v1y <= v2y
        if(y0 > y1){
            float xt = x0, yt = y0;
            x0 = x1; y0 = y1;
            x1 = xt; y1 = yt;
        }
        if(y1 > y2){
            float xt = x1, yt = y1;
            x1 = x2; y1 = y2;
            x2 = xt; y2 = yt;
        }
        if(y0 > y1){
            float xt = x0, yt = y0;
            x0 = x1; y0 = y1;
            x1 = xt; y1 = yt;
        }
        if(y1>y0){
            for(int y = y0; y <= y1; ++y){
                float x01 = x0+(x1-x0)*((KS_Min(KS_Max((float)y,y0),y1)-y0)/(y1-y0));
                float x02 = x0+(x2-x0)*((KS_Min(KS_Max((float)y,y0),y1)-y0)/(y2-y0));
                KMScanLine(dest, y, KS_Min(x01,x02), KS_Max(x01,x02), pixel);
            }
        }
        if(y2>y1){
            for(int y = y1; y <= y2; ++y){
                float x02 = x0+(x2-x0)*((KS_Min(KS_Max((float)y,y1),y2)-y0)/(y2-y0));
                float x12 = x1+(x2-x1)*((KS_Min(KS_Max((float)y,y1),y2)-y1)/(y2-y1));
                KMScanLine(dest, y, KS_Min(x02,x12), KS_Max(x02,x12), pixel);
            }
        }
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_SPRITE_H
#endif
