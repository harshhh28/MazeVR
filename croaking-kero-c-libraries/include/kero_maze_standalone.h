/*
kero_maze_standalone.h created by Nicholas Walton. Released to the public domain. If a license is required, licensed under MIT.

*******************************************************************************
*                               MIT LICENSE                                   *
*******************************************************************************
Copyright 2019 Nicholas Walton

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************

kero_maze_standalone requires the c standard library and no other dependencies.

*/


#ifndef KERO_MAZE_STANDALONE_H

#ifdef __cplusplus
extern "C"{
#endif
    
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
    
    typedef struct {
        int w, h, cell_size;
        struct { int x, y; }start, end;
        uint8_t* cells;
    } maze_t;
    // cells must be initialized to 0 or calling MazeFree() or any Generate() functions will segfault as they try to free() cells.
    
    typedef enum{
        MAZE_LEFT = 0b1,
        MAZE_UP = 0b10,
        MAZE_RIGHT = 0b100,
        MAZE_DOWN = 0b1000,
        MAZE_DIRECTION_COUNT = 4
    } MAZE_DIRECTION;
    
    
    
    // Function declarations
    
#define MazeGenerate MazeGeneratePersistentWalk
#define MazeFindSolution MazeDijkstra
    
    bool MazeGeneratePersistentWalk(maze_t* maze, int w, int h, int cell_size, float persistence);
    /*
    The crown jewel of this lib. Generates mazes that vary in texture based on the persistence value.
 Persistence = 0 generates very similar to Prim's or Aldous-Broder - short corridors with many forks, radial texture.
 Persistence = infinity generates very similar to recursive backtracker, except instead of starting new paths at the first possible previous cell, it selects a random cell to start the new path. Long corridors, few forks, disarrayed texture.
 Persistence = 2 seems to be a sweet spot for modest corridors with plenty of forking and an off-radial texture.
    */
    
    bool MazeGenerateShortWalk(maze_t* maze, int w, int h, int cell_size);
    /*
         Similar in texture to Prim's or Aldous-Broder.
    */
    
    bool MazeGenerateRecursiveBacktracker(maze_t* maze, int w, int h, int cell_size);
    /*
    Creates long passages with few forks.
    */
    
    int MazeDijkstra(maze_t* maze, unsigned int** weights);
    /*
    Using Dijkstra's algorithm calculate the weight of each cell (the distance from the start). Sets maze.end to the heaviest cell.
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
    
    bool MazeGeneratePersistentWalk(maze_t* maze, int w, int h, int cell_size, float persistence) {
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
        float current_persistence = 0;
        while(num_visited_cells > 0) {
            px = x;
            py = y;
            bool did_visit = false;
            if ((x > 0 && !(maze->cells[x-1+y*maze->w] & MAZE_CELL_VISITED)) || (x < maze->w-1 && !(maze->cells[x+1+y*maze->w] & MAZE_CELL_VISITED)) || (y > 0 && !(maze->cells[x+(y-1)*maze->w] & MAZE_CELL_VISITED)) || (y < maze->h-1 && !(maze->cells[x+(y+1)*maze->w] & MAZE_CELL_VISITED))) {
                current_persistence += persistence;
                do {
                    dir = rand()%4;
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
                } while(--current_persistence >= 1 && !did_visit);
                current_persistence = KMazeFMod(current_persistence, 1);
            }
            if(did_visit){
                visited_cells[num_visited_cells++] = x + y*maze->w;
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
        return true;
    }
    
    bool MazeGenerateShortWalk(maze_t* maze, int w, int h, int cell_size) {
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
        return true;
    }
    
    bool MazeGenerateRecursiveBacktracker(maze_t* maze, int w, int h, int cell_size) {
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
        int maze_stack[maze->w*maze->h];
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
        return max_weight;
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_MAZE_STANDALONE_H
#endif
