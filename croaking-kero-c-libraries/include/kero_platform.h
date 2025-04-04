/*
Kero Platform is designed to be used for single-window applications and has a simple API design in mind. Currently only supports software rendering but it is planned to add OpenGL contexts in the future.

Kero Platform gives you window events, a keyboard state, a software rendering context and software framerate limiting.
*/

#ifndef KERO_PLATFORM_H

#ifdef __cplusplus
extern "C"{
#endif
    
    //------------------------------------------------------------
    
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
    
#if __linux__
    
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <stdlib.h>
#include <string.h>
#include <X11/XKBlib.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
    typedef struct timespec timespec;
#else
#include <SDL2/SDL.h>
#endif
    
    typedef struct{
        unsigned int w, h;
        uint32_t* pixels;
    } kp_frame_buffer_t;
    
    typedef struct {
        struct{
            unsigned int w, h;
        } window;
        kp_frame_buffer_t frame_buffer;
        float delta;
        bool reset_keyboard_on_focus_out;
        bool fullscreen;
        unsigned int windowed_width, windowed_height;
        int windowed_x, windowed_y;
        struct{
            int x, y;
            bool invertx, inverty;
            uint32_t buttons;
        } mouse;
        unsigned long target_frame_time; // Linux: In nano seconds. Other platforms: In milliseconds.
#if __linux__
        Display* display;
        unsigned long root_window;
        int screen;
        XVisualInfo visual_info;
        XSetWindowAttributes window_attributes;
        Window xwindow;
        XImage* ximage;
        Atom WM_DELETE_WINDOW;
        GC graphics_context;
        timespec frame_start;
        timespec frame_finish;
        bool keyboard[256];
        Atom _NET_WM_STATE_ATOM;
#else
        SDL_Window* sdlwindow;
        SDL_Surface* canvas;
        uint32_t frame_start;
        uint32_t frame_finish;
        const uint8_t* keyboard;
#endif
    } kero_platform_t;
    
    typedef enum {
        KP_EVENT_KEY_PRESS, KP_EVENT_KEY_RELEASE, KP_EVENT_QUIT, KP_EVENT_RESIZE, KP_EVENT_FOCUS_OUT, KP_EVENT_FOCUS_IN, KP_EVENT_MOUSE_BUTTON_PRESS, KP_EVENT_MOUSE_BUTTON_RELEASE, KP_EVENT_NONE
    } kp_event_type_t;
    typedef struct {
        kp_event_type_t type;
        union {
            uint8_t key;
            struct {
                uint8_t button;
                uint16_t x, y;
            };
            struct {
                uint16_t width, height;
            };
        };
    } kp_event_t;
    
    //------------------------------------------------------------
    
    /*
     Usage
     
    Include this file. Currently this single header contains the entire Kero_Platform library. On Linux link against X11 (-lX11). On Windows/Mac link against SDL2 (-lSDL2)
    */
    
    
    
    void KP_Init(kero_platform_t *platform, const unsigned int width, const unsigned int height, const char* const title);
    /*
Initialize Kero Platform
width and height are the internal size of the frame, not the total size of the window. The actual frame may be smaller if it cannot fit on the screen.
    This sets a target framerate of 60fps and disables key repeat.
    */
    
    void KP_Flip(kero_platform_t *platform);
    /*
    Send frame buffer to screen.
    Sleep until time since last flip = target frame time (1/fps).
    Sets kp_delta variable to the number of seconds from the end of the last frame to the end of this one, including sleep time. DO NOT MANUALLY CHANGE THE VALUE OF kp_delta!
    */
    
    int KP_EventsQueued(kero_platform_t *platform);
    /*
    Returns number of events queued.
    Updates mouse position and button state.
    */
    
    kp_event_t* KP_NextEvent(kero_platform_t *platform);
    /*
    Returns pointer to next event in queue and removes that event from the queue.
    The event may have type KP_EVENT_NONE in which case it should be ignored.
    */
    
    void KP_FreeEvent(kp_event_t* e);
    /*
    Frees the event. Should be called for every event retrieved by KP_NextEvent to avoid a memory leak.
    */
    
    /*
    Example event handling code:
    
    while(KP_EventsQueued()) {
        kpEvent* e = KP_NextEvent();
        switch(e->type) {
            case KP_EVENT_KEY_PRESS:{
                switch(e->key) {
                    case KEY_ESCAPE:{
                        game_running = false;
                    }break;
                }
            }break;
            case KP_EVENT_QUIT:{
                game_running = false;
            }break;
        }
        KP_FreeEvent(e);
    }
    
    See enum kp_event_type_t for all event types.
    */
    
    void KP_SetTargetFramerate(kero_platform_t *platform, const unsigned int fps);
    /*
    Target number of frames per second. When calling KP_Flip() the process will sleep until 1/fps seconds have elapsed since the last flip.
    Call this with fps of 0 to disable frame limit.
    */
    
    void KP_UpdateMouse(kero_platform_t *platform);
    /*
    Updates the mouse position and buttons.
    Mouse x/y are kp_mouse.x/y
    Buttons are kp_mouse.buttons & MOUSE_LEFT/MOUSE_RIGHT/MOUSE_OTHER
    */
    
    void KP_SetWindowTitle(kero_platform_t *platform, const char* const title);
    /*
     Very slow on Linux. Don't call every frame.
    */
    
    /*
    The keyboard is stored in a boolean array called kp_keyboard. Updated inside KP_NextEvent() whenever a keyboard event is received.
Key repeats (holding key down) are ignored automatically.
    See Key defines for all the keys/
    
    if(kp_keyboard[KEY_ESCAPE]) {
        game_running = false;
    }
    */
    
    void KP_ShowCursor(kero_platform_t *platform, const bool show);
    /*
    True = draw the cursor over the window
    False = hide cursor
    */
    
    double KP_Clock();
    /*
    Returns current wall-clock time in milliseconds.
    */
    
    void KP_SetCursorPos(kero_platform_t *platform, int x, int y, int* dx, int* dy);
    /*
    Sets mouse cursor to x,y which are offsets from the top-left corner of the window.
    dx and dy are set to the distance the cursor was moved.
    dx/dy can be NULL.
    */
    
    void KP_Fullscreen(kero_platform_t *platform, bool full);
    /*
    True makes the window fullscreen.
    False returns to a windowed view.
    */
    
    void KP_Sleep(unsigned long nanoseconds);
    /*
    Pause thread for that many nanoseconds.
    */
    
    void KP_OpenURL(const char* const url);
    /*
    Open a URL in the default web browser.
    */
    
    void KP_CreateDirectory(const char* const path);
    /*
    Create a folder. Use this before file operations to make sure the folder exists.
    */
    
    char* KP_GetUserHomeDirectory();
    /*
    Gets the user's home directory, WITHOUT a slash at the end. Returns NULL if it can't be found.
    */
    
    //------------------------------------------------------------
    
#define KEY_ENTER KEY_RETURN
    
#if __linux__
    // Mouse buttons
#define MOUSE_LEFT (0b1)
#define MOUSE_RIGHT (0b10)
#define MOUSE_MIDDLE (0b100)
#define MOUSE_OTHER (0b1000)
    
    // Key consts
#define KEY_UP ((uint8_t)XK_Up)
#define KEY_DOWN ((uint8_t)XK_Down)
#define KEY_LEFT ((uint8_t)XK_Left)
#define KEY_RIGHT ((uint8_t)XK_Right)
#define KEY_SPACE ((uint8_t)XK_space)
#define KEY_RETURN ((uint8_t)XK_Return)
#define KEY_ESCAPE ((uint8_t)XK_Escape)
#define KEY_LSHIFT ((uint8_t)XK_Shift_L)
#define KEY_RSHIFT ((uint8_t)XK_Shift_R)
#define KEY_LALT ((uint8_t)XK_Alt_L)
#define KEY_RALT ((uint8_t)XK_Alt_R)
#define KEY_LCTRL ((uint8_t)XK_Control_L)
#define KEY_RCTRL ((uint8_t)XK_Control_R)
#define KEY_Q ((uint8_t)XK_q)
#define KEY_W ((uint8_t)XK_w)
#define KEY_E ((uint8_t)XK_e)
#define KEY_R ((uint8_t)XK_r)
#define KEY_T ((uint8_t)XK_t)
#define KEY_Y ((uint8_t)XK_y)
#define KEY_U ((uint8_t)XK_u)
#define KEY_I ((uint8_t)XK_i)
#define KEY_O ((uint8_t)XK_o)
#define KEY_P ((uint8_t)XK_p)
#define KEY_A ((uint8_t)XK_a)
#define KEY_S ((uint8_t)XK_s)
#define KEY_D ((uint8_t)XK_d)
#define KEY_F ((uint8_t)XK_f)
#define KEY_G ((uint8_t)XK_g)
#define KEY_H ((uint8_t)XK_h)
#define KEY_J ((uint8_t)XK_j)
#define KEY_K ((uint8_t)XK_k)
#define KEY_L ((uint8_t)XK_l)
#define KEY_Z ((uint8_t)XK_z)
#define KEY_X ((uint8_t)XK_x)
#define KEY_C ((uint8_t)XK_c)
#define KEY_V ((uint8_t)XK_v)
#define KEY_B ((uint8_t)XK_b)
#define KEY_N ((uint8_t)XK_n)
#define KEY_M ((uint8_t)XK_m)
#define KEY_EQUAL ((uint8_t)XK_equal)
#define KEY_MINUS ((uint8_t)XK_minus)
#define KEY_0 ((uint8_t)XK_0)
#define KEY_1 ((uint8_t)XK_1)
#define KEY_2 ((uint8_t)XK_2)
#define KEY_3 ((uint8_t)XK_3)
#define KEY_4 ((uint8_t)XK_4)
#define KEY_5 ((uint8_t)XK_5)
#define KEY_6 ((uint8_t)XK_6)
#define KEY_7 ((uint8_t)XK_7)
#define KEY_8 ((uint8_t)XK_8)
#define KEY_9 ((uint8_t)XK_9)
    
#define KEY_F1 ((uint8_t)XK_F1)
#define KEY_F2 ((uint8_t)XK_F2)
#define KEY_F3 ((uint8_t)XK_F3)
#define KEY_F4 ((uint8_t)XK_F4)
#define KEY_F5 ((uint8_t)XK_F5)
#define KEY_F6 ((uint8_t)XK_F6)
#define KEY_F7 ((uint8_t)XK_F7)
#define KEY_F8 ((uint8_t)XK_F8)
#define KEY_F9 ((uint8_t)XK_F9)
#define KEY_F10 ((uint8_t)XK_F10)
#define KEY_F11 ((uint8_t)XK_F11)
#define KEY_F12 ((uint8_t)XK_F12)
    
#define _NET_WM_STATE_TOGGLE    2
    
    // KP_SetWindowTitle on Linux is very slow and inconsistent. Don't call every frame.
    void KP_SetWindowTitle(kero_platform_t *platform, const char* const title) {
        XStoreName(platform->display, platform->xwindow, title);
    }
    
    // fps = 0 to disable frame limiting
    void KP_SetTargetFramerate(kero_platform_t *platform, const unsigned int fps) {
        if(fps) {
            platform->target_frame_time = 1000000000/fps;
        }
        else{
            platform->target_frame_time = 0;
        }
    }
    
    void KP_Init(kero_platform_t *platform, const unsigned int width, const unsigned int height, const char* const title) {
        platform->delta = 0;
        platform->reset_keyboard_on_focus_out = true;
        platform->fullscreen = false;
        platform->windowed_width = 0;
        platform->windowed_height = 0;
        platform->windowed_x = 0;
        platform->windowed_y = 0;
        platform->target_frame_time = 0;
        platform->windowed_width = width;
        platform->windowed_height = height;
        platform->window.w = width;
        platform->window.h = height;
        platform->display = XOpenDisplay(0);
        platform->root_window = XDefaultRootWindow(platform->display);
        platform->screen = XDefaultScreen(platform->display);
        XMatchVisualInfo(platform->display, platform->screen, 24, TrueColor, &platform->visual_info);
        platform->window_attributes.background_pixel = 0;
        platform->window_attributes.colormap = XCreateColormap(platform->display, platform->root_window, platform->visual_info.visual, AllocNone);
        platform->window_attributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | ButtonPressMask | ButtonReleaseMask;
        platform->xwindow = XCreateWindow(platform->display, platform->root_window, 0, 0, platform->window.w, platform->window.h, 0, platform->visual_info.depth, 0, platform->visual_info.visual, CWBackPixel | CWColormap | CWEventMask, &platform->window_attributes);
        XMapWindow(platform->display, platform->xwindow);
        XFlush(platform->display);
        platform->WM_DELETE_WINDOW = XInternAtom(platform->display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(platform->display, platform->xwindow, &platform->WM_DELETE_WINDOW, 1);
        XkbSetDetectableAutoRepeat(platform->display, True, 0);
        KP_SetWindowTitle(platform, title);
        platform->frame_buffer.pixels = (uint32_t*)malloc(sizeof(uint32_t)*platform->window.w*platform->window.h);
        platform->frame_buffer.w = platform->window.w;
        platform->frame_buffer.h = platform->window.h;
        platform->ximage = XCreateImage(platform->display, platform->visual_info.visual, platform->visual_info.depth, ZPixmap, 0, (char*)platform->frame_buffer.pixels, platform->window.w, platform->window.h, 32, 0);
        platform->graphics_context = DefaultGC(platform->display, platform->screen);
        KP_SetTargetFramerate(platform, 60);
        platform->_NET_WM_STATE_ATOM = XInternAtom(platform->display, "_NET_WM_STATE", False);
        clock_gettime(CLOCK_REALTIME, &platform->frame_start);
    }
    
#ifdef KERO_PLATFORM_GL
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>
    static GLXContext gl_context;
    static GLint gl_attributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    static GLXDrawable gl_drawable;
    
    void KPGL_Init(kero_platform_t *platform, const unsigned int width, const unsigned int height,
                   const char* const title) {
        platform->windowed_width = width;
        platform->windowed_height = height;
        platform->window.w = width;
        platform->window.h = height;
        platform->display = XOpenDisplay(0);
        platform->root_window = XDefaultRootWindow(platform->display);
        platform->screen = XDefaultScreen(platform->display);
        platform->visual_info = *glXChooseVisual(platform->display, 0, gl_attributes);
        platform->window_attributes.colormap = XCreateColormap(platform->display, platform->root_window, platform->visual_info.visual, AllocNone);
        platform->window_attributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | ButtonPressMask | ButtonReleaseMask;
        platform->xwindow = XCreateWindow(platform->display, platform->root_window, 0, 0, platform->window.w, platform->window.h, 0, platform->visual_info.depth, 0, platform->visual_info.visual, CWColormap | CWEventMask, &platform->window_attributes);
        XMapWindow(platform->display, platform->xwindow);
        XFlush(platform->display);
        platform->WM_DELETE_WINDOW = XInternAtom(platform->display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(platform->display, platform->xwindow, &platform->WM_DELETE_WINDOW, 1);
        XkbSetDetectableAutoRepeat(platform->display, True, 0);
        KP_SetWindowTitle(platform, title);
        gl_context = glXCreateContext(platform->display, &platform->visual_info, NULL, GL_TRUE);
        glXMakeCurrent(platform->display, platform->xwindow, gl_context);
        //printf("%s\n", glXQueryExtensionsString(platform->display, platform->screen));
        gl_drawable = glXGetCurrentDrawable();
        if(!gl_drawable) {
            fprintf(stderr, "Failed to get glX drawable.\n");
            exit(-1);
        }
        unsigned int swap, maxSwap;
        
        glXQueryDrawable(platform->display, gl_drawable, GLX_SWAP_INTERVAL_EXT, &swap);
        glXQueryDrawable(platform->display, gl_drawable, GLX_MAX_SWAP_INTERVAL_EXT,
                         &maxSwap);
        printf("The swap interval is %u and the max swap interval is "
               "%u\n", swap, maxSwap);
        
        glXSwapIntervalEXT(platform->display, gl_drawable, 1);
        KP_SetTargetFramerate(platform, 60);
        platform->_NET_WM_STATE_ATOM = XInternAtom(platform->display, "_NET_WM_STATE", False);
        clock_gettime(CLOCK_REALTIME, &platform->frame_start);
    }
    
    void KPGL_Flip(kero_platform_t *platform) {
        glXSwapBuffers(platform->display, platform->xwindow);
        //glFinish();
        clock_gettime(CLOCK_REALTIME, &platform->frame_finish);
        platform->delta = (platform->frame_finish.tv_sec - platform->frame_start.tv_sec) +(platform->frame_finish.tv_nsec - platform->frame_start.tv_nsec)/1000000000.f;
        platform->frame_start = platform->frame_finish;
    }
#endif // KERO_PLATFORM_GL
    
    void KP_Sleep(unsigned long nanoseconds) {
        timespec sleep_time = { 0, nanoseconds };
        nanosleep(&sleep_time, 0);
    }
    
    void KP_Flip(kero_platform_t *platform) {
        XPutImage(platform->display, platform->xwindow, platform->graphics_context, platform->ximage, 0, 0, 0, 0, platform->window.w, platform->window.h);
        if(platform->target_frame_time) {
            // Delay until we take up the full frame time
            clock_gettime(CLOCK_REALTIME, &platform->frame_finish);
            long unsigned frame_time = (platform->frame_finish.tv_sec - platform->frame_start.tv_sec)*1000000000 + (platform->frame_finish.tv_nsec - platform->frame_start.tv_nsec);
            timespec sleep_time;
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = platform->target_frame_time - frame_time;
            nanosleep(&sleep_time, 0);
        }
        clock_gettime(CLOCK_REALTIME, &platform->frame_finish);
        platform->delta = (platform->frame_finish.tv_sec - platform->frame_start.tv_sec) +(platform->frame_finish.tv_nsec - platform->frame_start.tv_nsec)/1000000000.f;
        platform->frame_start = platform->frame_finish;
    }
    
    void KP_UpdateMouse(kero_platform_t *platform) {
        Window window_returned;
        int display_x, display_y;
        unsigned int mask_return;
        if(XQueryPointer(platform->display, platform->xwindow, &window_returned,
                         &window_returned, &display_x, &display_y, &platform->mouse.x, &platform->mouse.y, 
                         &mask_return) == True) {
        }
        if(platform->mouse.invertx) {
            platform->mouse.x = platform->window.w - platform->mouse.x;
        }
        if(platform->mouse.inverty) {
            platform->mouse.y = platform->window.h - platform->mouse.y;
        }
    }
    
    int KP_EventsQueued(kero_platform_t *platform) {
        KP_UpdateMouse(platform);
        return XEventsQueued(platform->display, QueuedAfterFlush);
    }
    
    kp_event_t* KP_NextEvent(kero_platform_t *platform) {
        kp_event_t* event = (kp_event_t*)malloc(sizeof(kp_event_t));
        event->type = KP_EVENT_NONE;
        XEvent e;
        XNextEvent(platform->display, &e);
        switch(e.type) {
            case KeyPress:{
                event->type = KP_EVENT_KEY_PRESS;
                uint8_t symbol = (uint8_t)XLookupKeysym(&e.xkey, 0);
                platform->keyboard[symbol] = true;
                event->key = symbol;
            }break;
            case KeyRelease:{
                event->type = KP_EVENT_KEY_RELEASE;
                uint8_t symbol = (uint8_t)XLookupKeysym(&e.xkey, 0);
                platform->keyboard[symbol] = false;
                event->key = symbol;
            }break;
            case ButtonPress:{
                event->type = KP_EVENT_MOUSE_BUTTON_PRESS;
                event->x = platform->mouse.invertx ? platform->window.w - e.xbutton.x : e.xbutton.x;
                event->y = platform->mouse.inverty ? platform->window.h - e.xbutton.y : e.xbutton.y;
                event->button = MOUSE_OTHER;
                switch(e.xbutton.button) {
                    case Button1:{
                        event->button = MOUSE_LEFT;
                        platform->mouse.buttons |= MOUSE_LEFT;
                    }break;
                    case Button2:{
                        event->button = MOUSE_MIDDLE;
                        platform->mouse.buttons |= MOUSE_MIDDLE;
                    }break;
                    case Button3:{
                        event->button = MOUSE_RIGHT;
                        platform->mouse.buttons |= MOUSE_RIGHT;
                    }break;
                }
            }break;
            case ButtonRelease:{
                event->type = KP_EVENT_MOUSE_BUTTON_RELEASE;
                event->x = platform->mouse.invertx ? platform->window.w - e.xbutton.x : e.xbutton.x;
                event->y = platform->mouse.inverty ? platform->window.h - e.xbutton.y : e.xbutton.y;
                event->button = MOUSE_OTHER;
                switch(e.xbutton.button) {
                    case Button1:{
                        event->button = MOUSE_LEFT;
                        platform->mouse.buttons &= ~MOUSE_LEFT;
                    }break;
                    case Button2:{
                        event->button = MOUSE_MIDDLE;
                        platform->mouse.buttons &= ~MOUSE_MIDDLE;
                    }break;
                    case Button3:{
                        event->button = MOUSE_RIGHT;
                        platform->mouse.buttons &= ~MOUSE_RIGHT;
                    }break;
                }
            }break;
            case ClientMessage:{
                XClientMessageEvent* ev = (XClientMessageEvent*)&e;
                if((Atom)ev->data.l[0] == platform->WM_DELETE_WINDOW) {
                    event->type = KP_EVENT_QUIT;
                }
            }break;
            case ConfigureNotify:{
                XConfigureEvent* ev = (XConfigureEvent*)&e;
                if(ev->width != platform->window.w || ev->height != platform->window.h) {
                    event->type = KP_EVENT_RESIZE;
                    if(!platform->fullscreen) {
                        platform->windowed_width = ev->width;
                        platform->windowed_height = ev->height;
                    }
                    platform->window.w = ev->width;
                    platform->window.h = ev->height;
                    event->width = platform->window.w;
                    event->height = platform->window.h;
                    platform->frame_buffer.pixels = (uint32_t*)realloc(platform->frame_buffer.pixels, sizeof(uint32_t)*platform->window.w*platform->window.h);
                    platform->frame_buffer.w = platform->window.w;
                    platform->frame_buffer.h = platform->window.h;
                    platform->ximage = XCreateImage(platform->display, platform->visual_info.visual, platform->visual_info.depth, ZPixmap, 0, (char*)platform->frame_buffer.pixels, platform->window.w, platform->window.h, 32, 0);
                }
            }break;
            case DestroyNotify:{
                event->type = KP_EVENT_QUIT;
            }break;
            case FocusOut:{
                if(platform->reset_keyboard_on_focus_out) {
                    memset(platform->keyboard, 0, sizeof(platform->keyboard));
                }
                platform->mouse.buttons = 0;
                event->type = KP_EVENT_FOCUS_OUT;
            }break;
            case FocusIn:{
                event->type = KP_EVENT_FOCUS_IN;
            }break;
        }
        return event;
    }
    
    void KP_FreeEvent(kp_event_t* e) {
        free(e);
    }
    
    void KP_ShowCursor(kero_platform_t *platform, const bool show) {
        if(show) {
            XUndefineCursor(platform->display, platform->xwindow);
        }
        else{
            XColor color;
            const char data[1] = {0};
            Pixmap pixmap = XCreateBitmapFromData(platform->display, platform->xwindow, data, 1, 1);
            Cursor cursor = XCreatePixmapCursor(platform->display, pixmap, pixmap, &color, &color, 0, 0);
            XFreePixmap(platform->display, pixmap);
            XDefineCursor(platform->display, platform->xwindow, cursor);
            XFreeCursor(platform->display, cursor);
        }
    }
    
    double KP_Clock() {
        timespec clock_time;
        clock_gettime(CLOCK_REALTIME, &clock_time);
        return clock_time.tv_sec*1000.0 + clock_time.tv_nsec/1000000.0;
    }
    
    void KP_SetCursorPos(kero_platform_t *platform, int x, int y, int* dx, int* dy) {
        if(dx) *dx = x - platform->mouse.x;
        if(dy) *dy = y - platform->mouse.y;
        XWarpPointer(platform->display, platform->xwindow, platform->xwindow, 0, 0, 0, 0, platform->mouse.invertx ? platform->window.w - x : x, platform->mouse.inverty ? platform->window.h - y : y);
        XFlush(platform->display);
    }
    
    void KP_Fullscreen(kero_platform_t *platform, bool full) {
        platform->fullscreen = full;
        XEvent xev = {0};
        xev.type = ClientMessage;
        xev.xclient.window = platform->xwindow;
        xev.xclient.message_type = platform->_NET_WM_STATE_ATOM;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = _NET_WM_STATE_TOGGLE;
        xev.xclient.data.l[1] = XInternAtom(platform->display, "_NET_WM_STATE_FULLSCREEN", False);;
        xev.xclient.data.l[2] = 0;  /* no second property to toggle */
        xev.xclient.data.l[3] = 1;  /* source indication: application */
        xev.xclient.data.l[4] = 0;  /* unused */
        XSendEvent(platform->display, platform->root_window, 0, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    }
    
    void KP_OpenURL(const char* const url) {
        char command[1024];
        sprintf(command, "xdg-open %s", url);
        system(command);
    }
    
    // End of __linux__
    
    //------------------------------------------------------------
    
#else
    
#include <SDL2/SDL.h>
    
    // Mouse buttons
#define MOUSE_LEFT SDL_BUTTON_LEFT
#define MOUSE_RIGHT SDL_BUTTON_RIGHT
#define MOUSE_MIDDLE SDL_BUTTON_MIDDLE
#define MOUSE_OTHER SDL_BUTTON_MIDDLE
    
    // Keyboard keys
#define KEY_UP (SDL_SCANCODE_UP)
#define KEY_DOWN (SDL_SCANCODE_DOWN)
#define KEY_LEFT (SDL_SCANCODE_LEFT)
#define KEY_RIGHT (SDL_SCANCODE_RIGHT)
#define KEY_SPACE (SDL_SCANCODE_SPACE)
#define KEY_RETURN (SDL_SCANCODE_RETURN)
#define KEY_LALT (SDL_SCANCODE_LALT)
#define KEY_RALT (SDL_SCANCODE_LALT)
#define KEY_ESCAPE (SDL_SCANCODE_ESCAPE)
#define KEY_Q (SDL_SCANCODE_Q)
#define KEY_W (SDL_SCANCODE_W)
#define KEY_E (SDL_SCANCODE_E)
#define KEY_R (SDL_SCANCODE_R)
#define KEY_T (SDL_SCANCODE_T)
#define KEY_Y (SDL_SCANCODE_Y)
#define KEY_U (SDL_SCANCODE_U)
#define KEY_I (SDL_SCANCODE_I)
#define KEY_O (SDL_SCANCODE_O)
#define KEY_P (SDL_SCANCODE_P)
#define KEY_A (SDL_SCANCODE_A)
#define KEY_S (SDL_SCANCODE_S)
#define KEY_D (SDL_SCANCODE_D)
#define KEY_F (SDL_SCANCODE_F)
#define KEY_G (SDL_SCANCODE_G)
#define KEY_H (SDL_SCANCODE_H)
#define KEY_J (SDL_SCANCODE_J)
#define KEY_K (SDL_SCANCODE_K)
#define KEY_L (SDL_SCANCODE_L)
#define KEY_Z (SDL_SCANCODE_Z)
#define KEY_X (SDL_SCANCODE_X)
#define KEY_C (SDL_SCANCODE_C)
#define KEY_V (SDL_SCANCODE_V)
#define KEY_B (SDL_SCANCODE_B)
#define KEY_N (SDL_SCANCODE_N)
#define KEY_M (SDL_SCANCODE_M)
#define KEY_EQUAL (SDL_SCANCODE_EQUALS)
#define KEY_MINUS (SDL_SCANCODE_MINUS)
#define KEY_LSHIFT (SDL_SCANCODE_LSHIFT)
#define KEY_RSHIFT (SDL_SCANCODE_RSHIFT)
#define KEY_0 (SDL_SCANCODE_0)
#define KEY_1 (SDL_SCANCODE_1)
#define KEY_2 (SDL_SCANCODE_2)
#define KEY_3 (SDL_SCANCODE_3)
#define KEY_4 (SDL_SCANCODE_4)
#define KEY_5 (SDL_SCANCODE_5)
#define KEY_6 (SDL_SCANCODE_6)
#define KEY_7 (SDL_SCANCODE_7)
#define KEY_8 (SDL_SCANCODE_8)
#define KEY_9 (SDL_SCANCODE_9)
    
#define KEY_F1 (SDL_SCANCODE_F1)
#define KEY_F2 (SDL_SCANCODE_F2)
#define KEY_F3 (SDL_SCANCODE_F3)
#define KEY_F4 (SDL_SCANCODE_F4)
#define KEY_F5 (SDL_SCANCODE_F5)
#define KEY_F6 (SDL_SCANCODE_F6)
#define KEY_F7 (SDL_SCANCODE_F7)
#define KEY_F8 (SDL_SCANCODE_F8)
#define KEY_F9 (SDL_SCANCODE_F9)
#define KEY_F10 (SDL_SCANCODE_F10)
#define KEY_F11 (SDL_SCANCODE_F11)
#define KEY_F12 (SDL_SCANCODE_F12)
    
    void KP_SetWindowTitle(kero_platform_t *platform, const char* const title) {
        SDL_SetWindowTitle(platform->sdlwindow, title);
    }
    
    // fps = 0 to disable frame limiting
    void KP_SetTargetFramerate(kero_platform_t *platform, const unsigned int fps) {
        if(fps) {
            platform->target_frame_time = 1000/fps;
        }
        else{
            platform->target_frame_time = 0;
        }
    }
    
    void KP_Init(kero_platform_t *platform, const unsigned int width, const unsigned int height, const char* const title) {
        platform->delta = 0;
        platform->reset_keyboard_on_focus_out = true;
        platform->fullscreen = false;
        platform->windowed_width = 0;
        platform->windowed_height = 0;
        platform->windowed_x = 0;
        platform->windowed_y = 0;
        platform->target_frame_time = 0;
        SDL_Init(SDL_INIT_VIDEO);
        platform->windowed_width = width;
        platform->windowed_height = height;
        platform->window.w = width;
        platform->window.h = height;
        platform->sdlwindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        platform->canvas = SDL_GetWindowSurface(platform->sdlwindow);
        platform->frame_buffer.w = platform->window.w;
        platform->frame_buffer.h = platform->window.h;
        platform->frame_buffer.pixels = (uint32_t*)platform->canvas->pixels;
        platform->keyboard = SDL_GetKeyboardState(NULL);
        KP_SetTargetFramerate(platform, 60);
        platform->frame_start = SDL_GetTicks();
    }
    
    void KP_Sleep(unsigned long nanoseconds) {
        SDL_Delay(nanoseconds / 1000000);
    }
    
    void KP_Flip(kero_platform_t *platform) {
        SDL_UpdateWindowSurface(platform->sdlwindow);
        if(platform->target_frame_time) {
            // Delay until we take up the full frame time
            platform->frame_finish = SDL_GetTicks();
            int32_t sleep_time = platform->target_frame_time - (platform->frame_finish - platform->frame_start);
            sleep_time = sleep_time > 0 ? sleep_time : 0;
            SDL_Delay(sleep_time);
        }
        platform->frame_finish = SDL_GetTicks();
        platform->delta = (platform->frame_finish - platform->frame_start) / 1000.f;
        platform->frame_start = platform->frame_finish;
    }
    
    void KP_UpdateMouse(kero_platform_t *platform) {
        platform->mouse.buttons = SDL_GetMouseState(&platform->mouse.x, &platform->mouse.y);
        if(platform->mouse.invertx) {
            platform->mouse.x = platform->window.w - platform->mouse.x;
        }
        if(platform->mouse.inverty) {
            platform->mouse.y = platform->window.h - platform->mouse.y;
        }
    }
    
    int KP_EventsQueued(kero_platform_t *platform) {
        KP_UpdateMouse(platform);
        return SDL_PollEvent(0);
    }
    
    kp_event_t* KP_NextEvent(kero_platform_t *platform) {
        kp_event_t* event = (kp_event_t*)malloc(sizeof(kp_event_t));
        event->type = KP_EVENT_NONE;
        SDL_Event e;
        if(SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_KEYDOWN:{
                    event->type = KP_EVENT_KEY_PRESS;
                    int unsigned symbol = SDL_GetScancodeFromKey(e.key.keysym.sym);
                    event->key = symbol;
                }break;
                case SDL_KEYUP:{
                    if(!e.key.repeat) {
                        event->type = KP_EVENT_KEY_RELEASE;
                        int unsigned symbol = SDL_GetScancodeFromKey(e.key.keysym.sym);
                        event->key = symbol;
                    }
                }break;
                case SDL_MOUSEBUTTONDOWN:{
                    event->type = KP_EVENT_MOUSE_BUTTON_PRESS;
                    event->x = platform->mouse.invertx ? platform->window.w - e.button.x : e.button.x;
                    event->y = platform->mouse.inverty ? platform->window.h - e.button.y : e.button.y;
                    event->button = MOUSE_OTHER;
                    switch(e.button.button) {
                        case SDL_BUTTON_LEFT:{
                            event->button = MOUSE_LEFT;
                            platform->mouse.buttons |= MOUSE_LEFT;
                        }break;
                        case SDL_BUTTON_RIGHT:{
                            event->button = MOUSE_RIGHT;
                            platform->mouse.buttons |= MOUSE_RIGHT;
                        }break;
                    }
                }break;
                case SDL_MOUSEBUTTONUP:{
                    event->type = KP_EVENT_MOUSE_BUTTON_RELEASE;
                    event->x = platform->mouse.invertx ? platform->window.w - e.button.x : e.button.x;
                    event->y = platform->mouse.inverty ? platform->window.h - e.button.y : e.button.y;
                    event->button = MOUSE_OTHER;
                    switch(e.button.button) {
                        case SDL_BUTTON_LEFT:{
                            event->button = MOUSE_LEFT;
                            platform->mouse.buttons &= ~MOUSE_LEFT;
                        }break;
                        case SDL_BUTTON_RIGHT:{
                            event->button = MOUSE_RIGHT;
                            platform->mouse.buttons &= ~MOUSE_RIGHT;
                        }break;
                    }
                }break;
                case SDL_QUIT:{
                    event->type = KP_EVENT_QUIT;
                }break;
                case SDL_WINDOWEVENT:{
                    switch(e.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:{
                            event->type = KP_EVENT_RESIZE;
                            if(!platform->fullscreen) {
                                platform->windowed_width = e.window.data1;
                                platform->windowed_height = e.window.data2;
                            }
                            platform->window.w = e.window.data1;
                            platform->window.h = e.window.data2;
                            event->width = platform->window.w;
                            event->height = platform->window.h;
                            SDL_FreeSurface(platform->canvas);
                            platform->canvas = SDL_GetWindowSurface(platform->sdlwindow);
                            platform->frame_buffer.pixels = platform->canvas->pixels;
                            platform->frame_buffer.w = platform->window.w;
                            platform->frame_buffer.h = platform->window.h;
                        }break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:{
                            event->type = KP_EVENT_FOCUS_OUT;
                        }break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:{
                            event->type = KP_EVENT_FOCUS_IN;
                        }break;
                    }
                }break;
            }
        }
        
        return event;
    }
    
    void KP_FreeEvent(kp_event_t* e) {
        free(e);
    }
    
    void KP_ShowCursor(kero_platform_t *platform, const bool show) {
        SDL_ShowCursor(show);
    }
    
    double KP_Clock() {
        return SDL_GetTicks();
    }
    
    void KP_SetCursorPos(kero_platform_t *platform, int x, int y, int* dx, int* dy) {
        if(dx) *dx = x - platform->mouse.x;
        if(dy) *dy = y - platform->mouse.y;
        SDL_WarpMouseInWindow(platform->sdlwindow, platform->mouse.invertx ? platform->window.w - x : x, platform->mouse.inverty ? platform->window.h - y : y);
    }
    
    void KP_Fullscreen(kero_platform_t *platform, bool full) {
        platform->fullscreen = full;
        if(full) {
            SDL_GetWindowSize(platform->sdlwindow, &platform->windowed_width, &platform->windowed_height);
            SDL_GetWindowPosition(platform->sdlwindow, &platform->windowed_x, &platform->windowed_y);
            SDL_SetWindowFullscreen(platform->sdlwindow, SDL_WINDOW_FULLSCREEN);
        }
        else {
            SDL_SetWindowFullscreen(platform->sdlwindow, 0);
            SDL_SetWindowPosition(platform->sdlwindow, platform->windowed_x, platform->windowed_y);
        }
    }
    
#if _WIN32
    
#else
    
#endif
    
#endif // Not Linux
    
#if !_WIN32
    
    // The same for Linux and Mac
#include <unistd.h>
#include <pwd.h>
    char* KP_GetUserHomeDirectory() {
        struct passwd* pw = getpwuid(getuid());
        if(pw) {
            return pw->pw_dir;
        }
        else {
            char* home_path = getenv("HOME");
            if(home_path) {
                return home_path;
            }
            else {
                return NULL;
            }
        }
    }
    
#if !__linux__
    // Mac
    void KP_OpenURL(const char* const url) {
        char command[1024];
        sprintf(command, "open %s", url);
        system(command);
    }
#endif
    
#else // Then it's Windows
    
    void KP_OpenURL(const char* const url) {
        char command[1024];
        sprintf(command, "start %s", url);
        system(command);
    }
    
    char* KP_GetUserHomeDirectory(bool append_defaults) {
        char* user_profile = getenv("USERPROFILE");
        if(user_profile) {
            return user_profile;
        }
        else {
            char* home_drive = getenv("HOMEDRIVE");
            char* home_path = getenv("HOMEDRIVE");
            if(home_drive && home_path) {
                char* home_concatenated = (char*)malloc(sizeof(char)*(strlen(home_drive)+strlen(home_path)));
                free(home_drive);
                free(home_path);
                return home_concatenated;
            }
            else {
                return NULL;
            }
        }
    }
#endif
    
    // The same across Linux, Windows and Mac
    void KP_CreateDirectory(const char* const path) {
        char command[1024];
        sprintf(command, "mkdir \"%s\"", path);
        system(command);
    }
    
    //------------------------------------------------------------
    
#ifdef __cplusplus
}
#endif

#define KERO_PLATFORM_H
#endif
