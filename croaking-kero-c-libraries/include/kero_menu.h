#ifndef KERO_MENU_H

#include "kero_sprite.h"
#include <stdint.h>

#define KERO_MENU_MAX_ITEMS 32

typedef struct {
    int selected;
    unsigned int h; // Height for one item
    unsigned int count;
    unsigned int max_len;
    char name[KERO_MENU_MAX_ITEMS][32];
    unsigned int name_len[KERO_MENU_MAX_ITEMS];
    void (*func[KERO_MENU_MAX_ITEMS])();
} kmenu_t;

static inline int KMenuItemFromY(kmenu_t *menu, int y) {
    y += 5;
    if(y > -1 && y < menu->h*menu->count) {
        return y / menu->h;
    }
    return -1;
}

static inline void KMenuCalculateStringLengths(kmenu_t *menu) {
    menu->max_len = 0;
    int len = 0;
    for(int i = 0; i < menu->count; ++i) {
        menu->name_len[i] = strlen(menu->name[i]);
        if(menu->name_len[i] > menu->max_len) menu->max_len = menu->name_len[i];
    }
}

void KMenuDraw(kmenu_t *menu, ksprite_t *frame_buffer, kfont_t *font, int centerx, int centery, uint32_t color) {
    int charw = font->characters[0].w;
    int boxh = font->characters[0].h;
    int charh2 = boxh*2;
    int top = centery - menu->count/2*charh2;
    int maxwidth = menu->max_len*charw+10;
    int halfmaxwidth = maxwidth/2;
    int selected_top = menu->selected*charh2;
    boxh += 5;
    KSDrawRectFilled(frame_buffer, centerx-halfmaxwidth, top+selected_top-5, centerx+halfmaxwidth, top+selected_top+boxh, color);
    for(int i = 0; i < menu->count; ++i) {
        KFDrawAlpha10(font, frame_buffer, centerx - menu->name_len[i]*charw/2, top+i*charh2, menu->name[i]);
    }
}

#define KERO_MENU_H
#endif