/*
kero_maze.h created by Nicholas Walton. Released to the public domain. If a license is required, licensed under MIT.

*******************************************************************************
*                               MIT LICENSE                                   *
*******************************************************************************
Copyright 2019 Nicholas Walton

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************

Dependencies:

   Stretchy buffer for dynamically sized arrays: https://github.com/nothings/stb/blob/master/stretchy_buffer.h
   
Vec2: https://gitlab.com/UltimaN3rd/croaking-kero-c-libraries/blob/master/include/kero_vec2.h

To draw them as they generate: https://gitlab.com/UltimaN3rd/croaking-kero-c-libraries/blob/master/include/kero_sprite.h

With a few small changes you can replace these dependancies with your own libraries. Or lose the walls and animated generation by using "kero_maze_standalone.h"

*/


#ifndef KERO_MAZE_H

#ifdef __cplusplus
extern "C"{
#endif
    
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "stretchy_buffer.h"
#include "kero_sprite.h"
#include "kero_vec2.h"
    
    typedef struct {
        int w, h, cell_size;
        vec2i_t start, end;
        uint8_t* cells;
    } maze_t;
    // cells must be initialized to 0 or calling MazeFree() or any Generate() functions will segfault as they try to free() cells.
    
    typedef struct{
        vec2_t a, b;
    } wall_t;
    
    typedef enum{
        MAZE_LEFT = 0b1,
        MAZE_UP = 0b10,
        MAZE_RIGHT = 0b100,
        MAZE_DOWN = 0b1000,
        MAZE_DIRECTION_COUNT = 4
    } MAZE_DIRECTION;
    const uint8_t MAZE_CELL_VISITED = 0b10000000;
    
    
    
    // Function declarations
    
#define MazeGenerate MazeGeneratePersistentWalk
#define MazeFindSolution MazeDijkstra
    
    bool MazeGeneratePersistentWalk(maze_t* maze, wall_t** walls, int w, int h, int cell_size, float persistence, ksprite_t* frame_buffer, kero_platform_t *platform);
    /*
    The crown jewel of this lib. Generates mazes that vary in texture based on the persistence value.
 Persistence = 0 generates very similar to Prim's or Aldous-Broder - short corridors with many forks, radial texture.
 Persistence = infinity generates very similar to recursive backtracker, except instead of starting new paths at the first possible previous cell, it selects a random cell to start the new path. Long corridors, few forks, disarrayed texture.
 Persistence = 2 seems to be a sweet spot for modest corridors with plenty of forking and an off-radial texture.
    */
    
    bool MazeGenerateShortWalk(maze_t* maze, wall_t** walls, int w, int h, int cell_size, ksprite_t* frame_buffer, kero_platform_t *platform);
    /*
         Similar in texture to Prim's or Aldous-Broder.
    */
    
    bool MazeGenerateRecursiveBacktracker(maze_t* maze, wall_t** walls, int w, int h, int cell_size, ksprite_t* frame_buffer, kero_platform_t *platform);
    /*
    Creates long passages with few forks.
    */
    
    bool MazeGeneratePrims(maze_t* maze, wall_t** walls, int w, int h, int cell_size, ksprite_t* frame_buffer, kero_platform_t *platform);
    
    int MazeDijkstra(maze_t* maze, unsigned int** weights);
    /*
    Using Dijkstra's algorithm calculate the weight of each cell (the distance from the start). Sets maze.end to the heaviest cell.
    */
    
    bool MazeGenerateWalls(maze_t* maze, wall_t** walls);
    /*
    Called automatically by maze generation functions if the walls pointer is supplied.
        */
    
    bool MazeGenerateWallsNoOuterEdges(maze_t* maze, wall_t** walls);
    /*
    Generate walls without forcing closed outer edges
        */
    
    void MazeDrawCell(maze_t* maze, int x, int y, ksprite_t* dest, uint32_t color);
    /*
    Draws a single cell in the given color with left and down walls in black.
    */
    
    void MazeFree(maze_t* maze);
    /*
    Free memory used by maze.cells and set to NULL. Called automatically by each generate() function so does not need to be called to generate a new maze.
    */
    
#define KMazeFMod(numerator, denominator) ((numerator) - (int)((numerator)/(denominator))*(denominator))
    /*
    Just used internally to avoid depending on an external math lib
    */
    
    
    
    // Function definitions
    
    bool MazeGeneratePersistentWalk(maze_t* maze, wall_t** walls, int w, int h, int cell_size, float persistence, ksprite_t* frame_buffer, kero_platform_t *platform) {
        MazeFree(maze);
        maze->w = w;
        maze->h = h;
        maze->cell_size = cell_size;
        maze->cells = (uint8_t*)calloc(sizeof(uint8_t), maze->w*maze->h);
        int *visited_cells = (int*)malloc(sizeof(int)*maze->w*maze->h);
        int num_visited_cells = 1;
        typedef enum {
            ALeft, AUp, ARight, ADown
        } ALGO_DIRECTION;
        ALGO_DIRECTION dir;
        int x = rand()%maze->w;
        int y = rand()%maze->h;
        visited_cells[0] = x+y*maze->w;
        maze->start.x = x;
        maze->start.y = y;
        maze->cells[y*maze->w + x] = MAZE_CELL_VISITED;
        int px, py;
        float current_persistence = 0;
        while(num_visited_cells > 0) {
            while(KP_EventsQueued(platform)) {
                kp_event_t* e = KP_NextEvent(platform);
                switch(e->type) {
                    case KP_EVENT_KEY_PRESS:{
                        switch(e->key) {
                            case KEY_ESCAPE:{
                                exit(0);
                            }break;
                            case KEY_SPACE:{
                                frame_buffer = NULL;
                            }break;
                        }
                    }break;
                    case KP_EVENT_RESIZE:{
                        frame_buffer->pixels = platform->frame_buffer.pixels;
                        frame_buffer->w = platform->frame_buffer.w;
                        frame_buffer->h = platform->frame_buffer.h;
                    }break;
                    case KP_EVENT_QUIT:{
                        exit(0);
                    }break;
                }
                KP_FreeEvent(e);
            }
            px = x;
            py = y;
            bool did_visit = false;
            int directions[MAZE_DIRECTION_COUNT];
            if ((x > 0 && !(maze->cells[x-1+y*maze->w] & MAZE_CELL_VISITED)) || (x < maze->w-1 && !(maze->cells[x+1+y*maze->w] & MAZE_CELL_VISITED)) || (y > 0 && !(maze->cells[x+(y-1)*maze->w] & MAZE_CELL_VISITED)) || (y < maze->h-1 && !(maze->cells[x+(y+1)*maze->w] & MAZE_CELL_VISITED))) {
                current_persistence += persistence;
                directions[0] = rand()%MAZE_DIRECTION_COUNT;
                for(int i = 1; i < MAZE_DIRECTION_COUNT; ++i) {
                    bool already_did_that_direction = false;
                    do {
                        directions[i] = rand()%MAZE_DIRECTION_COUNT;
                        for(int j = 0; j < i && !already_did_that_direction; ++j) {
                            if(directions[i] == directions[j]) {
                                already_did_that_direction = true;
                            }
                        }
                    } while(!already_did_that_direction);
                }
                for(int i = 0; i < MAZE_DIRECTION_COUNT && current_persistence-- >= 1 && !did_visit; ++i) {
                    dir = directions[i];
                    switch(dir){
                        case ALeft:{
                            if(x > 0 && !(maze->cells[y*maze->w + x-1] & MAZE_CELL_VISITED)) {
                                maze->cells[y*maze->w + x] |= MAZE_LEFT;
                                --x;
                                maze->cells[y*maze->w + x] |= MAZE_RIGHT;
                                did_visit = true;
                            }
                        }break;
                        case ARight:{
                            if(x < maze->w-1 && !(maze->cells[y*maze->w + x+1] & MAZE_CELL_VISITED)){
                                maze->cells[y*maze->w + x] |= MAZE_RIGHT;
                                ++x;
                                maze->cells[y*maze->w + x] |= MAZE_LEFT;
                                did_visit = true;
                            }
                        }break;
                        case AUp:{
                            if(y < maze->h-1 && !(maze->cells[(y+1)*maze->w + x] & MAZE_CELL_VISITED)){
                                maze->cells[y*maze->w + x] |= MAZE_UP;
                                ++y;
                                maze->cells[y*maze->w + x] |= MAZE_DOWN;
                                did_visit = true;
                            }
                        }break;
                        case ADown:{
                            if(y > 0 && !(maze->cells[(y-1)*maze->w + x] & MAZE_CELL_VISITED)){
                                maze->cells[y*maze->w + x] |= MAZE_DOWN;
                                --y;
                                maze->cells[y*maze->w + x] |= MAZE_UP;
                                did_visit = true;
                            }
                        }break;
                    }
                }
                current_persistence = KMazeFMod(current_persistence, 1);
            }
            if(did_visit){
                visited_cells[num_visited_cells++] = x + y*maze->w;
                if(frame_buffer) {
                    MazeDrawCell(maze, px, py, frame_buffer, 0xffffffff);
                    MazeDrawCell(maze, x, y, frame_buffer, 0xffffffff);
                    KP_Flip(platform);
                }
                int new_cell = y*maze->w + x;
                maze->cells[new_cell] |= MAZE_CELL_VISITED;
            }
            else {
                for(;;) {
                    int cell = rand()%num_visited_cells;
                    x = visited_cells[cell]%maze->w;
                    y = visited_cells[cell]/maze->w;
                    if (!(x == 0 || maze->cells[x-1+y*maze->w] & MAZE_CELL_VISITED) || !(x == maze->w-1 || maze->cells[x+1+y*maze->w] & MAZE_CELL_VISITED) || !(y == 0 || maze->cells[x+(y-1)*maze->w] & MAZE_CELL_VISITED) || !(y == maze->h-1 || maze->cells[x+(y+1)*maze->w] & MAZE_CELL_VISITED)) {
                        break;
                    }
                    else {
                        if(--num_visited_cells == 0) {
                            break;
                        }
                        if(frame_buffer) {
                            MazeDrawCell(maze, x, y, frame_buffer, 0xffbbbbbb);
                        }
                        for(int i = cell; i < num_visited_cells; ++i) {
                            visited_cells[i] = visited_cells[i+1];
                        }
                    }
                }
            }
        }
        maze->end.y = 0;
        maze->end.x = 0;
        for(unsigned int y = 0; y < maze->h; ++y){
            for(unsigned int x = 0; x < maze->w; ++x){
                if(maze->cells[y*maze->w+x] & MAZE_CELL_VISITED){
                    maze->cells[y*maze->w+x] &= ~MAZE_CELL_VISITED;
                }
            }
        }
        free(visited_cells);
        MazeGenerateWalls(maze, walls);
        return true;
    }
    
    bool MazeGenerateShortWalk(maze_t* maze, wall_t** walls, int w, int h, int cell_size, ksprite_t* frame_buffer, kero_platform_t *platform) {
        MazeFree(maze);
        maze->w = w;
        maze->h = h;
        maze->cell_size = cell_size;
        maze->cells = (uint8_t*)calloc(sizeof(uint8_t), maze->w*maze->h);
        int *visited_cells = (int*)malloc(sizeof(int)*maze->w*maze->h);
        int num_visited_cells = 1;
        typedef enum {
            ALeft, AUp, ARight, ADown
        } ALGO_DIRECTION;
        ALGO_DIRECTION dir;
        int x = rand()%maze->w;
        int y = rand()%maze->h;
        visited_cells[0] = x+y*maze->w;
        maze->start.x = x;
        maze->start.y = y;
        uint8_t MAZE_CELL_VISITED = 0b10000000;
        maze->cells[y*maze->w + x] = MAZE_CELL_VISITED;
        int px, py;
        while(num_visited_cells > 0) {
            while(KP_EventsQueued(platform)) {
                kp_event_t* e = KP_NextEvent(platform);
                switch(e->type) {
                    case KP_EVENT_KEY_PRESS:{
                        switch(e->key) {
                            case KEY_ESCAPE:{
                                exit(0);
                            }break;
                            case KEY_SPACE:{
                                frame_buffer = NULL;
                            }break;
                        }
                    }break;
                    case KP_EVENT_RESIZE:{
                        frame_buffer->pixels = platform->frame_buffer.pixels;
                        frame_buffer->w = platform->frame_buffer.w;
                        frame_buffer->h = platform->frame_buffer.h;
                    }break;
                    case KP_EVENT_QUIT:{
                        exit(0);
                    }break;
                }
                KP_FreeEvent(e);
            }
            px = x;
            py = y;
            dir = rand()%4;
            bool did_visit = false;
            switch(dir){
                case ALeft:{
                    if(x > 0 && !(maze->cells[y*maze->w + x-1] & MAZE_CELL_VISITED)) {
                        maze->cells[y*maze->w + x] |= MAZE_LEFT;
                        --x;
                        maze->cells[y*maze->w + x] |= MAZE_RIGHT;
                        did_visit = true;
                    }
                }break;
                case ARight:{
                    if(x < maze->w-1 && !(maze->cells[y*maze->w + x+1] & MAZE_CELL_VISITED)){
                        maze->cells[y*maze->w + x] |= MAZE_RIGHT;
                        ++x;
                        maze->cells[y*maze->w + x] |= MAZE_LEFT;
                        did_visit = true;
                    }
                }break;
                case AUp:{
                    if(y < maze->h-1 && !(maze->cells[(y+1)*maze->w + x] & MAZE_CELL_VISITED)){
                        maze->cells[y*maze->w + x] |= MAZE_UP;
                        ++y;
                        maze->cells[y*maze->w + x] |= MAZE_DOWN;
                        did_visit = true;
                    }
                }break;
                case ADown:{
                    if(y > 0 && !(maze->cells[(y-1)*maze->w + x] & MAZE_CELL_VISITED)){
                        maze->cells[y*maze->w + x] |= MAZE_DOWN;
                        --y;
                        maze->cells[y*maze->w + x] |= MAZE_UP;
                        did_visit = true;
                    }
                }break;
            }
            if(did_visit){
                visited_cells[num_visited_cells++] = x + y*maze->w;
                if(frame_buffer) {
                    MazeDrawCell(maze, px, py, frame_buffer, 0xffffffff);
                    MazeDrawCell(maze, x, y, frame_buffer, 0xffffffff);
                    KP_Flip(platform);
                }
                int new_cell = y*maze->w + x;
                maze->cells[new_cell] |= MAZE_CELL_VISITED;
            }
            else {
                for(;;) {
                    int cell = rand()%num_visited_cells;
                    x = visited_cells[cell]%maze->w;
                    y = visited_cells[cell]/maze->w;
                    if (!(x == 0 || maze->cells[x-1+y*maze->w] & MAZE_CELL_VISITED) || !(x == maze->w-1 || maze->cells[x+1+y*maze->w] & MAZE_CELL_VISITED) || !(y == 0 || maze->cells[x+(y-1)*maze->w] & MAZE_CELL_VISITED) || !(y == maze->h-1 || maze->cells[x+(y+1)*maze->w] & MAZE_CELL_VISITED)) {
                        break;
                    }
                    else {
                        if(--num_visited_cells == 0) {
                            break;
                        }
                        for(int i = cell; i < num_visited_cells; ++i) {
                            visited_cells[i] = visited_cells[i+1];
                        }
                    }
                }
            }
        }
        maze->end.y = 0;
        maze->end.x = 0;
        for(unsigned int y = 0; y < maze->h; ++y){
            for(unsigned int x = 0; x < maze->w; ++x){
                if(maze->cells[y*maze->w+x] & MAZE_CELL_VISITED){
                    maze->cells[y*maze->w+x] &= ~MAZE_CELL_VISITED;
                }
            }
        }
        free(visited_cells);
        MazeGenerateWalls(maze, walls);
        return true;
    }
    
    bool MazeGenerateRecursiveBacktracker(maze_t* maze, wall_t** walls, int w, int h, int cell_size, ksprite_t* frame_buffer, kero_platform_t *platform) {
        MazeFree(maze);
        maze->w = w;
        maze->h = h;
        maze->cell_size = cell_size;
        maze->cells = (uint8_t*)calloc(sizeof(uint8_t), maze->w*maze->h);
        int *maze_stack = (int*)malloc(sizeof(int)*maze->w*maze->h);
        int maze_stack_top = 0;
        int num_cells_to_visit = maze->w*maze->h-1;
        typedef enum {
            ALeft, AUp, ARight, ADown
        } ALGO_DIRECTION;
        ALGO_DIRECTION dir;
        int x = rand()%maze->w;
        int y = rand()%maze->h;
        maze->start.x = x;
        maze->start.y = y;
        maze_stack[0] = y*maze->w + x;
        uint8_t MAZE_CELL_VISITED = 0b10000000;
        maze->cells[y*maze->w + x] = MAZE_CELL_VISITED;
        int deepest_cell = 0;
        int deepest_cell_depth = 0;
        do {
            // Draw the maze
            if(frame_buffer){
                KS_SetAllPixels(frame_buffer, 0xffffffff);
                KS_DrawRectFilled(frame_buffer, x*maze->cell_size, y*maze->cell_size, (x+1)*maze->cell_size, (y+1)*maze->cell_size, 0xff00ff00);
                for(int y = 0; y < maze->h; ++y) {
                    for(int x = 0; x < maze->w; ++x) {
                        if( !(maze->cells[x + y*maze->w] & MAZE_UP) ) {
                            KS_DrawLine(frame_buffer, x*maze->cell_size, (y+1)*maze->cell_size, (x+1)*maze->cell_size, (y+1)*maze->cell_size, 0xff000000);
                        }
                        if( !(maze->cells[x + y*maze->w] & MAZE_RIGHT) ) {
                            KS_DrawLine(frame_buffer, (x+1)*maze->cell_size, y*maze->cell_size, (x+1)*maze->cell_size, (y+1)*maze->cell_size, 0xff000000);
                        }
                    }
                }
                KP_Flip(platform);
            }
            dir = rand();
            bool did_visit = false;
            for(int direction_attempts = 0; direction_attempts < MAZE_DIRECTION_COUNT && !did_visit; ++direction_attempts){
                dir = (dir+1)%4;
                switch(dir){
                    case ALeft:{
                        if(x == 0 || maze->cells[y*maze->w + x-1] & MAZE_CELL_VISITED){
                        }else{
                            maze->cells[y*maze->w + x] |= MAZE_LEFT;
                            --x;
                            maze->cells[y*maze->w + x] |= MAZE_RIGHT;
                            did_visit = true;
                        }
                    }break;
                    case ARight:{
                        if(x == maze->w-1 || maze->cells[y*maze->w + x+1] & MAZE_CELL_VISITED){
                        }else{
                            maze->cells[y*maze->w + x] |= MAZE_RIGHT;
                            ++x;
                            maze->cells[y*maze->w + x] |= MAZE_LEFT;
                            did_visit = true;
                        }
                    }break;
                    case AUp:{
                        if(y == maze->h-1 || maze->cells[(y+1)*maze->w + x] & MAZE_CELL_VISITED){
                        }else{
                            maze->cells[y*maze->w + x] |= MAZE_UP;
                            ++y;
                            maze->cells[y*maze->w + x] |= MAZE_DOWN;
                            did_visit = true;
                        }
                    }break;
                    case ADown:{
                        if(y == 0 || maze->cells[(y-1)*maze->w + x] & MAZE_CELL_VISITED){
                        }else{
                            maze->cells[y*maze->w + x] |= MAZE_DOWN;
                            --y;
                            maze->cells[y*maze->w + x] |= MAZE_UP;
                            did_visit = true;
                        }
                    }break;
                }
            }
            if(did_visit){
                --num_cells_to_visit;
                int new_cell = y*maze->w + x;
                maze->cells[new_cell] |= MAZE_CELL_VISITED;
                maze_stack[++maze_stack_top] = new_cell;
                if(maze_stack_top > deepest_cell_depth){
                    deepest_cell_depth = maze_stack_top;
                    deepest_cell = new_cell;
                }
            }else{
                y = maze_stack[maze_stack_top] / maze->w;
                x = maze_stack[maze_stack_top] - y*maze->w;
                --maze_stack_top;
            }
        } while (num_cells_to_visit);
        maze->end.y = deepest_cell / maze->w;
        maze->end.x = deepest_cell - maze->end.y*maze->w;
        for(unsigned int y = 0; y < maze->h; ++y){
            for(unsigned int x = 0; x < maze->w; ++x){
                if(maze->cells[y*maze->w+x] & MAZE_CELL_VISITED){
                    maze->cells[y*maze->w+x] &= ~MAZE_CELL_VISITED;
                }
            }
        }
        free(maze_stack);
        MazeGenerateWalls(maze, walls);
        return true;
    }
    
    bool MazeGeneratePrims(maze_t* maze, wall_t** walls, int w, int h, int cell_size, ksprite_t* frame_buffer, kero_platform_t *platform) {
        // Preparation
        MazeFree(maze);
        maze->w = w;
        maze->h = h;
        maze->cell_size = cell_size;
        maze->cells = (uint8_t*)calloc(sizeof(uint8_t), maze->w * maze->h);
        int* visited_cells = (int*)malloc(maze->w * maze->h * sizeof(int));
        int num_visited_cells = 1;
        // Step 1: Select a random point, mark as visited and add it to the list of visited cells.
        visited_cells[0] = rand() % (maze->w*maze->h);
        maze->cells[visited_cells[0]] |= MAZE_CELL_VISITED;
        // Step 2: Until the list of visited cells is empty. . .
        while(num_visited_cells > 0) {
            // Step 3: Select a random cell from the list of visited cells.
            int selected = rand()%num_visited_cells;
            int cell = visited_cells[selected];
            int x = cell%maze->w;
            int y = cell/maze->w;
            // Step 4: If the current cell has no unvisited neighbours, remove it from the list. Go to (2)
            if(!(y < maze->h-1 && !(maze->cells[x + (y+1)*maze->w] & MAZE_CELL_VISITED) ) && !(y > 0 && !(maze->cells[x + (y-1)*maze->w] & MAZE_CELL_VISITED) ) && !(x < maze->w-1 && !(maze->cells[x+1 + y*maze->w] & MAZE_CELL_VISITED) ) && !(x > 0 && !(maze->cells[x-1 + y*maze->w] & MAZE_CELL_VISITED) )) {
                --num_visited_cells;
                for(int i = selected; i < num_visited_cells; ++i) {
                    visited_cells[i] = visited_cells[i+1];
                }
            }
            // Step 5: Connect to a random unvisited neighbour of the current cell, mark that neighbour as visited and add it to the list. Go to (2)
            else {
                int direction = rand();
                bool connected = false;
                for(int neighbour_checks = 0; neighbour_checks < MAZE_DIRECTION_COUNT && !connected; ++neighbour_checks) {
                    direction = (direction+1)%4;
                    switch(direction) {
                        case 0: { // Up
                            if( y < maze->h-1 && !(maze->cells[x + (y+1)*maze->w] & MAZE_CELL_VISITED) ) {
                                maze->cells[x + y*maze->w] |= MAZE_UP;
                                ++y;
                                maze->cells[x + y*maze->w] |= MAZE_DOWN;
                                connected = true;
                            }
                        }break;
                        case 1: { // Down
                            if( y > 0 && !(maze->cells[x + (y-1)*maze->w] & MAZE_CELL_VISITED) ) {
                                maze->cells[x + y*maze->w] |= MAZE_DOWN;
                                --y;
                                maze->cells[x + y*maze->w] |= MAZE_UP;
                                connected = true;
                            }
                        }break;
                        case 2: { // Right
                            if( x < maze->w-1 && !(maze->cells[x+1 + y*maze->w] & MAZE_CELL_VISITED) ) {
                                maze->cells[x + y*maze->w] |= MAZE_RIGHT;
                                ++x;
                                maze->cells[x + y*maze->w] |= MAZE_LEFT;
                                connected = true;
                            }
                        }break;
                        case 3: { // Left
                            if( x > 0 && !(maze->cells[x-1 + y*maze->w] & MAZE_CELL_VISITED) ) {
                                maze->cells[x + y*maze->w] |= MAZE_LEFT;
                                --x;
                                maze->cells[x + y*maze->w] |= MAZE_RIGHT;
                                connected = true;
                            }
                        }break;
                    }
                }
                visited_cells[num_visited_cells++] = x + y*maze->w;
                maze->cells[x + y*maze->w] |= MAZE_CELL_VISITED;
                // Draw the maze
                if(frame_buffer){
                    KS_SetAllPixels(frame_buffer, 0xffffffff);
                    for(int i = 0; i < num_visited_cells; ++i) {
                        int x = visited_cells[i]%maze->w;
                        int y = visited_cells[i]/maze->w;
                        KS_DrawRectFilled(frame_buffer, x*maze->cell_size, y*maze->cell_size, (x+1)*maze->cell_size, (y+1)*maze->cell_size, 0xff888888);
                    }
                    for(int y = 0; y < maze->h; ++y) {
                        for(int x = 0; x < maze->w; ++x) {
                            if( !(maze->cells[x + y*maze->w] & MAZE_UP) ) {
                                KS_DrawLine(frame_buffer, x*maze->cell_size, (y+1)*maze->cell_size, (x+1)*maze->cell_size, (y+1)*maze->cell_size, 0xff000000);
                            }
                            if( !(maze->cells[x + y*maze->w] & MAZE_RIGHT) ) {
                                KS_DrawLine(frame_buffer, (x+1)*maze->cell_size, y*maze->cell_size, (x+1)*maze->cell_size, (y+1)*maze->cell_size, 0xff000000);
                            }
                        }
                    }
                    KP_Flip(platform);
                }
            }
        }
        free(visited_cells);
        MazeGenerateWalls(maze, walls);
        return true;
    }
    
    void MazeFree(maze_t* maze) {
        if(maze->cells){
            free(maze->cells);
            maze->cells = NULL;
        }
    }
    
    int MazeDijkstra(maze_t* maze, unsigned int** weights) {
        int x = maze->start.x;
        int y = maze->start.y;
        int* maze_stack = (int*)malloc(sizeof(int)*maze->w*maze->h);
        int maze_stack_top = 0;
        maze_stack[0] = x+y*maze->w;
        *weights = (unsigned int*)malloc(sizeof(unsigned int)*maze->w*maze->h);
        int* w = *weights;
        for(int i = 0; i < maze->w*maze->h; ++i) {
            w[i] = -1;
        }
        w[x+y*maze->w] = 0;
        unsigned int weight = 0;
        char weight_str[4];
        unsigned int max_weight = 0;
        for(;;) {
            weight = w[x+y*maze->w];
            if(weight > max_weight) {
                max_weight = weight;
                maze->end.x = x;
                maze->end.y = y;
            }
            if(x < maze->w-1 && w[x+1+y*maze->w] > weight+1 && maze->cells[x+y*maze->w] & MAZE_RIGHT) {
                ++x;
                w[x+y*maze->w] = weight+1;
                maze_stack[++maze_stack_top] = x+y*maze->w;
                continue;
            }
            if(x > 0 && w[x-1+y*maze->w] > weight+1 && maze->cells[x+y*maze->w] & MAZE_LEFT) {
                --x;
                w[x+y*maze->w] = weight+1;
                maze_stack[++maze_stack_top] = x+y*maze->w;
                continue;
            }
            if(y < maze->h-1 && w[x+(y+1)*maze->w] > weight+1 && maze->cells[x+y*maze->w] & MAZE_UP) {
                ++y;
                w[x+y*maze->w] = weight+1;
                maze_stack[++maze_stack_top] = x+y*maze->w;
                continue;
            }
            if(y > 0 && w[x+(y-1)*maze->w] > weight+1 && maze->cells[x+y*maze->w] & MAZE_DOWN) {
                --y;
                w[x+y*maze->w] = weight+1;
                maze_stack[++maze_stack_top] = x+y*maze->w;
                continue;
            }
            if(--maze_stack_top < 0) break;
            x = maze_stack[maze_stack_top] % maze->w;
            y = maze_stack[maze_stack_top] / maze->w;
        }
        free(maze_stack);
        return max_weight;
    }
    
    void MazeDrawCell(maze_t* maze, int x, int y, ksprite_t* dest, uint32_t color) {
        if(x < 0 || x > maze->w-1 || y < 0 || y > maze->h-1) return;
        KS_DrawRectFilledSafe(dest, x*maze->cell_size, y*maze->cell_size, (x+1)*maze->cell_size-1, (y+1)*maze->cell_size-1, color);
        uint8_t cell = maze->cells[y*maze->w + x];
        if(!(cell & MAZE_LEFT)){KS_DrawLineSafe(dest, x*maze->cell_size, y*maze->cell_size, x*maze->cell_size, (y+1)*maze->cell_size, 0xff000000);
        }
        if(!(cell & MAZE_DOWN)){
            KS_DrawLineSafe(dest, x*maze->cell_size, y*maze->cell_size, (x+1)*maze->cell_size, y*maze->cell_size, 0xff000000);
        }
    }
    
    bool MazeGenerateWalls(maze_t* maze, wall_t** walls){
        if(*walls){
            sb_free(*walls);
            *walls = NULL;
        }
        wall_t* hwalls = NULL;
        wall_t* vwalls = NULL;
        uint8_t MAZE_CELL_WALLED_BOTTOM = 0b01000000;
        uint8_t MAZE_CELL_WALLED_LEFT = 0b00100000;
        wall_t new_wall = {0};
        for(unsigned int y = 0; y < maze->h; ++y){
            for(unsigned int x = 0; x < maze->w; ++x){
                if(!(maze->cells[y*maze->w+x] & MAZE_DOWN) && !(maze->cells[y*maze->w+x] & MAZE_CELL_WALLED_BOTTOM)){
                    maze->cells[y*maze->w+x] |= MAZE_CELL_WALLED_BOTTOM;
                    new_wall.a.x = x;
                    new_wall.a.y = y;
                    new_wall.b.y = y;
                    for(new_wall.b.x = x+1; new_wall.b.x < maze->w; ++new_wall.b.x){
                        maze->cells[(int)(y*maze->w+new_wall.b.x)] |= MAZE_CELL_WALLED_BOTTOM;
                        if(maze->cells[(int)(y*maze->w+new_wall.b.x)] & MAZE_DOWN){
                            break;
                        }
                    }
                    new_wall.a.x *= maze->cell_size;
                    new_wall.a.y *= maze->cell_size;
                    new_wall.b.x *= maze->cell_size;
                    new_wall.b.y *= maze->cell_size;
                    sb_push(hwalls, new_wall);
                }
                if(!(maze->cells[y*maze->w+x] & MAZE_LEFT) && !(maze->cells[y*maze->w+x] & MAZE_CELL_WALLED_LEFT)){
                    maze->cells[y*maze->w+x] |= MAZE_CELL_WALLED_LEFT;
                    new_wall.a.x = x;
                    new_wall.a.y = y;
                    new_wall.b.x = x;
                    for(new_wall.b.y = y+1; new_wall.b.y < maze->h; ++new_wall.b.y){
                        maze->cells[(int)(new_wall.b.y*maze->w+x)] |= MAZE_CELL_WALLED_LEFT;
                        if(maze->cells[(int)(new_wall.b.y*maze->w+x)] & MAZE_LEFT){
                            break;
                        }
                    }
                    new_wall.a.x *= maze->cell_size;
                    new_wall.a.y *= maze->cell_size;
                    new_wall.b.x *= maze->cell_size;
                    new_wall.b.y *= maze->cell_size;
                    sb_push(vwalls, new_wall);
                }
            }
        }
        wall_t right_edge_wall;
        right_edge_wall.a.x = maze->w*maze->cell_size;
        right_edge_wall.a.y = 0;
        right_edge_wall.b.x = maze->w*maze->cell_size;
        right_edge_wall.b.y = maze->h*maze->cell_size;
        sb_push(vwalls, right_edge_wall);
        wall_t top_edge_wall;
        top_edge_wall.a.x = 0;
        top_edge_wall.a.y = maze->h*maze->cell_size;
        top_edge_wall.b.x = maze->w*maze->cell_size;
        top_edge_wall.b.y = maze->h*maze->cell_size;
        sb_push(hwalls, top_edge_wall);
        vec2_t intersection;
        for(int hwall_it = 0; hwall_it < sb_count(hwalls); ++hwall_it){
            bool did_split = false;
            for(int vwall_it = 0; vwall_it < sb_count(vwalls); ++vwall_it){
                if(Vec2IntersectLineSegments(hwalls[hwall_it].a, hwalls[hwall_it].b, vwalls[vwall_it].a, vwalls[vwall_it].b, &intersection)){
                    if(!Vec2Equals(intersection, hwalls[hwall_it].a) && !Vec2Equals(intersection, hwalls[hwall_it].b) && !Vec2Equals(intersection, vwalls[vwall_it].a) && !Vec2Equals(intersection, vwalls[vwall_it].b)) {
                        did_split = true;
                        wall_t left_wall;
                        left_wall.a = hwalls[hwall_it].a;
                        left_wall.b.x = vwalls[vwall_it].a.x;
                        left_wall.b.y = hwalls[hwall_it].a.y;
                        sb_push(*walls, left_wall);
                        wall_t right_wall;
                        right_wall.a = hwalls[hwall_it].b;
                        right_wall.b.x = vwalls[vwall_it].a.x;
                        right_wall.b.y = hwalls[hwall_it].a.y;
                        sb_push(*walls, right_wall);
                    }
                }
            }
            if(!did_split){
                sb_push(*walls, hwalls[hwall_it]);
            }
        }
        for(int vwall_it = 0; vwall_it < sb_count(vwalls); ++vwall_it){
            sb_push(*walls, vwalls[vwall_it]);
        }
        sb_free(hwalls);
        sb_free(vwalls);
        for(unsigned int m = 0; m < maze->w*maze->h; ++m){
            maze->cells[m] &= ~MAZE_CELL_WALLED_BOTTOM;
            maze->cells[m] &= ~MAZE_CELL_WALLED_LEFT;
        }
        return true;
    }
    
    bool MazeGenerateWallsNoOuterEdges(maze_t* maze, wall_t** walls){
        if(*walls){
            sb_free(*walls);
            *walls = NULL;
        }
        wall_t* hwalls = NULL;
        wall_t* vwalls = NULL;
        uint8_t MAZE_CELL_WALLED_BOTTOM = 0b01000000;
        uint8_t MAZE_CELL_WALLED_LEFT = 0b00100000;
        wall_t new_wall = {0};
        for(unsigned int y = 0; y < maze->h; ++y){
            for(unsigned int x = 0; x < maze->w; ++x){
                if(!(maze->cells[y*maze->w+x] & MAZE_DOWN) && !(maze->cells[y*maze->w+x] & MAZE_CELL_WALLED_BOTTOM)){
                    maze->cells[y*maze->w+x] |= MAZE_CELL_WALLED_BOTTOM;
                    new_wall.a.x = x;
                    new_wall.a.y = y;
                    new_wall.b.y = y;
                    for(new_wall.b.x = x+1; new_wall.b.x < maze->w; ++new_wall.b.x){
                        maze->cells[(int)(y*maze->w+new_wall.b.x)] |= MAZE_CELL_WALLED_BOTTOM;
                        if(maze->cells[(int)(y*maze->w+new_wall.b.x)] & MAZE_DOWN){
                            break;
                        }
                    }
                    new_wall.a.x *= maze->cell_size;
                    new_wall.a.y *= maze->cell_size;
                    new_wall.b.x *= maze->cell_size;
                    new_wall.b.y *= maze->cell_size;
                    sb_push(hwalls, new_wall);
                }
                if(!(maze->cells[y*maze->w+x] & MAZE_LEFT) && !(maze->cells[y*maze->w+x] & MAZE_CELL_WALLED_LEFT)){
                    maze->cells[y*maze->w+x] |= MAZE_CELL_WALLED_LEFT;
                    new_wall.a.x = x;
                    new_wall.a.y = y;
                    new_wall.b.x = x;
                    for(new_wall.b.y = y+1; new_wall.b.y < maze->h; ++new_wall.b.y){
                        maze->cells[(int)(new_wall.b.y*maze->w+x)] |= MAZE_CELL_WALLED_LEFT;
                        if(maze->cells[(int)(new_wall.b.y*maze->w+x)] & MAZE_LEFT){
                            break;
                        }
                    }
                    new_wall.a.x *= maze->cell_size;
                    new_wall.a.y *= maze->cell_size;
                    new_wall.b.x *= maze->cell_size;
                    new_wall.b.y *= maze->cell_size;
                    sb_push(vwalls, new_wall);
                }
            }
        }
        new_wall.a.x = new_wall.b.x = maze->w*maze->cell_size;
        for(int y = 0; y < maze->h; ++y) {
            if(!(maze->cells[y*maze->w+maze->w-1] & MAZE_RIGHT)) {
                new_wall.a.y = y;
                new_wall.b.y = y+1;
                for(; y < maze->h; ++y) {
                    if(maze->cells[y*maze->w+maze->w-1] & MAZE_RIGHT) break;
                    new_wall.b.y = y+1;
                }
                new_wall.a.y *= maze->cell_size;
                new_wall.b.y *= maze->cell_size;
                sb_push(vwalls, new_wall);
            }
        }
        new_wall.a.y = new_wall.b.y = maze->h*maze->cell_size;
        for(int x = 0; x < maze->w; ++x) {
            if(!(maze->cells[(maze->h-1)*maze->w+x] & MAZE_UP)) {
                new_wall.a.x = x;
                new_wall.b.x = x+1;
                for(; x < maze->w; ++x) {
                    if(maze->cells[x + (maze->h-1)*maze->w] & MAZE_UP) break;
                    new_wall.b.x = x+1;
                }
                new_wall.a.x *= maze->cell_size;
                new_wall.b.x *= maze->cell_size;
                sb_push(vwalls, new_wall);
            }
        }
        vec2_t intersection;
        for(int hwall_it = 0; hwall_it < sb_count(hwalls); ++hwall_it){
            bool did_split = false;
            for(int vwall_it = 0; vwall_it < sb_count(vwalls); ++vwall_it){
                if(Vec2IntersectLineSegments(hwalls[hwall_it].a, hwalls[hwall_it].b, vwalls[vwall_it].a, vwalls[vwall_it].b, &intersection)){
                    if(!Vec2Equals(intersection, hwalls[hwall_it].a) && !Vec2Equals(intersection, hwalls[hwall_it].b) && !Vec2Equals(intersection, vwalls[vwall_it].a) && !Vec2Equals(intersection, vwalls[vwall_it].b)) {
                        did_split = true;
                        wall_t left_wall;
                        left_wall.a = hwalls[hwall_it].a;
                        left_wall.b.x = vwalls[vwall_it].a.x;
                        left_wall.b.y = hwalls[hwall_it].a.y;
                        sb_push(*walls, left_wall);
                        wall_t right_wall;
                        right_wall.a = hwalls[hwall_it].b;
                        right_wall.b.x = vwalls[vwall_it].a.x;
                        right_wall.b.y = hwalls[hwall_it].a.y;
                        sb_push(*walls, right_wall);
                    }
                }
            }
            if(!did_split){
                sb_push(*walls, hwalls[hwall_it]);
            }
        }
        for(int vwall_it = 0; vwall_it < sb_count(vwalls); ++vwall_it){
            sb_push(*walls, vwalls[vwall_it]);
        }
        sb_free(hwalls);
        sb_free(vwalls);
        for(unsigned int m = 0; m < maze->w*maze->h; ++m){
            maze->cells[m] &= ~MAZE_CELL_WALLED_BOTTOM;
            maze->cells[m] &= ~MAZE_CELL_WALLED_LEFT;
        }
        return true;
    }
    
    void MazeOpenAllDeadEnds(maze_t* maze) {
        int directions[MAZE_DIRECTION_COUNT];
        for(int y = 0; y < maze->h; ++y) {
            for(int x = 0; x < maze->w; ++x) {
                int m = y*maze->w+x;
                if((maze->cells[m] & MAZE_LEFT ? 1:0) + (maze->cells[m] & MAZE_UP ? 1:0) + (maze->cells[m] & MAZE_RIGHT ? 1:0) + (maze->cells[m] & MAZE_DOWN ? 1:0) == 1) {
                    int the_directions[MAZE_DIRECTION_COUNT] = {
                        MAZE_LEFT, MAZE_UP, MAZE_RIGHT, MAZE_DOWN
                    };
                    int remaining_directions = MAZE_DIRECTION_COUNT;
                    for(int i = 0; i < MAZE_DIRECTION_COUNT-1; ++i) {
                        int selected_direction = rand()%remaining_directions;
                        directions[i] = the_directions[selected_direction];
                        --remaining_directions;
                        for(int j = selected_direction; j < remaining_directions; ++j) {
                            the_directions[j] = the_directions[j+1];
                        }
                    }
                    directions[MAZE_DIRECTION_COUNT-1] = the_directions[0];
                    bool connected = false;
                    for(int i = 0; i < MAZE_DIRECTION_COUNT && !connected; ++i) {
                        if(maze->cells[m] & directions[i]) continue;
                        switch(directions[i]) {
                            case MAZE_LEFT:{
                                if(x > 0) {
                                    maze->cells[m] |= MAZE_LEFT;
                                    maze->cells[y*maze->w + x-1] |= MAZE_RIGHT;
                                    connected = true;
                                }
                            }break;
                            case MAZE_RIGHT:{
                                if(x < maze->w-1) {
                                    maze->cells[m] |= MAZE_RIGHT;
                                    maze->cells[y*maze->w + x+1] |= MAZE_LEFT;
                                    connected = true;
                                }
                            }break;
                            case MAZE_DOWN:{
                                if(y > 0) {
                                    maze->cells[m] |= MAZE_DOWN;
                                    maze->cells[(y-1)*maze->w + x] |= MAZE_UP;
                                    connected = true;
                                }
                            }break;
                            case MAZE_UP:{
                                if(y < maze->h-1) {
                                    maze->cells[m] |= MAZE_UP;
                                    maze->cells[(y+1)*maze->w + x] |= MAZE_DOWN;
                                    connected = true;
                                }
                            }break;
                        }
                    }
                }
            }
        }
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_MAZE_H
#endif
