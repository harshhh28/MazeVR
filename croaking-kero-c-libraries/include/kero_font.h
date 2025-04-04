#ifndef FONT_H
#define FONT_H

#ifdef __cplusplus
extern "C"{
#endif
    
#include "kero_sprite.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
    
    typedef struct{
        ksprite_t characters[256];
    } kfont_t;
    
    bool KF_Load(kfont_t* font, char* file_path){
        ksprite_t font_spr;
        if(!KS_Load(&font_spr, file_path)) {
            fprintf(stderr, "Failed to load font: %s\n", file_path);
            return false;
        }
        int w16th = font_spr.w / 16;
        int h16th = font_spr.h / 16;
        for(int y = 0; y < 16; y++){
            for(int x = 0; x < 16; x++){
                KS_CreateFromSprite(&font->characters[y*16+x], &font_spr, x*w16th, y*h16th, (x+1)*w16th, (y+1)*h16th);
            }
        }
        return true;
    }
    
    void KF_Draw(kfont_t* font, ksprite_t* target, int x, int y, char* text){
        int xoffset = 0;
        for(int unsigned i = 0; text[i] != '\0'; i++){
            if(text[i] == '\n'){
                y+=16;
                xoffset = -(i+1)*16;
            } else{
                KS_Blit(&font->characters[text[i]], target, x+i*16+xoffset, y);
            }
        }
    }
    
    void KF_DrawAlpha10(kfont_t* font, ksprite_t* target, int x, int y, char* text){
        int xoffset = 0;
        for(int unsigned i = 0; text[i] != '\0'; i++){
            if(text[i] == '\n'){
                y+=16;
                xoffset = -(i+1)*16;
            } else{
                KS_BlitAlpha10(&font->characters[text[i]], target, x+i*16+xoffset, y);
            }
        }
    }
    
    void KF_DrawBlend(kfont_t* font, ksprite_t* target, int x, int y, char* text){
        int xoffset = 0;
        for(int unsigned i = 0; text[i] != '\0'; i++){
            if(text[i] == '\n'){
                y+=16;
                xoffset = -(i+1)*16;
            } else{
                KS_BlitBlend(&font->characters[text[i]], target, x+i*16+xoffset, y);
            }
        }
    }
    
    void KF_DrawColored(kfont_t* font, ksprite_t* target, int x, int y, char* text, uint32_t colour){
        int xoffset = 0;
        for(int unsigned i = 0; text[i] != '\0'; i++){
            if(text[i] == '\n'){
                y+=16;
                xoffset = -(i+1)*16;
            } else{
                KS_BlitColoredAlpha10(&font->characters[text[i]], target, x+i*16+xoffset, y, 0, 0, colour);
            }
        }
    }
    
#ifdef __cplusplus
}
#endif

#endif