#include "kero_software_3d.h"
#include "kero_std.h"
#include "kero_math.h"
#include <math.h>
#include <assert.h>
#include "kero_platform.h"
#include "kero_matrix.h"
#include "kero_font.h"
#include "kero_maze.h"

#define CAM_SPEED 8.f
#define CAM_ROT_SPEED 0.002f
#define NUM_CUBES 0
#define NUM_CUBE_FACES 12
#define NEAR_Z 1.f
#define FAR_Z 50.f
#define MAX_FACES 100000
#define NUM_GAME_TEXTURES 11
#define MAX_TEXTURES 16
#define AI_TIME_PER_MOVE 0.5f // In seconds

struct
{
    struct
    {
        int x, z;
    } last_pos;
    float time;
} ai;

enum MENUS
{
    MENU_MAIN,
    MENU_OPTIONS,
    MENU_WIN,
    MENU_TITLE,
    NUM_MENUS
};

typedef struct
{
    vec2_t a, b, text_pos;
    uint32_t color, highlight_color;
    struct
    {
        int left, right, up, down;
    } target;
    bool toggle;
    char text[128];
    void (*func)();
} menu_item_t;

typedef struct
{
    menu_item_t items[8];
    int num_items;
    int active_item;
} menu_t;

menu_t menus[NUM_MENUS];

kero_platform_t platform;

void MenuItemMake(menu_item_t *o, int left, int top, int right, int bottom, int textx, int texty, char *text, int target_left, int target_right, int target_up, int target_down, void (*func)(), uint32_t color, uint32_t highlight_color)
{
    o->a.x = left;
    o->a.y = top;
    o->b.x = right;
    o->b.y = bottom;
    o->text_pos.x = textx;
    o->text_pos.y = texty;
    o->color = color;
    o->highlight_color = highlight_color;
    strcpy(o->text, text);
    o->toggle = false;
    o->func = func;
    o->target.left = target_left;
    o->target.right = target_right;
    o->target.up = target_up;
    o->target.down = target_down;
}

void MenuItemMakeDefaultColor(menu_item_t *o, int left, int top, int right, int bottom, int textx, int texty, char *text, int target_left, int target_right, int target_up, int target_down, void (*func)())
{
    MenuItemMake(o, left, top, right, bottom, textx, texty, text, target_left, target_right, target_up, target_down, func, 0xff0ca000, 0xffafff29);
}

int internal_resolutions[][2] = {
    {320, 240},
    {640, 480},
    {800, 600},
    {1024, 768},
    {1280, 1024},
    {320, 180},
    {640, 360},
    {1280, 720},
    {1920, 1080},
};
int num_internal_resolutions = 9;
int internal_resolution = 0;

bool draw_minimap = false;
bool draw_framerate = false;
int active_menu = MENU_TITLE;
bool auto_launch_menu = true;
bool game_running = true;
bool menu_running = true;
bool draw_profiles = false;
ksprite_t frame_buffer;
ksprite_t menu_frame;
int internal_resolution_width = 320;
int internal_resolution_height = 240;
ksprite_t render_frame;
float *depth_buffer;
float aspect_ratio;
kfont_t font;
maze_t maze;
wall_t *walls; // Stretchy buffer. DO NOT free(). Use sb_free()
ksprite_t maze_sprite;
face_t dodecahedron[36];
unsigned int maze_size = 20;
struct
{
    vec3_t pos;
    union
    {
        vec3_t rot;
        struct
        {
            float pitch, yaw, roll;
        };
    };
    bool active;
} dodecahedrons[333];
unsigned int num_dodecahedrons;
bool ai_control = false;

vec3_t cube_verts[] = {
    {1.f, 1.f, 1.f},
    {1.f, -1.f, 1.f},
    {-1.f, -1.f, 1.f},
    {-1.f, 1.f, 1.f},

    {-1.f, 1.f, -1.f},
    {-1.f, -1.f, -1.f},
    {1.f, -1.f, -1.f},
    {1.f, 1.f, -1.f},
};

typedef struct
{
    union
    {
        struct
        {
            unsigned int i0, i1, i2;
        };
        unsigned int i[3];
    };
    uint32_t c;
} face_indexed_t;

// Face = vert0,1,2, colour
face_indexed_t cube_faces[NUM_CUBE_FACES] = {
    // front
    {0, 2, 1, 0xffff0000},
    {0, 3, 2, 0xffff0000},
    // back
    {4, 6, 5, 0xff00ff00},
    {4, 7, 6, 0xff00ff00},
    // right
    {0, 1, 6, 0xff0000ff},
    {0, 6, 7, 0xff0000ff},
    // left
    {5, 2, 3, 0xff00ffff},
    {5, 3, 4, 0xff00ffff},
    // top
    {4, 3, 0, 0xffffff00},
    {4, 0, 7, 0xffffff00},
    // bottom
    {5, 1, 2, 0xffff00ff},
    {5, 6, 1, 0xffff00ff},
};

vec3_t cube_pos[] = {
    {0.f, 0.f, 0.f},
    {5.f, 0.5f, 6.f},
    {-2.f, -1.f, 13.f},
    {0.f, 0.f, -15.f},
    {5.f, 1.f, -7.f},
    {-6.f, -0.5f, -6.f},
};
vec3_t cube_rot[] = {
    {0.f, 0.f, 0.f},
    {0.f, 0.f, 0.f},
    {0.f, 0.f, 0.f},
    {0.f, 0.f, 0.f},
    {0.f, 0.f, 0.f},
    {0.f, 0.f, 0.f},
};

face_t maze_faces[MAX_FACES];
int num_maze_faces;

face_t world_faces[MAX_FACES];
int num_world_faces;
face_t cam_faces[MAX_FACES];
int num_cam_faces;
face_t view_faces[MAX_FACES];
int num_view_faces;

face_t end_board[2];

ksprite_t textures[16];

struct
{
    vec3_t pos;
    union
    {
        vec3_t rot;
        struct
        {
            float pitch, yaw, roll;
        };
    };
} cam = {
    {3.f, 2.5f, 3.f},
    {HALFPI, -HALFPI / 2.f, 0.f}};
int turn_direction = 0;
int turn_target = 0;
int roll_target = 0;
struct
{
    int x, z;
} target_pos = {0};
#define CONTROL_WASD 0b1
#define CONTROL_MOUSE 0b10
#define CONTROL_TURN90 0b100
#define CONTROL_GRID 0b1000
#define CONTROL_DOWN_MOVE 0b10000
unsigned int control_mode = CONTROL_GRID | CONTROL_TURN90;

#define MAX_PROFILE_TIMES 16
#define MAX_PROFILE_FRAMES 60
double profile_times[MAX_PROFILE_TIMES];
char profile_time_names[MAX_PROFILE_TIMES][64];
int current_profile_time = 0;
int current_profile_frame = 0;
uint32_t profile_colours[MAX_PROFILE_TIMES];
typedef struct
{
    int num_profiles;
    double profiles[MAX_PROFILE_TIMES];
} profile_frame_t;
profile_frame_t profile_frames[MAX_PROFILE_FRAMES] = {0};
profile_frame_t new_profile_frame = {0};
void ProfileTime(char *name)
{
    if (current_profile_time >= MAX_PROFILE_TIMES)
        return;
    profile_times[current_profile_time] = KP_Clock();
    strcpy(profile_time_names[current_profile_time], name);
    ++current_profile_time;
}

typedef struct
{
    char text[128];
    float time;
} timed_message_t;
timed_message_t player_messages[5];
int top_message = 0;

void SortRays(int f, int l, float *ray_angles, vec2_t *rays)
{
    int i, j, p = 0;
    float tf;
    vec2_t tr;
    if (f < l)
    {
        p = f, i = f, j = l;
        while (i < j)
        {
            while (ray_angles[i] <= ray_angles[p] && i < l)
                i++;
            while (ray_angles[j] > ray_angles[p])
                j--;
            if (i < j)
            {
                Swap(float, ray_angles[i], ray_angles[j]);
                Swap(vec2_t, rays[i], rays[j]);
            }
        }
        Swap(float, ray_angles[p], ray_angles[j]);
        Swap(vec2_t, rays[p], rays[j]);
        SortRays(f, j - 1, ray_angles, rays);
        SortRays(j + 1, l, ray_angles, rays);
    }
}

static inline void PlayerMessage(char *text)
{
    if (top_message > 4)
    {
        for (int i = 0; i < 4; ++i)
        {
            player_messages[i].time = player_messages[i + 1].time;
            for (int j = 0; j < 127 && (player_messages[i].text[j] = player_messages[i + 1].text[j]); ++j)
                ;
        }
        --top_message;
    }
    for (int i = 0; i < 127 && (player_messages[top_message].text[i] = text[i]) != '\0'; ++i)
        ;
    player_messages[top_message].text[127] = '0';
    player_messages[top_message].time = 5;
    ++top_message;
}

static inline void ResizeRenderFrame(int w, int h)
{
    if (w <= 0 || h <= 0)
    {
        PlayerMessage("Invalid resolution");
        return;
    }
    internal_resolution_width = w;
    internal_resolution_height = h;
    render_frame.w = internal_resolution_width;
    render_frame.h = internal_resolution_height;
    render_frame.pixels = (uint32_t *)realloc(render_frame.pixels, sizeof(uint32_t) * internal_resolution_width * internal_resolution_height);
    aspect_ratio = (float)internal_resolution_width / (float)internal_resolution_height;
    depth_buffer = (float *)realloc(depth_buffer, sizeof(float) * internal_resolution_width * internal_resolution_height);
    KS_Clear(&frame_buffer);
}

void RestartMaze()
{
    ai_control = false;
    MazeFree(&maze);
    MazeGeneratePersistentWalk(&maze, &walls, maze_size, maze_size, 5, 2, NULL, &platform);
    unsigned int *weights;
    MazeFindSolution(&maze, &weights);
    free(weights);
    // Put player at start, set end pos for kero sprite
    cam.pos.x = (float)maze.start.x * maze.cell_size + maze.cell_size / 2;
    cam.pos.z = (float)maze.start.y * maze.cell_size + maze.cell_size / 2;
    target_pos.x = maze.start.x;
    target_pos.z = maze.start.y;
    end_board[0].v0.x = -2.f, end_board[0].v0.y = 2.f, end_board[0].v0.z = 0.f;
    end_board[0].v1.x = 2.f, end_board[0].v1.y = -2.f, end_board[0].v1.z = 0.f;
    end_board[0].v2.x = -2.f, end_board[0].v2.y = -2.f, end_board[0].v2.z = 0.f;
    end_board[0].flags.double_sided = false;
    end_board[0].uv[0].u = 0, end_board[0].uv[1].u = 1, end_board[0].uv[2].u = 0;
    end_board[0].uv[0].v = 0, end_board[0].uv[1].v = 1, end_board[0].uv[2].v = 1;
    end_board[0].texture_index = 15;
    end_board[1].v0.x = -2.f, end_board[1].v0.y = 2.f, end_board[1].v0.z = 0.f;
    end_board[1].v1.x = 2.f, end_board[1].v1.y = 2.f, end_board[1].v1.z = 0.f;
    end_board[1].v2.x = 2.f, end_board[1].v2.y = -2.f, end_board[1].v2.z = 0.f;
    end_board[1].flags.double_sided = false;
    end_board[1].uv[0].u = 0, end_board[1].uv[1].u = 1, end_board[1].uv[2].u = 1;
    end_board[1].uv[0].v = 0, end_board[1].uv[1].v = 0, end_board[1].uv[2].v = 1;
    end_board[1].texture_index = 15;
    KS_Create(&maze_sprite, maze.w * maze.cell_size + 1, maze.h * maze.cell_size + 1);
    KS_SetAllPixels(&maze_sprite, 0xff880088);
    for (int y = 0; y < maze_sprite.h * maze.cell_size; y += maze.cell_size)
    {
        int x = (y / maze.cell_size) % 2 * maze.cell_size;
        for (; x < maze_sprite.w * maze.cell_size; x += maze.cell_size * 2)
        {
            KS_DrawRectFilled(&maze_sprite, x, y, x + maze.cell_size, y + maze.cell_size, 0xff0000bb);
        }
    }
    KS_DrawRectFilled(&maze_sprite, maze.end.x * maze.cell_size + 1, maze_sprite.h - maze.end.y * maze.cell_size - 1, (maze.end.x + 1) * maze.cell_size - 1, maze_sprite.h - (maze.end.y + 1) * maze.cell_size - 1, 0xff00ff00);
    for (int wall = 0; wall < sb_count(walls); ++wall)
    {
        KS_DrawRectFilled(&maze_sprite, walls[wall].a.x, maze_sprite.h - walls[wall].a.y - 1, walls[wall].b.x, maze_sprite.h - walls[wall].b.y - 1, 0xffffffff);
    }

    num_dodecahedrons = Max(1, Square(maze_size) / 30.f);
    for (int i = 0; i < num_dodecahedrons; ++i)
    {
        dodecahedrons[i].pos.y = 2.5f;
        dodecahedrons[i].active = true;
        bool conflict;
        do
        {
            dodecahedrons[i].pos.x = (rand() % maze.w) * maze.cell_size + 0.5f * maze.cell_size;
            dodecahedrons[i].pos.z = (rand() % maze.h) * maze.cell_size + 0.5f * maze.cell_size;
            conflict = false;
            if (dodecahedrons[i].pos.x == maze.end.x * maze.cell_size + 0.5f * maze.cell_size && dodecahedrons[i].pos.z == maze.end.y * maze.cell_size + 0.5f * maze.cell_size)
            {
                conflict = true;
            }
            else if (dodecahedrons[i].pos.x == maze.start.x * maze.cell_size + 0.5f * maze.cell_size && dodecahedrons[i].pos.z == maze.start.y * maze.cell_size + 0.5f * maze.cell_size)
            {
                conflict = true;
            }
            else
            {
                for (int j = 0; j < i && !conflict; ++j)
                {
                    if (
                        dodecahedrons[i].pos.x == dodecahedrons[j].pos.x && dodecahedrons[i].pos.z == dodecahedrons[j].pos.z)
                    {
                        conflict = true;
                    }
                }
            }
        } while (conflict);
    }

    {
        int maze_face_it = 0;
        uint32_t color = 0xff225588;
        uint8_t wall_alpha = 255;
        const float offset = 0.3f;

        // Floor
        maze_faces[maze_face_it].v0 = Vec3Make(0, 0, 0);
        maze_faces[maze_face_it].v1 = Vec3Make(0, 0, maze.h * maze.cell_size);
        maze_faces[maze_face_it].v2 = Vec3Make(maze.w * maze.cell_size, 0, 0);
        maze_faces[maze_face_it].c = color;
        maze_faces[maze_face_it].uv[0].u = 0;
        maze_faces[maze_face_it].uv[0].v = 0;
        maze_faces[maze_face_it].uv[1].u = maze.h;
        maze_faces[maze_face_it].uv[1].v = 0;
        maze_faces[maze_face_it].uv[2].u = 0;
        maze_faces[maze_face_it].uv[2].v = maze.w;
        maze_faces[maze_face_it].flags.double_sided = true;
        maze_faces[maze_face_it].texture_index = 0;
        ++maze_face_it;
        maze_faces[maze_face_it].v0 = Vec3Make(maze.w * maze.cell_size, 0, maze.h * maze.cell_size);
        maze_faces[maze_face_it].v1 = Vec3Make(0, 0, maze.h * maze.cell_size);
        maze_faces[maze_face_it].v2 = Vec3Make(maze.w * maze.cell_size, 0, 0);
        maze_faces[maze_face_it].c = color;
        maze_faces[maze_face_it].uv[0].u = maze.w;
        maze_faces[maze_face_it].uv[0].v = maze.h;
        maze_faces[maze_face_it].uv[1].u = 0;
        maze_faces[maze_face_it].uv[1].v = maze.h;
        maze_faces[maze_face_it].uv[2].u = maze.w;
        maze_faces[maze_face_it].uv[2].v = 0;
        maze_faces[maze_face_it].flags.double_sided = true;
        maze_faces[maze_face_it].texture_index = 0;
        ++maze_face_it;

        // Ceiling
        maze_faces[maze_face_it].v0 = Vec3Make(0, 5, 0);
        maze_faces[maze_face_it].v1 = Vec3Make(0, 5, maze.h * maze.cell_size);
        maze_faces[maze_face_it].v2 = Vec3Make(maze.w * maze.cell_size, 5, 0);
        maze_faces[maze_face_it].c = color;
        maze_faces[maze_face_it].uv[0].u = 0;
        maze_faces[maze_face_it].uv[0].v = 0;
        maze_faces[maze_face_it].uv[1].u = maze.h;
        maze_faces[maze_face_it].uv[1].v = 0;
        maze_faces[maze_face_it].uv[2].u = 0;
        maze_faces[maze_face_it].uv[2].v = maze.w;
        maze_faces[maze_face_it].flags.double_sided = true;
        maze_faces[maze_face_it].texture_index = 2;
        ++maze_face_it;
        maze_faces[maze_face_it].v0 = Vec3Make(maze.w * maze.cell_size, 5, maze.h * maze.cell_size);
        maze_faces[maze_face_it].v1 = Vec3Make(0, 5, maze.h * maze.cell_size);
        maze_faces[maze_face_it].v2 = Vec3Make(maze.w * maze.cell_size, 5, 0);
        maze_faces[maze_face_it].c = color;
        maze_faces[maze_face_it].uv[0].u = maze.w;
        maze_faces[maze_face_it].uv[0].v = maze.h;
        maze_faces[maze_face_it].uv[1].u = 0;
        maze_faces[maze_face_it].uv[1].v = maze.h;
        maze_faces[maze_face_it].uv[2].u = maze.w;
        maze_faces[maze_face_it].uv[2].v = 0;
        maze_faces[maze_face_it].flags.double_sided = true;
        maze_faces[maze_face_it].texture_index = 2;
        ++maze_face_it;

        for (int wall_it = 0; wall_it < sb_count(walls); ++wall_it)
        {
            color = rand() % 16581375 + (wall_alpha << 24);
            if (walls[wall_it].a.y != walls[wall_it].b.y)
            {
                float tex_length = (walls[wall_it].b.y - walls[wall_it].a.y) / 5.f;
                if (Absolute(walls[wall_it].b.y - walls[wall_it].a.y) == maze.cell_size && !(rand() % 8))
                {
                    tex_length = 1;
                    maze_faces[maze_face_it].texture_index = maze_faces[maze_face_it + 1].texture_index = rand() % NUM_GAME_TEXTURES + 3;
                }
                else
                {
                    maze_faces[maze_face_it].texture_index = maze_faces[maze_face_it + 1].texture_index = 1;
                }
                maze_faces[maze_face_it].v0 = Vec3Make(walls[wall_it].b.x, 0, walls[wall_it].b.y);
                maze_faces[maze_face_it].v1 = Vec3Make(walls[wall_it].a.x, 0, walls[wall_it].a.y);
                maze_faces[maze_face_it].v2 = Vec3Make(walls[wall_it].a.x, 5, walls[wall_it].a.y);
                maze_faces[maze_face_it].c = color;
                maze_faces[maze_face_it].uv[0].u = 0;
                maze_faces[maze_face_it].uv[0].v = 1;
                maze_faces[maze_face_it].uv[1].u = tex_length;
                maze_faces[maze_face_it].uv[1].v = 1;
                maze_faces[maze_face_it].uv[2].u = tex_length;
                maze_faces[maze_face_it].uv[2].v = 0;
                maze_faces[maze_face_it].flags.double_sided = true;
                ++maze_face_it;
                maze_faces[maze_face_it].v0 = Vec3Make(walls[wall_it].b.x, 0, walls[wall_it].b.y);
                maze_faces[maze_face_it].v1 = Vec3Make(walls[wall_it].a.x, 5, walls[wall_it].a.y);
                maze_faces[maze_face_it].v2 = Vec3Make(walls[wall_it].b.x, 5, walls[wall_it].b.y);
                maze_faces[maze_face_it].c = color;
                maze_faces[maze_face_it].uv[0].u = 0;
                maze_faces[maze_face_it].uv[0].v = 1;
                maze_faces[maze_face_it].uv[1].u = tex_length;
                maze_faces[maze_face_it].uv[1].v = 0;
                maze_faces[maze_face_it].uv[2].u = 0;
                maze_faces[maze_face_it].uv[2].v = 0;
                maze_faces[maze_face_it].flags.double_sided = true;
                ++maze_face_it;
            }
            else
            {
                float tex_length = (walls[wall_it].b.x - walls[wall_it].a.x) / 5.f;
                if (Absolute(walls[wall_it].b.x - walls[wall_it].a.x) == maze.cell_size && !(rand() % 8))
                {
                    tex_length = 1;
                    maze_faces[maze_face_it].texture_index = maze_faces[maze_face_it + 1].texture_index = rand() % NUM_GAME_TEXTURES + 3;
                }
                else
                {
                    maze_faces[maze_face_it].texture_index = maze_faces[maze_face_it + 1].texture_index = 1;
                }
                maze_faces[maze_face_it].v0 = Vec3Make(walls[wall_it].a.x, 0, walls[wall_it].a.y);
                maze_faces[maze_face_it].v1 = Vec3Make(walls[wall_it].b.x, 0, walls[wall_it].b.y);
                maze_faces[maze_face_it].v2 = Vec3Make(walls[wall_it].a.x, 5, walls[wall_it].a.y);
                maze_faces[maze_face_it].c = color;
                maze_faces[maze_face_it].uv[0].u = tex_length;
                maze_faces[maze_face_it].uv[0].v = 1;
                maze_faces[maze_face_it].uv[1].u = 0;
                maze_faces[maze_face_it].uv[1].v = 1;
                maze_faces[maze_face_it].uv[2].u = tex_length;
                maze_faces[maze_face_it].uv[2].v = 0;
                maze_faces[maze_face_it].flags.double_sided = true;
                ++maze_face_it;
                maze_faces[maze_face_it].v0 = Vec3Make(walls[wall_it].a.x, 5, walls[wall_it].a.y);
                maze_faces[maze_face_it].v1 = Vec3Make(walls[wall_it].b.x, 0, walls[wall_it].b.y);
                maze_faces[maze_face_it].v2 = Vec3Make(walls[wall_it].b.x, 5, walls[wall_it].b.y);
                maze_faces[maze_face_it].c = color;
                maze_faces[maze_face_it].uv[0].u = tex_length;
                maze_faces[maze_face_it].uv[0].v = 0;
                maze_faces[maze_face_it].uv[1].u = 0;
                maze_faces[maze_face_it].uv[1].v = 1;
                maze_faces[maze_face_it].uv[2].u = 0;
                maze_faces[maze_face_it].uv[2].v = 0;
                maze_faces[maze_face_it].flags.double_sided = true;
                ++maze_face_it;
            }
        }
        // End ceiling
        num_maze_faces = maze_face_it;
    }

    menu_running = false;
    roll_target = 0;

    // Randomize dodecahedron face colours
    for (int i = 0; i < 12; ++i)
    {
        dodecahedron[i * 3].c = (rand() % (255));
        dodecahedron[i * 3].c = (dodecahedron[i * 3].c << 16) + (dodecahedron[i * 3].c << 8) + (dodecahedron[i * 3].c) + (255 << 24);
        for (int j = i * 3 + 1; j < i * 3 + 3; ++j)
        {
            dodecahedron[j].c = dodecahedron[i * 3].c;
        }
    }
}

static inline void MainMenuPlay()
{
    menu_running = false;
    KP_ShowCursor(&platform, false);
}

static inline void MainMenuOptions()
{
    active_menu = MENU_OPTIONS;
}

static inline void MenuToMain()
{
    active_menu = MENU_MAIN;
}

static inline void QuitGame()
{
    exit(0);
}

static inline void ToggleFullscreen()
{
    KP_Fullscreen(&platform, !platform.fullscreen);
    menus[MENU_OPTIONS].items[0].toggle = platform.fullscreen;
}

static inline void ResetInternalResolution()
{
    internal_resolution = 0;
    auto_launch_menu = true;
    ResizeRenderFrame(internal_resolutions[internal_resolution][0], internal_resolutions[internal_resolution][1]);
    MainMenuPlay();
}

static inline void InternalResolutionDown()
{
    if (--internal_resolution < 0)
    {
        internal_resolution = 0;
        return;
    }
    auto_launch_menu = true;
    ResizeRenderFrame(internal_resolutions[internal_resolution][0], internal_resolutions[internal_resolution][1]);
    MainMenuPlay();
}

static inline void InternalResolutionUp()
{
    if (++internal_resolution > num_internal_resolutions - 1)
    {
        internal_resolution = num_internal_resolutions - 1;
        return;
    }
    auto_launch_menu = true;
    ResizeRenderFrame(internal_resolutions[internal_resolution][0], internal_resolutions[internal_resolution][1]);
    MainMenuPlay();
}

static inline void ResetMazeSize()
{
    maze_size = 30;
    sprintf((char *)&menus[MENU_OPTIONS].items[5].text, "Maze size\n   %3d", maze_size);
    RestartMaze();
    auto_launch_menu = true;
}

static inline void MazeSizeDown()
{
    if (maze_size > 5)
    {
        --maze_size;
        sprintf((char *)&menus[MENU_OPTIONS].items[5].text, "Maze size\n   %3d", maze_size);
        RestartMaze();
        auto_launch_menu = true;
    }
}

static inline void MazeSizeUp()
{
    if (maze_size < 100)
    {
        ++maze_size;
        sprintf((char *)&menus[MENU_OPTIONS].items[5].text, "Maze size\n   %3d", maze_size);
        RestartMaze();
        auto_launch_menu = true;
    }
}

static inline void OtherGames()
{
    KP_OpenURL("http://www.croakingkero.com/recommendations/maze95");
}

static inline void NilFunc() {}

static inline void TitleStart()
{
    active_menu = MENU_MAIN;
    auto_launch_menu = true;
}

void Menu()
{
    KP_ShowCursor(&platform, true);
    menu_running = true;
    auto_launch_menu = false;
    vec2_t mouse_pos;

    while (menu_running)
    {
        while (KP_EventsQueued(&platform))
        {
            kp_event_t *e = KP_NextEvent(&platform);
            switch (e->type)
            {
            case KP_EVENT_KEY_PRESS:
            {
                if (active_menu == MENU_TITLE)
                {
                    TitleStart();
                    return;
                }
                switch (e->key)
                {
                case KEY_ESCAPE:
                {
                    if (active_menu == MENU_MAIN)
                        MainMenuPlay();
                    active_menu = MENU_MAIN;
                }
                break;
                case KEY_ENTER:
                {
                    if (platform.keyboard[KEY_RALT] || platform.keyboard[KEY_LALT])
                    {
                        ToggleFullscreen();
                    }
                    else
                    {
                        menus[active_menu].items[menus[active_menu].active_item].func();
                    }
                }
                break;
                case KEY_LEFT:
                case KEY_A:
                {
                    if (menus[active_menu].items[menus[active_menu].active_item].target.left != -1)
                    {
                        menus[active_menu].active_item = menus[active_menu].items[menus[active_menu].active_item].target.left;
                    }
                }
                break;
                case KEY_RIGHT:
                case KEY_D:
                {
                    if (menus[active_menu].items[menus[active_menu].active_item].target.right != -1)
                    {
                        menus[active_menu].active_item = menus[active_menu].items[menus[active_menu].active_item].target.right;
                    }
                }
                break;
                case KEY_UP:
                case KEY_W:
                {
                    if (menus[active_menu].items[menus[active_menu].active_item].target.up != -1)
                    {
                        menus[active_menu].active_item = menus[active_menu].items[menus[active_menu].active_item].target.up;
                    }
                }
                break;
                case KEY_DOWN:
                case KEY_S:
                {
                    if (menus[active_menu].items[menus[active_menu].active_item].target.down != -1)
                    {
                        menus[active_menu].active_item = menus[active_menu].items[menus[active_menu].active_item].target.down;
                    }
                }
                break;
                }
            }
            break;
            case KP_EVENT_MOUSE_BUTTON_PRESS:
            {
                if (active_menu == MENU_TITLE)
                {
                    TitleStart();
                    return;
                }
                switch (e->button)
                {
                case MOUSE_LEFT:
                {
                    for (int i = 0; i < menus[active_menu].num_items; ++i)
                    {
                        if (mouse_pos.x > menus[active_menu].items[i].a.x && mouse_pos.y > menus[active_menu].items[i].a.y && mouse_pos.x < menus[active_menu].items[i].b.x && mouse_pos.y < menus[active_menu].items[i].b.y)
                        {
                            menus[active_menu].items[i].func();
                            break;
                        }
                    }
                }
                break;
                }
            }
            break;
            case KP_EVENT_RESIZE:
            {
                frame_buffer.pixels = platform.frame_buffer.pixels;
                frame_buffer.w = platform.frame_buffer.w;
                frame_buffer.h = platform.frame_buffer.h;
                KS_SetAllPixels(&frame_buffer, 0x00000000);
            }
            break;
            case KP_EVENT_QUIT:
            {
                exit(0);
            }
            break;
            }
            KP_FreeEvent(e);
        }

        float frame_scale = Min((float)frame_buffer.h / (float)menu_frame.h, (float)frame_buffer.w / (float)menu_frame.w);
        vec2_t prev_mouse_pos = mouse_pos;
        mouse_pos.x = (platform.mouse.x - (frame_buffer.w - menu_frame.w * frame_scale) / 2) / frame_scale;
        mouse_pos.y = (platform.mouse.y - (frame_buffer.h - menu_frame.h * frame_scale) / 2) / frame_scale;
        bool mouse_moved = !Vec2Equals(mouse_pos, prev_mouse_pos);

        KS_Clear(&menu_frame);
        float game_scale = Min((float)frame_buffer.h / (float)render_frame.h, (float)frame_buffer.w / (float)render_frame.w);
        KS_BlitScaled(&render_frame, &frame_buffer, frame_buffer.w / 2, frame_buffer.h / 2, game_scale, game_scale, render_frame.w / 2, render_frame.h / 2);

        for (int i = 0; i < menus[active_menu].num_items; ++i)
        {
            uint32_t rect_col = menus[active_menu].items[i].color;
            if (mouse_moved && menus[active_menu].items[i].color != menus[active_menu].items[i].highlight_color && mouse_pos.x > menus[active_menu].items[i].a.x && mouse_pos.y > menus[active_menu].items[i].a.y && mouse_pos.x < menus[active_menu].items[i].b.x && mouse_pos.y < menus[active_menu].items[i].b.y)
            {
                menus[active_menu].active_item = i;
            }
            if (menus[active_menu].items[i].toggle || i == menus[active_menu].active_item)
            {
                rect_col = menus[active_menu].items[i].highlight_color;
            }
            KS_DrawRectFilled(&menu_frame, menus[active_menu].items[i].a.x, menus[active_menu].items[i].a.y, menus[active_menu].items[i].b.x, menus[active_menu].items[i].b.y, rect_col);
            KF_DrawAlpha10(&font, &menu_frame, menus[active_menu].items[i].text_pos.x, menus[active_menu].items[i].text_pos.y, menus[active_menu].items[i].text);
        }

        for (int i = 0; i < top_message; ++i)
        {
            player_messages[i].time -= platform.delta;
            if (player_messages[i].time < 0)
            {
                --top_message;
                for (int j = i; j < top_message; ++j)
                {
                    player_messages[j].time = player_messages[j + 1].time;
                    for (int k = 0; k < 127 && (player_messages[j].text[k] = player_messages[j + 1].text[k]); ++k)
                        ;
                }
            }
            else
            {
                KF_Draw(&font, &menu_frame, 0, i * 16, player_messages[i].text);
            }
        }

        KS_BlitScaledAlpha10(&menu_frame, &frame_buffer, frame_buffer.w / 2, frame_buffer.h / 2, frame_scale, frame_scale, menu_frame.w / 2, menu_frame.h / 2);

        if (active_menu == MENU_TITLE)
        {
            KS_BlitScaledAlpha10(&textures[14], &frame_buffer, frame_buffer.w / 2, frame_buffer.h / 2 - 80 * frame_scale, frame_scale, frame_scale, textures[14].w / 2, textures[14].h / 2);
            KS_BlitScaledAlpha10(&textures[15], &frame_buffer, frame_buffer.w / 2, frame_buffer.h / 2, frame_scale * 2.f, frame_scale * 2.f, textures[15].w / 2, 0);
        }

        KP_Flip(&platform);
    }
}

static inline void Win()
{
    active_menu = MENU_WIN;
    Menu();
}

void AITurnRight()
{
    printf("AI Right\n");
    cam.pitch = HALFPI;
    if (++turn_target > 4)
    {
        turn_target = 1;
        cam.yaw = 0.f;
    }
}

void AITurnFrom(int previous)
{
    if (previous == 0 && turn_target == 3)
    {
        cam.yaw = TWOPI;
    }
    else if (previous == 3 && turn_target == 0)
    {
        cam.yaw = -HALFPI;
    }
}

void AILoop()
{
    uint8_t MAZE_CELL_VISITED = 0b10000000;
    for (int i = 0; i < maze.w * maze.h; ++i)
    {
        maze.cells[i] &= ~MAZE_CELL_VISITED;
    }
    int *maze_stack = malloc(sizeof(int) * maze.w * maze.h);
    int maze_stack_top = 0;
    maze_stack[0] = target_pos.x + target_pos.z * maze.w;
    maze.cells[target_pos.z * maze.w + target_pos.x] |= MAZE_CELL_VISITED;
    int active_roll_target = roll_target;
    while (ai_control && game_running)
    {
        while (KP_EventsQueued(&platform))
        {
            kp_event_t *e = KP_NextEvent(&platform);
            switch (e->type)
            {
            case KP_EVENT_KEY_PRESS:
            {
                switch (e->key)
                {
                case KEY_ENTER:
                {
                    if (platform.keyboard[KEY_RALT] || platform.keyboard[KEY_LALT])
                    {
                        ToggleFullscreen();
                    }
                }
                break;
                case KEY_ESCAPE:
                {
                    active_menu = MENU_MAIN;
                    Menu();
                    continue;
                }
                break;
                }
            }
            break;
            case KP_EVENT_RESIZE:
            {
                frame_buffer.pixels = platform.frame_buffer.pixels;
                frame_buffer.w = platform.frame_buffer.w;
                frame_buffer.h = platform.frame_buffer.h;
                KS_SetAllPixels(&frame_buffer, 0x00000000);
            }
            break;
            case KP_EVENT_QUIT:
            {
                exit(0);
            }
            break;
            }
            KP_FreeEvent(e);
        }

        if (Absolute(target_pos.x * maze.cell_size + maze.cell_size / 2.f - cam.pos.x) < 0.1f && Absolute(target_pos.z * maze.cell_size + maze.cell_size / 2.f - cam.pos.z) < 0.1f)
        {
            if (target_pos.x == maze.end.x && target_pos.z == maze.end.y)
            {
                // Win
                ai_control = false;
                menus[MENU_MAIN].items[3].toggle = ai_control;
                free(maze_stack);
                Win();
                return;
            }
            ai.time += AI_TIME_PER_MOVE;
            bool moved = false;
            float siny = sin((float)turn_target * HALFPI);
            float cosy = cos((float)turn_target * HALFPI);
            float rsiny = sin((float)(turn_target - 1) * HALFPI);
            float rcosy = cos((float)(turn_target - 1) * HALFPI);

            int dx = Absolute(cosy) > 0.1f ? Sign(cosy) : 0;
            int dz = Absolute(siny) > 0.1f ? Sign(siny) : 0;
            int rdx = Absolute(rcosy) > 0.1f ? Sign(rcosy) : 0;
            int rdz = Absolute(rsiny) > 0.1f ? Sign(rsiny) : 0;

            int current_cell = target_pos.x + target_pos.z * maze.w;
            int right_cell = target_pos.x + 1 + target_pos.z * maze.w;
            int forward_cell = target_pos.x + (target_pos.z + 1) * maze.w;
            int left_cell = target_pos.x - 1 + target_pos.z * maze.w;
            int back_cell = target_pos.x + (target_pos.z - 1) * maze.w;

            bool cango_right = target_pos.x < maze.w - 1 && maze.cells[current_cell] & MAZE_RIGHT;
            bool cango_left = target_pos.x > 0 && maze.cells[current_cell] & MAZE_LEFT;
            bool cango_forward = target_pos.z < maze.h - 1 && maze.cells[current_cell] & MAZE_UP;
            bool cango_back = target_pos.z > 0 && maze.cells[current_cell] & MAZE_DOWN;

            int initial_turn_target = turn_target;
            for (int direction_attempts = 0; !moved && direction_attempts < 4; ++direction_attempts)
            {
                turn_target = (initial_turn_target + direction_attempts) % 4;
                switch (turn_target)
                {
                case 0:
                { // Right
                    if (cango_back && !(maze.cells[back_cell] & MAZE_CELL_VISITED))
                    {
                        moved = true;
                        --target_pos.z;
                        maze.cells[back_cell] |= MAZE_CELL_VISITED;
                        turn_target = 1;
                    }
                    else if (cango_right && !(maze.cells[right_cell] & MAZE_CELL_VISITED))
                    {
                        moved = true;
                        ++target_pos.x;
                        maze.cells[right_cell] |= MAZE_CELL_VISITED;
                    }
                }
                break;
                case 1:
                { // Back
                    if (cango_left && !(maze.cells[left_cell] & MAZE_CELL_VISITED))
                    {
                        moved = true;
                        --target_pos.x;
                        maze.cells[left_cell] |= MAZE_CELL_VISITED;
                        turn_target = 2;
                    }
                    else if (cango_back && !(maze.cells[back_cell] & MAZE_CELL_VISITED))
                    {
                        moved = true;
                        --target_pos.z;
                        maze.cells[back_cell] |= MAZE_CELL_VISITED;
                    }
                }
                break;
                case 2:
                { // Left
                    if (cango_forward && !(maze.cells[forward_cell] & MAZE_CELL_VISITED))
                    {
                        moved = true;
                        ++target_pos.z;
                        maze.cells[forward_cell] |= MAZE_CELL_VISITED;
                        turn_target = 3;
                    }
                    else if (cango_left && !(maze.cells[left_cell] & MAZE_CELL_VISITED))
                    {
                        moved = true;
                        --target_pos.x;
                        maze.cells[left_cell] |= MAZE_CELL_VISITED;
                    }
                }
                break;
                case 3:
                { // Forward
                    if (cango_right && !(maze.cells[right_cell] & MAZE_CELL_VISITED))
                    {
                        moved = true;
                        ++target_pos.x;
                        maze.cells[right_cell] |= MAZE_CELL_VISITED;
                        turn_target = 0;
                    }
                    else if (cango_forward && !(maze.cells[forward_cell] & MAZE_CELL_VISITED))
                    {
                        moved = true;
                        ++target_pos.z;
                        maze.cells[forward_cell] |= MAZE_CELL_VISITED;
                    }
                }
                break;
                }
            }
            if (moved)
            {
                int new_cell = target_pos.x + target_pos.z * maze.w;
                maze_stack[++maze_stack_top] = new_cell;
            }
            else
            {
                --maze_stack_top;
                int oldx = target_pos.x;
                int oldz = target_pos.z;
                target_pos.z = maze_stack[maze_stack_top] / maze.w;
                target_pos.x = maze_stack[maze_stack_top] - target_pos.z * maze.w;
                if (oldx < target_pos.x)
                {
                    turn_target = turn_target = 0;
                }
                else if (oldx > target_pos.x)
                {
                    turn_target = turn_target = 2;
                }
                else if (oldz < target_pos.z)
                {
                    turn_target = turn_target = 3;
                }
                else if (oldz > target_pos.z)
                {
                    turn_target = turn_target = 1;
                }
            }
            AITurnFrom(initial_turn_target);
        }

#define AI_LERP 10.f

        int ai_final_lerp = AI_LERP;
        if (platform.keyboard[KEY_LSHIFT] || platform.keyboard[KEY_RSHIFT])
        {
            ai_final_lerp = 20.f;
        }

        float yaw_dist = Absolute((float)(turn_target % 4) * HALFPI - cam.yaw);
        if (yaw_dist > 0.00001f)
        {
            cam.yaw = Lerp(cam.yaw, (float)turn_target * HALFPI, Min(1.f, platform.delta * ai_final_lerp));
        }
        if (yaw_dist < 0.1f)
        {
            cam.pos.x = Lerp(cam.pos.x, target_pos.x * maze.cell_size + maze.cell_size / 2.f, Min(1.f, platform.delta * ai_final_lerp));
            cam.pos.z = Lerp(cam.pos.z, target_pos.z * maze.cell_size + maze.cell_size / 2.f, Min(1.f, platform.delta * ai_final_lerp));
            active_roll_target = roll_target;
            for (int i = 0; i < num_dodecahedrons; ++i)
            {
                bool hit = false;
                if (control_mode & CONTROL_GRID)
                {
                    if (dodecahedrons[i].active && Vec2Length(Vec2Sub(Vec2Make(target_pos.x * maze.cell_size + maze.cell_size / 2.f, target_pos.z * maze.cell_size + maze.cell_size / 2.f), Vec2Make(dodecahedrons[i].pos.x, dodecahedrons[i].pos.z))) < 0.5f)
                    {
                        roll_target = (roll_target + 1) % 2;
                        dodecahedrons[i].active = false;
                    }
                }
            }
        }
        if (Absolute((float)(active_roll_target % 2) * PI - cam.roll) > 0.00001f)
        {
            cam.roll = Lerp(cam.roll, (float)active_roll_target * PI, Min(1.f, platform.delta * ai_final_lerp));
        }

        if (Absolute(cam.yaw) > TWOPI)
        {
            cam.yaw -= Sign(cam.yaw) * TWOPI;
        }
        if (cam.pitch < 0.f)
        {
            cam.pitch = 0.f;
        }
        else if (cam.pitch > PI)
        {
            cam.pitch = PI;
        }
        if (Absolute(cam.roll) > TWOPI)
        {
            cam.roll -= Sign(cam.roll) * TWOPI;
        }

        for (int i = 0; i < num_dodecahedrons; ++i)
        {
            dodecahedrons[i].rot.V[i % 3] += platform.delta;
        }

        KS_Clear(&frame_buffer);
        memset(depth_buffer, 0, internal_resolution_width * internal_resolution_height * sizeof(float));

        int world_it = 0;
        // Transform dodecahedron faces to world
        for (int dodec_it = 0; dodec_it < num_dodecahedrons; ++dodec_it)
        {
            if (!dodecahedrons[dodec_it].active)
                continue;
            for (int f = 0; f < 36; ++f)
            {
                world_faces[world_it] = K3D_TranslateRotate(dodecahedron[f], dodecahedrons[dodec_it].pos, dodecahedrons[dodec_it].rot);
                ++world_it;
            }
        }
        // Transform end board faces to world
        for (int f = 0; f < 2; ++f)
        {
            if (end_board[f].texture_index > 15)
            {
                printf("end_board[%d].texture_index = %d\n", f, end_board[f].texture_index);
                exit(-1);
            }
            world_faces[world_it] = K3D_TranslateRotate(end_board[f], Vec3Make((float)maze.end.x * maze.cell_size + maze.cell_size / 2, 2.5f, (float)maze.end.y * maze.cell_size + maze.cell_size / 2), Vec3Make(cam.pitch, cam.yaw + PI, 0));
            ++world_it;
        }
        // Copy maze faces to world faces
        // Copy all maze faces
        for (int maze_it = 0; maze_it < num_maze_faces; ++maze_it)
        {
            if (maze_faces[maze_it].texture_index > 15)
            {
                printf("maze_faces[%d].texture_index = %d\n", maze_it, maze_faces[maze_it].texture_index);
                exit(-1);
            }
            world_faces[world_it].v0 = maze_faces[maze_it].v0;
            world_faces[world_it].v1 = maze_faces[maze_it].v1;
            world_faces[world_it].v2 = maze_faces[maze_it].v2;
            world_faces[world_it].c = maze_faces[maze_it].c;
            world_faces[world_it].flags = maze_faces[maze_it].flags;
            world_faces[world_it].texture_index = maze_faces[maze_it].texture_index;
            world_faces[world_it].uv[0].u = maze_faces[maze_it].uv[0].u;
            world_faces[world_it].uv[1].u = maze_faces[maze_it].uv[1].u;
            world_faces[world_it].uv[2].u = maze_faces[maze_it].uv[2].u;
            world_faces[world_it].uv[0].v = maze_faces[maze_it].uv[0].v;
            world_faces[world_it].uv[1].v = maze_faces[maze_it].uv[1].v;
            world_faces[world_it].uv[2].v = maze_faces[maze_it].uv[2].v;
            ++world_it;
        }
        num_world_faces = world_it;
        ProfileTime("Object->World");

        // transform world faces to view space
        int cam_it = world_it = 0;
        face_t world_face_trans, world_face_rot, world_face_clipped[2], temp;
        vec3_t v[4];
        int num_verts_to_clip, verts_to_clip;
        for (; world_it < num_world_faces; ++world_it)
        {
            if (!world_faces[world_it].flags.double_sided)
            {
                // back-face culling
                vec3_t n = Vec3Norm(Vec3Cross(Vec3AToB(world_faces[world_it].v0, world_faces[world_it].v1), Vec3AToB(world_faces[world_it].v0, world_faces[world_it].v2)));
                vec3_t poly_to_cam = Vec3AToB(world_faces[world_it].v0, cam.pos);
                float angle = Vec3Dot(n, poly_to_cam);
                if (angle <= 0)
                    continue;
            }
            world_face_rot = K3D_CameraTranslateRotate(world_faces[world_it], cam.pos, cam.rot);

            if (world_face_rot.v0.z < NEAR_Z && world_face_rot.v1.z < NEAR_Z && world_face_rot.v2.z < NEAR_Z)
                continue;
            // clip on NEAR_Z plane
            num_verts_to_clip = verts_to_clip = 0;
            if (world_face_rot.v0.z < NEAR_Z)
            {
                ++num_verts_to_clip;
                verts_to_clip |= 0b1;
            }
            if (world_face_rot.v1.z < NEAR_Z)
            {
                ++num_verts_to_clip;
                verts_to_clip |= 0b10;
            }
            if (world_face_rot.v2.z < NEAR_Z)
            {
                ++num_verts_to_clip;
                verts_to_clip |= 0b100;
            }
            vec2_t uv[4] = {
                world_face_rot.uv[0].u, world_face_rot.uv[0].v, world_face_rot.uv[1].u, world_face_rot.uv[1].v, world_face_rot.uv[2].u, world_face_rot.uv[2].v,
                0, 0};
            if (num_verts_to_clip == 0)
            {
                // All verts are >= NEAR_Z. No clipping
                cam_faces[cam_it++] = world_face_rot;
            }
            else if (num_verts_to_clip == 1)
            {
                if (verts_to_clip == 0b1)
                { // Clip v0
                    // Distance along v0->v1 where z == NEAR_Z
                    float t = (NEAR_Z - world_face_rot.v0.z) / (world_face_rot.v1.z - world_face_rot.v0.z);
                    v[0].x = world_face_rot.v0.x + (world_face_rot.v1.x - world_face_rot.v0.x) * t;
                    v[0].y = world_face_rot.v0.y + (world_face_rot.v1.y - world_face_rot.v0.y) * t;
                    v[0].z = NEAR_Z;
                    uv[0].u = world_face_rot.uv[0].u + (world_face_rot.uv[1].u - world_face_rot.uv[0].u) * t;
                    uv[0].v = world_face_rot.uv[0].v + (world_face_rot.uv[1].v - world_face_rot.uv[0].v) * t;
                    // Distance along v0->v2 where z == NEAR_Z
                    t = (NEAR_Z - world_face_rot.v0.z) / (world_face_rot.v2.z - world_face_rot.v0.z);
                    v[3].x = world_face_rot.v0.x + (world_face_rot.v2.x - world_face_rot.v0.x) * t;
                    v[3].y = world_face_rot.v0.y + (world_face_rot.v2.y - world_face_rot.v0.y) * t;
                    v[3].z = NEAR_Z;
                    uv[3].u = world_face_rot.uv[0].u + (world_face_rot.uv[2].u - world_face_rot.uv[0].u) * t;
                    uv[3].v = world_face_rot.uv[0].v + (world_face_rot.uv[2].v - world_face_rot.uv[0].v) * t;
                    v[1] = world_face_rot.v1;
                    v[2] = world_face_rot.v2;
                }
                else if (verts_to_clip == 0b10)
                { // Clip v1
                    // Distance along v1->v2 where z == NEAR_Z
                    float t = (NEAR_Z - world_face_rot.v1.z) / (world_face_rot.v2.z - world_face_rot.v1.z);
                    v[0].x = world_face_rot.v1.x + (world_face_rot.v2.x - world_face_rot.v1.x) * t;
                    v[0].y = world_face_rot.v1.y + (world_face_rot.v2.y - world_face_rot.v1.y) * t;
                    v[0].z = NEAR_Z;
                    uv[0].u = world_face_rot.uv[1].u + (world_face_rot.uv[2].u - world_face_rot.uv[1].u) * t;
                    uv[0].v = world_face_rot.uv[1].v + (world_face_rot.uv[2].v - world_face_rot.uv[1].v) * t;
                    // Distance along v1->v0 where z == NEAR_Z
                    t = (NEAR_Z - world_face_rot.v1.z) / (world_face_rot.v0.z - world_face_rot.v1.z);
                    v[3].x = world_face_rot.v1.x + (world_face_rot.v0.x - world_face_rot.v1.x) * t;
                    v[3].y = world_face_rot.v1.y + (world_face_rot.v0.y - world_face_rot.v1.y) * t;
                    v[3].z = NEAR_Z;
                    uv[3].u = world_face_rot.uv[1].u + (world_face_rot.uv[0].u - world_face_rot.uv[1].u) * t;
                    uv[3].v = world_face_rot.uv[1].v + (world_face_rot.uv[0].v - world_face_rot.uv[1].v) * t;
                    v[1] = world_face_rot.v2;
                    v[2] = world_face_rot.v0;
                    uv[1].u = world_face_rot.uv[2].u;
                    uv[2].u = world_face_rot.uv[0].u;
                    uv[1].v = world_face_rot.uv[2].v;
                    uv[2].v = world_face_rot.uv[0].v;
                }
                else if (verts_to_clip == 0b100)
                { // Clip v2
                    // Distance along v2->v0 where z == NEAR_Z
                    float t = (NEAR_Z - world_face_rot.v2.z) / (world_face_rot.v0.z - world_face_rot.v2.z);
                    v[0].x = world_face_rot.v2.x + (world_face_rot.v0.x - world_face_rot.v2.x) * t;
                    v[0].y = world_face_rot.v2.y + (world_face_rot.v0.y - world_face_rot.v2.y) * t;
                    v[0].z = NEAR_Z;
                    uv[0].u = world_face_rot.uv[2].u + (world_face_rot.uv[0].u - world_face_rot.uv[2].u) * t;
                    uv[0].v = world_face_rot.uv[2].v + (world_face_rot.uv[0].v - world_face_rot.uv[2].v) * t;
                    // Distance along v2->v1 where z == NEAR_Z
                    t = (NEAR_Z - world_face_rot.v2.z) / (world_face_rot.v1.z - world_face_rot.v2.z);
                    v[3].x = world_face_rot.v2.x + (world_face_rot.v1.x - world_face_rot.v2.x) * t;
                    v[3].y = world_face_rot.v2.y + (world_face_rot.v1.y - world_face_rot.v2.y) * t;
                    v[3].z = NEAR_Z;
                    uv[3].u = world_face_rot.uv[2].u + (world_face_rot.uv[1].u - world_face_rot.uv[2].u) * t;
                    uv[3].v = world_face_rot.uv[2].v + (world_face_rot.uv[1].v - world_face_rot.uv[2].v) * t;
                    v[1] = world_face_rot.v0;
                    v[2] = world_face_rot.v1;
                    uv[1].u = world_face_rot.uv[0].u;
                    uv[2].u = world_face_rot.uv[1].u;
                    uv[1].v = world_face_rot.uv[0].v;
                    uv[2].v = world_face_rot.uv[1].v;
                }
                cam_faces[cam_it].c = cam_faces[cam_it + 1].c = world_face_rot.c;
                cam_faces[cam_it].v0 = v[0];
                cam_faces[cam_it].v1 = v[1];
                cam_faces[cam_it].v2 = v[2];
                cam_faces[cam_it].texture_index = world_face_rot.texture_index;
                cam_faces[cam_it].c = world_face_rot.c;
                cam_faces[cam_it].flags = world_face_rot.flags;
                cam_faces[cam_it].uv[0].u = uv[0].u;
                cam_faces[cam_it].uv[1].u = uv[1].u;
                cam_faces[cam_it].uv[2].u = uv[2].u;
                cam_faces[cam_it].uv[0].v = uv[0].v;
                cam_faces[cam_it].uv[1].v = uv[1].v;
                cam_faces[cam_it].uv[2].v = uv[2].v;
                ++cam_it;
                cam_faces[cam_it].v0 = v[0];
                cam_faces[cam_it].v1 = v[2];
                cam_faces[cam_it].v2 = v[3];
                cam_faces[cam_it].texture_index = world_face_rot.texture_index;
                cam_faces[cam_it].c = world_face_rot.c;
                cam_faces[cam_it].flags = world_face_rot.flags;
                cam_faces[cam_it].uv[0].u = uv[0].u;
                cam_faces[cam_it].uv[1].u = uv[2].u;
                cam_faces[cam_it].uv[2].u = uv[3].u;
                cam_faces[cam_it].uv[0].v = uv[0].v;
                cam_faces[cam_it].uv[1].v = uv[2].v;
                cam_faces[cam_it].uv[2].v = uv[3].v;
                ++cam_it;
            }
            else if (num_verts_to_clip == 2)
            {
                if (verts_to_clip & 0b1)
                { // Clip v0
                    if (verts_to_clip & 0b10)
                    {                                                                                           // v1 will be clipped so use v2
                        float t = (NEAR_Z - world_face_rot.v0.z) / (world_face_rot.v2.z - world_face_rot.v0.z); // Distance along v0->v2 where z == NEAR_Z
                        v[0].x = world_face_rot.v0.x + (world_face_rot.v2.x - world_face_rot.v0.x) * t;
                        v[0].y = world_face_rot.v0.y + (world_face_rot.v2.y - world_face_rot.v0.y) * t;
                        v[0].z = NEAR_Z;
                        uv[0].u = world_face_rot.uv[0].u + (world_face_rot.uv[2].u - world_face_rot.uv[0].u) * t;
                        uv[0].v = world_face_rot.uv[0].v + (world_face_rot.uv[2].v - world_face_rot.uv[0].v) * t;
                    }
                    else
                    {                                                                                           // v2 will be clipped so use v1
                        float t = (NEAR_Z - world_face_rot.v0.z) / (world_face_rot.v1.z - world_face_rot.v0.z); // Distance along v0->v1 where z == NEAR_Z
                        v[0].x = world_face_rot.v0.x + (world_face_rot.v1.x - world_face_rot.v0.x) * t;
                        v[0].y = world_face_rot.v0.y + (world_face_rot.v1.y - world_face_rot.v0.y) * t;
                        v[0].z = NEAR_Z;
                        uv[0].u = world_face_rot.uv[0].u + (world_face_rot.uv[1].u - world_face_rot.uv[0].u) * t;
                        uv[0].v = world_face_rot.uv[0].v + (world_face_rot.uv[1].v - world_face_rot.uv[0].v) * t;
                    }
                }
                else
                {
                    v[0] = world_face_rot.v0;
                }
                if (verts_to_clip & 0b10)
                { // Clip v1
                    if (verts_to_clip & 0b1)
                    {                                                                                           // v0 will be clipped so use v2
                        float t = (NEAR_Z - world_face_rot.v1.z) / (world_face_rot.v2.z - world_face_rot.v1.z); // Distance along v1->v2 where z == NEAR_Z
                        v[1].x = world_face_rot.v1.x + (world_face_rot.v2.x - world_face_rot.v1.x) * t;
                        v[1].y = world_face_rot.v1.y + (world_face_rot.v2.y - world_face_rot.v1.y) * t;
                        v[1].z = NEAR_Z;
                        uv[1].u = world_face_rot.uv[1].u + (world_face_rot.uv[2].u - world_face_rot.uv[1].u) * t;
                        uv[1].v = world_face_rot.uv[1].v + (world_face_rot.uv[2].v - world_face_rot.uv[1].v) * t;
                    }
                    else
                    {                                                                                           // v2 will be clipped so use v0
                        float t = (NEAR_Z - world_face_rot.v1.z) / (world_face_rot.v0.z - world_face_rot.v1.z); // Distance along v1->v0 where z == NEAR_Z
                        v[1].x = world_face_rot.v1.x + (world_face_rot.v0.x - world_face_rot.v1.x) * t;
                        v[1].y = world_face_rot.v1.y + (world_face_rot.v0.y - world_face_rot.v1.y) * t;
                        v[1].z = NEAR_Z;
                        uv[1].u = world_face_rot.uv[1].u + (world_face_rot.uv[0].u - world_face_rot.uv[1].u) * t;
                        uv[1].v = world_face_rot.uv[1].v + (world_face_rot.uv[0].v - world_face_rot.uv[1].v) * t;
                    }
                }
                else
                {
                    v[1] = world_face_rot.v1;
                }
                if (verts_to_clip & 0b100)
                { // Clip v2
                    if (verts_to_clip & 0b1)
                    {                                                                                           // v0 will be clipped so use v1
                        float t = (NEAR_Z - world_face_rot.v2.z) / (world_face_rot.v1.z - world_face_rot.v2.z); // Distance along v2->v1 where z == NEAR_Z
                        v[2].x = world_face_rot.v2.x + (world_face_rot.v1.x - world_face_rot.v2.x) * t;
                        v[2].y = world_face_rot.v2.y + (world_face_rot.v1.y - world_face_rot.v2.y) * t;
                        v[2].z = NEAR_Z;
                        uv[2].u = world_face_rot.uv[2].u + (world_face_rot.uv[1].u - world_face_rot.uv[2].u) * t;
                        uv[2].v = world_face_rot.uv[2].v + (world_face_rot.uv[1].v - world_face_rot.uv[2].v) * t;
                    }
                    else
                    {                                                                                           // v2 will be clipped so use v0
                        float t = (NEAR_Z - world_face_rot.v2.z) / (world_face_rot.v0.z - world_face_rot.v2.z); // Distance along v1->v0 where z == NEAR_Z
                        v[2].x = world_face_rot.v2.x + (world_face_rot.v0.x - world_face_rot.v2.x) * t;
                        v[2].y = world_face_rot.v2.y + (world_face_rot.v0.y - world_face_rot.v2.y) * t;
                        v[2].z = NEAR_Z;
                        uv[2].u = world_face_rot.uv[2].u + (world_face_rot.uv[0].u - world_face_rot.uv[2].u) * t;
                        uv[2].v = world_face_rot.uv[2].v + (world_face_rot.uv[0].v - world_face_rot.uv[2].v) * t;
                    }
                }
                else
                {
                    v[2] = world_face_rot.v2;
                }
                cam_faces[cam_it].v0 = v[0];
                cam_faces[cam_it].v1 = v[1];
                cam_faces[cam_it].v2 = v[2];
                cam_faces[cam_it].c = world_face_rot.c;
                cam_faces[cam_it].flags = world_face_rot.flags;
                cam_faces[cam_it].texture_index = world_face_rot.texture_index;
                cam_faces[cam_it].uv[0].u = uv[0].u;
                cam_faces[cam_it].uv[1].u = uv[1].u;
                cam_faces[cam_it].uv[2].u = uv[2].u;
                cam_faces[cam_it].uv[0].v = uv[0].v;
                cam_faces[cam_it].uv[1].v = uv[1].v;
                cam_faces[cam_it].uv[2].v = uv[2].v;
                ++cam_it;
            }
            else
            { // Should have already clipped this before so fail assertion.
                assert(false);
            }
        }
        num_cam_faces = cam_it;

        int view_it = cam_it = 0;
        // transform view verts to screen space
        for (; view_it < num_cam_faces; ++view_it, ++cam_it)
        {
            for (int v = 0; v < 3; ++v)
            {
                view_faces[view_it].v[v].x = (cam_faces[cam_it].v[v].x / cam_faces[cam_it].v[v].z + 1) * internal_resolution_width / 2;
                view_faces[view_it].v[v].y = (aspect_ratio * -cam_faces[cam_it].v[v].y / cam_faces[cam_it].v[v].z + 1) * internal_resolution_height / 2;
                view_faces[view_it].v[v].z = NEAR_Z / cam_faces[cam_it].v[v].z;
            }
            view_faces[view_it].c = cam_faces[cam_it].c;
            view_faces[view_it].flags = cam_faces[cam_it].flags;
            view_faces[view_it].texture_index = cam_faces[cam_it].texture_index;
            view_faces[view_it].uv[0].u = cam_faces[cam_it].uv[0].u / cam_faces[cam_it].v0.z;
            view_faces[view_it].uv[1].u = cam_faces[cam_it].uv[1].u / cam_faces[cam_it].v1.z;
            view_faces[view_it].uv[2].u = cam_faces[cam_it].uv[2].u / cam_faces[cam_it].v2.z;
            view_faces[view_it].uv[0].v = cam_faces[cam_it].uv[0].v / cam_faces[cam_it].v0.z;
            view_faces[view_it].uv[1].v = cam_faces[cam_it].uv[1].v / cam_faces[cam_it].v1.z;
            view_faces[view_it].uv[2].v = cam_faces[cam_it].uv[2].v / cam_faces[cam_it].v2.z;
        }
        num_view_faces = view_it;

        // render wireframe to screen
        for (int i = 0; i < num_view_faces; ++i)
        {
            if (view_faces[i].texture_index < MAX_TEXTURES)
            {
                // printf("%d ", view_faces[i].texture_index);
                K3D_DrawTriangleTextured(&render_frame, depth_buffer, &textures[view_faces[i].texture_index], view_faces[i].v0, view_faces[i].v1, view_faces[i].v2, view_faces[i].uv);
            }
            else
            {
                K3D_DrawTriangle(&render_frame, depth_buffer, view_faces[i].v0, view_faces[i].v1, view_faces[i].v2, view_faces[i].c);
            }
        }

        float frame_scale = Min((float)frame_buffer.h / (float)render_frame.h, (float)frame_buffer.w / (float)render_frame.w);
        KS_BlitScaled(&render_frame, &frame_buffer, frame_buffer.w / 2, frame_buffer.h / 2, frame_scale, frame_scale, render_frame.w / 2, render_frame.h / 2);

        for (int i = 0; i < top_message; ++i)
        {
            player_messages[i].time -= platform.delta;
            if (player_messages[i].time < 0)
            {
                --top_message;
                for (int j = i; j < top_message; ++j)
                {
                    player_messages[j].time = player_messages[j + 1].time;
                    for (int k = 0; k < 127 && (player_messages[j].text[k] = player_messages[j + 1].text[k]); ++k)
                        ;
                }
            }
            else
            {
                KF_Draw(&font, &frame_buffer, 0, i * 16, player_messages[i].text);
            }
        }

        if (draw_minimap)
        {
            KS_BlitScaledSafe(&maze_sprite, &frame_buffer, frame_buffer.w - maze_sprite.w * 5, 0, 5, 5, 0, 0);
            // Draw player on minimap
            KS_DrawRectFilled(&frame_buffer, cam.pos.x * 5 + frame_buffer.w - maze_sprite.w * 5, (maze_sprite.h - 1) * 5 - cam.pos.z * 5, cam.pos.x * 5 + frame_buffer.w - maze_sprite.w * 5 + 5, (maze_sprite.h - 1) * 5 - cam.pos.z * 5 + 5, 0xff000000);
            KS_DrawLine(&frame_buffer, cam.pos.x * 5 + frame_buffer.w - maze_sprite.w * 5 + 2, (maze_sprite.h - 1) * 5 - cam.pos.z * 5 + 2, cam.pos.x * 5 + frame_buffer.w - maze_sprite.w * 5 + 2 + 5 * sin(-cam.yaw + 3.f * PI / 4.f) - 5 * cos(-cam.yaw + 3.f * PI / 4.f), (maze_sprite.h - 1) * 5 - cam.pos.z * 5 + 2 + 5 * cos(-cam.yaw + 3.f * PI / 4.f) + 5 * sin(-cam.yaw + 3.f * PI / 4.f), 0xffffffff);
        }

        KP_Flip(&platform);

        if (auto_launch_menu)
        {
            Menu();
        }
    }
    free(maze_stack);
}

void ToggleAI()
{
    ai_control = !ai_control;
    menus[MENU_MAIN].items[3].toggle = ai_control;
    auto_launch_menu = false;
    menu_running = false;
    active_menu = MENU_MAIN;
    ai.last_pos.x = target_pos.x;
    ai.last_pos.z = target_pos.z;
    AILoop();
}

int main(int argc, char *argv[])
{
    internal_resolution_width = internal_resolutions[0][0];
    internal_resolution_height = internal_resolutions[0][1];
    srand(time(0));

    KP_Init(&platform, 1280, 960, "Maze95");

    KP_ShowCursor(&platform, false);

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-fullscreen"))
        {
            printf("fullscreen\n");
            ToggleFullscreen();
        }
    }

    KS_Create(&render_frame, internal_resolution_width, internal_resolution_height);
    KS_Create(&menu_frame, 320, 240);

    frame_buffer.pixels = platform.frame_buffer.pixels;
    frame_buffer.w = platform.frame_buffer.w;
    frame_buffer.h = platform.frame_buffer.h;
    // aspect_ratio = (float)frame_buffer.w / (float)frame_buffer.h;
    // depth_buffer = (float*)malloc(frame_buffer.w*frame_buffer.h*sizeof(float));
    aspect_ratio = (float)internal_resolution_width / (float)internal_resolution_height;
    depth_buffer = (float *)malloc(internal_resolution_width * internal_resolution_height * sizeof(float));

    RestartMaze();

    if (!(
            KF_Load(&font, "font.png") &&

            KS_Load(&textures[0], "textures/floor.png") &&
            KS_Load(&textures[1], "textures/wall.png") &&
            KS_Load(&textures[2], "textures/ceiling.png") &&

            KS_Load(&textures[3], "textures/games/0.png") &&
            KS_Load(&textures[4], "textures/games/1.png") &&
            KS_Load(&textures[5], "textures/games/2.png") &&
            KS_Load(&textures[6], "textures/games/3.png") &&
            KS_Load(&textures[7], "textures/games/4.png") &&
            KS_Load(&textures[8], "textures/games/5.png") &&
            KS_Load(&textures[9], "textures/games/6.png") &&
            KS_Load(&textures[10], "textures/games/7.png") &&
            KS_Load(&textures[11], "textures/games/8.png") &&
            KS_Load(&textures[12], "textures/games/9.png") &&
            KS_Load(&textures[13], "textures/games/10.png") &&

            KS_Load(&textures[14], "textures/title.png") &&
            KS_Load(&textures[15], "textures/mazevr.png")))
    {
        printf("Failed to load textures.\n");
        return -1;
    }

    profile_colours[0] = 0xff888888;
    profile_colours[1] = 0xffff0000;
    profile_colours[2] = 0xff00ff00;
    profile_colours[3] = 0xff0000ff;
    profile_colours[4] = 0xffffff00;
    profile_colours[5] = 0xff00ffff;
    profile_colours[6] = 0xffff00ff;
    profile_colours[7] = 0xffffffff;
    profile_colours[8] = 0xff88ff00;
    profile_colours[9] = 0xffff8800;
    profile_colours[10] = 0xff8800ff;
    profile_colours[11] = 0xffff0088;
    profile_colours[12] = 0xff00ff88;
    profile_colours[13] = 0xff44bb44;
    profile_colours[14] = 0xffbb44bb;
    profile_colours[15] = 0xff4444ff;

    { // Menu setup
        menus[MENU_MAIN].num_items = 5;
        MenuItemMakeDefaultColor(&menus[MENU_MAIN].items[0], 60, 20, 260, 60, 128, 32, "Play", -1, -1, 4, 1, MainMenuPlay);
        MenuItemMakeDefaultColor(&menus[MENU_MAIN].items[1], 60, 70, 260, 110, 104, 82, "Options", -1, -1, 0, 2, MainMenuOptions);
        MenuItemMakeDefaultColor(&menus[MENU_MAIN].items[2], 60, 120, 200, 160, 74, 132, "Restart", -1, 3, 1, 4, RestartMaze);
        MenuItemMake(&menus[MENU_MAIN].items[3], 220, 120, 260, 160, 224, 132, "AI", 2, -1, 1, 4, ToggleAI, 0xff002f79, 0xff5e9dff);
        MenuItemMake(&menus[MENU_MAIN].items[4], 60, 180, 260, 220, 128, 192, "Quit", -1, -1, 2, 0, QuitGame, 0xffbc0000, 0xffff3737);

        menus[MENU_OPTIONS].num_items = 8;
        MenuItemMakeDefaultColor(&menus[MENU_OPTIONS].items[0], 60, 20, 260, 60, 80, 32, "Fullscreen", -1, -1, 7, 2, ToggleFullscreen);
        MenuItemMake(&menus[MENU_OPTIONS].items[1], 10, 70, 50, 110, 22, 82, "-", -1, 2, 0, 4, InternalResolutionDown, 0xff002f79, 0xff5e9dff);
        MenuItemMakeDefaultColor(&menus[MENU_OPTIONS].items[2], 60, 70, 260, 110, 80, 74, "Internal\nresolution", 1, 3, 0, 5, ResetInternalResolution);
        MenuItemMake(&menus[MENU_OPTIONS].items[3], 270, 70, 310, 110, 282, 82, "+", 2, -1, 0, 6, InternalResolutionUp, 0xff002f79, 0xff5e9dff);
        MenuItemMake(&menus[MENU_OPTIONS].items[4], 10, 120, 50, 160, 22, 132, "-", -1, 5, 1, 7, MazeSizeDown, 0xff002f79, 0xff5e9dff);
        MenuItemMakeDefaultColor(&menus[MENU_OPTIONS].items[5], 60, 120, 260, 160, 93, 124, "Maze size", 4, 6, 2, 7, ResetMazeSize);
        sprintf((char *)&menus[MENU_OPTIONS].items[5].text, "Maze size\n   %3d", maze_size);
        MenuItemMake(&menus[MENU_OPTIONS].items[6], 270, 120, 310, 160, 282, 132, "+", 5, -1, 3, 7, MazeSizeUp, 0xff002f79, 0xff5e9dff);
        MenuItemMake(&menus[MENU_OPTIONS].items[7], 60, 180, 260, 220, 128, 192, "Back", -1, -1, 5, 0, MenuToMain, 0xffbc0000, 0xffff3737);

        menus[MENU_WIN].num_items = 4;
        menus[MENU_WIN].active_item = 1;
        MenuItemMakeDefaultColor(&menus[MENU_WIN].items[0], 60, 70, 260, 110, 80, 82, "Play again", -1, -1, 2, 1, RestartMaze);
        MenuItemMakeDefaultColor(&menus[MENU_WIN].items[1], 60, 120, 260, 160, 72, 124, "Other great\nindie games", -1, -1, 0, 2, OtherGames);
        MenuItemMake(&menus[MENU_WIN].items[2], 60, 180, 260, 220, 128, 192, "Quit", -1, -1, 1, 0, QuitGame, 0xffbc0000, 0xffff3737);
        MenuItemMake(&menus[MENU_WIN].items[3], 60, 20, 260, 60, 96, 32, "You win!", 0, 0, 0, 0, NilFunc, 0xff0ca000, 0xff0ca000);

        menus[MENU_TITLE].num_items = 0;
    }

    K3D_CreateDodecahedron(dodecahedron, 0.5f);

    KP_SetCursorPos(&platform, frame_buffer.w / 2, frame_buffer.h / 2, NULL, NULL);
    while (game_running)
    {
        ProfileTime("Frame start");

        while (KP_EventsQueued(&platform))
        {
            kp_event_t *e = KP_NextEvent(&platform);
            switch (e->type)
            {
            case KP_EVENT_KEY_PRESS:
            {
                switch (e->key)
                {
                case KEY_ENTER:
                {
                    if (platform.keyboard[KEY_RALT] || platform.keyboard[KEY_LALT])
                    {
                        KP_Fullscreen(&platform, !platform.fullscreen);
                    }
                }
                break;
                case KEY_ESCAPE:
                {
                    active_menu = MENU_MAIN;
                    Menu();
                    continue;
                }
                break;
                case KEY_1:
                {
                    control_mode = CONTROL_GRID | CONTROL_TURN90;
                    target_pos.x = cam.pos.x / maze.cell_size;
                    target_pos.z = cam.pos.z / maze.cell_size;
                    turn_target = cam.yaw / HALFPI;
                    PlayerMessage("Normal movement");
                }
                break;
                case KEY_2:
                {
                    control_mode = CONTROL_WASD | CONTROL_MOUSE;
                    PlayerMessage("Free movement");
                }
                break;
                case KEY_3:
                {
                    draw_framerate = !draw_framerate;
                    if (draw_framerate)
                    {
                        PlayerMessage("Show fps");
                    }
                    else
                    {
                        PlayerMessage("Hide fps");
                    }
                }
                break;
                case KEY_4:
                {
                    draw_minimap = !draw_minimap;
                    if (draw_framerate)
                    {
                        PlayerMessage("Show map");
                    }
                    else
                    {
                        PlayerMessage("Hide map");
                    }
                }
                break;
                case KEY_P:
                {
                    draw_profiles = !draw_profiles;
                }
                break;
                case KEY_LEFT:
                case KEY_A:
                {
                    if (control_mode & CONTROL_TURN90)
                    {
                        cam.pitch = HALFPI;
                        if (roll_target % 2)
                        {
                            if (++turn_target > 3)
                            {
                                turn_target = 0;
                                cam.yaw = -HALFPI / 2.f;
                            }
                        }
                        else
                        {
                            if (--turn_target < 0)
                            {
                                turn_target = 3;
                                cam.yaw = TWOPI;
                            }
                        }
                    }
                }
                break;
                case KEY_RIGHT:
                case KEY_D:
                {
                    if (control_mode & CONTROL_TURN90)
                    {
                        cam.pitch = HALFPI;
                        if (roll_target % 2)
                        {
                            if (--turn_target < 0)
                            {
                                turn_target = 3;
                                cam.yaw = TWOPI;
                            }
                        }
                        else
                        {
                            if (++turn_target > 3)
                            {
                                turn_target = 0;
                                cam.yaw = -HALFPI / 2.f;
                            }
                        }
                    }
                }
                break;
                case KEY_UP:
                case KEY_W:
                {
                    if (control_mode & CONTROL_GRID)
                    {
                        float siny = sin((float)turn_target * HALFPI);
                        float cosy = cos((float)turn_target * HALFPI);
                        int dx = Absolute(cosy) > 0.1f ? Sign(cosy) : 0;
                        int dz = Absolute(siny) > 0.1f ? -Sign(siny) : 0;
                        if (dx > 0)
                        {
                            if (target_pos.x < maze.w - 1)
                            {
                                if (!(maze.cells[target_pos.z * maze.w + target_pos.x] & MAZE_RIGHT))
                                {
                                    dx = 0;
                                }
                            }
                            else
                            {
                                dx = 0;
                            }
                        }
                        if (dx < 0)
                        {
                            if (target_pos.x > 0)
                            {
                                if (!(maze.cells[target_pos.z * maze.w + target_pos.x] & MAZE_LEFT))
                                {
                                    dx = 0;
                                }
                            }
                            else
                            {
                                dx = 0;
                            }
                        }
                        if (dz > 0)
                        {
                            if (target_pos.z < maze.h - 1)
                            {
                                if (!(maze.cells[target_pos.z * maze.w + target_pos.x] & MAZE_UP))
                                {
                                    dz = 0;
                                }
                            }
                            else
                            {
                                dz = 0;
                            }
                        }
                        if (dz < 0)
                        {
                            if (target_pos.z > 0)
                            {
                                if (!(maze.cells[target_pos.z * maze.w + target_pos.x] & MAZE_DOWN))
                                {
                                    dz = 0;
                                }
                            }
                            else
                            {
                                dz = 0;
                            }
                        }
                        target_pos.x += dx;
                        target_pos.z += dz;
                        cam.pitch = HALFPI;
                        if (target_pos.x == maze.end.x && target_pos.z == maze.end.y)
                        {
                            Win();
                        }
                    }
                }
                break;
                case KEY_DOWN:
                case KEY_S:
                {
                    if (control_mode & CONTROL_GRID)
                    {
                        cam.pitch = HALFPI;
                        if (control_mode & CONTROL_DOWN_MOVE)
                        {
                            float siny = sin((float)turn_target * HALFPI);
                            float cosy = cos((float)turn_target * HALFPI);
                            target_pos.x -= Absolute(cosy) > 0.1f ? Sign(cosy) : 0;
                            target_pos.z += Absolute(siny) > 0.1f ? Sign(siny) : 0;
                        }
                        else
                        {
                            if (turn_target < 2)
                            {
                                turn_target += 2;
                            }
                            else if (turn_target > -2)
                            {
                                turn_target -= 2;
                            }
                        }
                    }
                }
                break;
                }
            }
            break;
            case KP_EVENT_RESIZE:
            {
                frame_buffer.pixels = platform.frame_buffer.pixels;
                frame_buffer.w = platform.frame_buffer.w;
                frame_buffer.h = platform.frame_buffer.h;
                KS_SetAllPixels(&frame_buffer, 0x00000000);
                // aspect_ratio = (float)frame_buffer.w / (float)frame_buffer.h;
                // depth_buffer = (float*)realloc(depth_buffer, sizeof(float)*frame_buffer.w*frame_buffer.h);
            }
            break;
            case KP_EVENT_QUIT:
            {
                exit(0);
            }
            break;
            }
            KP_FreeEvent(e);
        }

        if (control_mode & CONTROL_MOUSE)
        {
            int dx, dy;
            KP_SetCursorPos(&platform, frame_buffer.w / 2, frame_buffer.h / 2, &dx, &dy);
            cam.yaw -= (float)dx * CAM_ROT_SPEED;
            cam.pitch -= (float)dy * CAM_ROT_SPEED;
        }

        if (control_mode & CONTROL_GRID)
        {
            cam.pos.x = Lerp(cam.pos.x, target_pos.x * maze.cell_size + maze.cell_size / 2.f, Min(1.f, platform.delta * 15.f));
            cam.pos.z = Lerp(cam.pos.z, target_pos.z * maze.cell_size + maze.cell_size / 2.f, Min(1.f, platform.delta * 15.f));
        }
        else if (control_mode & CONTROL_WASD)
        {
            float siny = sin(cam.yaw);
            float cosy = cos(cam.yaw);
            float cam_speed_delta = platform.delta * CAM_SPEED;
            if (platform.keyboard[KEY_LSHIFT] || platform.keyboard[KEY_RSHIFT])
            {
                cam_speed_delta *= 2.f;
            }
            float sinr = sin(cam.yaw + HALFPI);
            float cosr = cos(cam.yaw + HALFPI);
            if (platform.keyboard[KEY_W])
            {
                cam.pos.x += cosy * cam_speed_delta;
                cam.pos.z -= siny * cam_speed_delta;
            }
            if (platform.keyboard[KEY_S])
            {
                cam.pos.x -= cosy * cam_speed_delta;
                cam.pos.z += siny * cam_speed_delta;
            }
            if (platform.keyboard[KEY_D])
            {
                cam.pos.x += cosr * cam_speed_delta;
                cam.pos.z -= sinr * cam_speed_delta;
            }
            if (platform.keyboard[KEY_A])
            {
                cam.pos.x -= cosr * cam_speed_delta;
                cam.pos.z += sinr * cam_speed_delta;
            }
        }

        if (control_mode & CONTROL_TURN90)
        {
            if (Absolute((float)(turn_target % 4) * HALFPI - cam.yaw) > 0.00001f)
            {
                cam.yaw = Lerp(cam.yaw, (float)turn_target * HALFPI, Min(1.f, platform.delta * 15.f));
            }
        }
        else if (control_mode & CONTROL_MOUSE)
        {
            if (platform.keyboard[KEY_LEFT])
            {
                cam.yaw -= CAM_ROT_SPEED * 1000.f * platform.delta;
            }
            if (platform.keyboard[KEY_RIGHT])
            {
                cam.yaw += CAM_ROT_SPEED * 1000.f * platform.delta;
            }
            if (platform.keyboard[KEY_UP])
            {
                cam.pitch -= CAM_ROT_SPEED * 1000.f * platform.delta;
            }
            if (platform.keyboard[KEY_DOWN])
            {
                cam.pitch += CAM_ROT_SPEED * 1000.f * platform.delta;
            }
        }
        if (Absolute((float)(roll_target % 2) * PI - cam.roll) > 0.00001f)
        {
            cam.roll = Lerp(cam.roll, (float)roll_target * PI, Min(1.f, platform.delta * 10.f));
        }

        /*if(platform.keyboard[KEY_Q]) {
            cam.roll -= CAM_ROT_SPEED * 1000.f * platform.delta;
        }
        if(platform.keyboard[KEY_E]) {
            cam.roll += CAM_ROT_SPEED * 1000.f * platform.delta;
        }*/

        if (Absolute(cam.yaw) > TWOPI)
        {
            cam.yaw -= Sign(cam.yaw) * TWOPI;
        }
        if (cam.pitch < 0.f)
        {
            cam.pitch = 0.f;
        }
        else if (cam.pitch > PI)
        {
            cam.pitch = PI;
        }
        if (Absolute(cam.roll) > TWOPI)
        {
            cam.roll -= Sign(cam.roll) * TWOPI;
        }

        for (int i = 0; i < num_dodecahedrons; ++i)
        {
            if (dodecahedrons[i].active)
            {
                if (control_mode & CONTROL_GRID)
                {
                    if (Vec2Length(Vec2Sub(Vec2Make(target_pos.x * maze.cell_size + maze.cell_size / 2.f, target_pos.z * maze.cell_size + maze.cell_size / 2.f), Vec2Make(dodecahedrons[i].pos.x, dodecahedrons[i].pos.z))) < 0.5f)
                    {
                        roll_target = (roll_target + 1) % 2;
                        dodecahedrons[i].active = false;
                    }
                }
                else
                {
                    if (Vec2Length(Vec2Sub(Vec2Make(cam.pos.x, cam.pos.z), Vec2Make(dodecahedrons[i].pos.x, dodecahedrons[i].pos.z))) < maze.cell_size)
                    {
                        roll_target = (roll_target + 1) % 2;
                        dodecahedrons[i].active = false;
                    }
                }
            }
        }

        ProfileTime("Inputs");

        for (int i = 0; i < num_dodecahedrons; ++i)
        {
            dodecahedrons[i].rot.V[i % 3] += platform.delta;
        }

#if 0
        vec2_t* rays = NULL;
        vec2_t vec_player_pos = Vec2Make(cam.pos.x, cam.pos.z);
        unsigned int maze_faces_to_render[MAX_FACES] = { 0, 1, 2, 3 };
        unsigned int num_maze_faces_to_render = 4;
        // Start by enabling floor/ceiling faces
#define MAX_RAY_LENGTH 850
        float* ray_angles = 0;
        for(int wall_it = 0; wall_it < sb_count(walls); ++wall_it){
            vec2_t intersection0 = Vec2Make(walls[wall_it].a.x, walls[wall_it].a.y);
            vec2_t intersection1 = Vec2Make(walls[wall_it].b.x, walls[wall_it].b.y);
            float angle0 = Vec2Angle(Vec2Sub(intersection0, vec_player_pos));
            float angle1 = Vec2Angle(Vec2Sub(intersection1, vec_player_pos));
            if(angle1 < angle0 || angle1 > angle0 + PI){
                float anglet = angle0; angle0 = angle1; angle1 = anglet;
                vec2_t intersectiont = intersection0; intersection0 = intersection1; intersection1 = intersectiont;
            }
            vec2_t intersection0_off = Vec2Add(Vec2Rotated(Vec2Make(MAX_RAY_LENGTH, 0), angle0-0.001f), vec_player_pos);
            vec2_t intersection1_off = Vec2Add(Vec2Rotated(Vec2Make(MAX_RAY_LENGTH, 0), angle1+0.001f), vec_player_pos);
            bool check_point0 = true, check_point1 = true;
            int secondary_wall[2] = {-1, -1};
            for(int wall_it_sub = 0; wall_it_sub < sb_count(walls); ++wall_it_sub){
                if(wall_it_sub != wall_it){
                    if(check_point0){
                        vec2_t intersection0_prev = intersection0;
                        if(Vec2IntersectLineSegments(vec_player_pos, intersection0, walls[wall_it_sub].a, walls[wall_it_sub].b, &intersection0)){
                            if(Vec2DistanceBetween(intersection0, intersection0_prev) >= 1.f){
                                check_point0 = false;
                            }
                        }
                    }
                    else if(!check_point1){
                        break;
                    }
                    if(check_point1){
                        vec2_t intersection1_prev = intersection1;
                        if(Vec2IntersectLineSegments(vec_player_pos, intersection1, walls[wall_it_sub].a, walls[wall_it_sub].b, &intersection1)){
                            if(Vec2DistanceBetween(intersection1, intersection1_prev) >= 1.f){
                                check_point1 = false;
                            }
                        }
                    }
                }
            }
            if(check_point0 || check_point1){
                for(int wall_it_sub = 0; wall_it_sub < sb_count(walls); ++wall_it_sub){
                    if(check_point0){
                        if(Vec2IntersectLineSegments(vec_player_pos, intersection0_off, walls[wall_it_sub].a, walls[wall_it_sub].b, &intersection0_off)) {
                            secondary_wall[0] = wall_it_sub;
                        }
                    }
                    if(check_point1){
                        if(Vec2IntersectLineSegments(vec_player_pos, intersection1_off, walls[wall_it_sub].a, walls[wall_it_sub].b, &intersection1_off)) {
                            secondary_wall[1] = wall_it_sub;
                        }
                    }
                }
            }
            if(check_point0){
                sb_push(ray_angles, angle0-0.001f); sb_push(ray_angles, angle0);
                sb_push(rays, Vec2Sub(intersection0_off, vec_player_pos));
                sb_push(rays, Vec2Sub(intersection0, vec_player_pos));
            }
            if(check_point1){
                sb_push(ray_angles, angle1); sb_push(ray_angles, angle1+0.001f);
                sb_push(rays, Vec2Sub(intersection1, vec_player_pos));
                sb_push(rays, Vec2Sub(intersection1_off, vec_player_pos));
            }
            if(check_point0 || check_point1) {
                maze_faces_to_render[num_maze_faces_to_render++] = wall_it*2+4;
                maze_faces_to_render[num_maze_faces_to_render++] = wall_it*2+5;
            }
            if(secondary_wall[0] > -1) {
                maze_faces_to_render[num_maze_faces_to_render++] = secondary_wall[0]*2+4;
                maze_faces_to_render[num_maze_faces_to_render++] = secondary_wall[0]*2+5;
            }
            if(secondary_wall[1] > -1) {
                maze_faces_to_render[num_maze_faces_to_render++] = secondary_wall[1]*2+4;
                maze_faces_to_render[num_maze_faces_to_render++] = secondary_wall[1]*2+5;
            }
        }
        SortRays(0, sb_count(rays)-1, ray_angles, rays);
        for(int face = 0; face < num_maze_faces_to_render; ++face) {
            for(int nface = face+1; nface < num_maze_faces_to_render; ++nface) {
                if(maze_faces_to_render[face] == maze_faces_to_render[nface]) {
                    for(int i = nface; i < num_maze_faces_to_render; ++i) {
                        maze_faces_to_render[i] = maze_faces_to_render[i+1];
                    }
                    --num_maze_faces_to_render;
                }
            }
        }
        ProfileTime("Cast rays");
#endif

        KS_Clear(&frame_buffer);
        // KS_SetAllPixels(&frame_buffer, 0x00000000);
        // memset(depth_buffer, 0, frame_buffer.w*frame_buffer.h*sizeof(float));
        // KS_SetAllPixels(&render_frame, 0x00000000);
        memset(depth_buffer, 0, internal_resolution_width * internal_resolution_height * sizeof(float));
        ProfileTime("Clear buffers");

        /*for(int i = 0; i < NUM_CUBES; ++i) {
        cube_rot[i].y += platform.delta/(i+1);
        }*/

        // transform cube vertices to world space
        int world_it = 0;
        // Transform dodecahedron faces to world
        for (int dodec_it = 0; dodec_it < num_dodecahedrons; ++dodec_it)
        {
            if (!dodecahedrons[dodec_it].active)
                continue;
            for (int f = 0; f < 36; ++f)
            {
                world_faces[world_it] = K3D_TranslateRotate(dodecahedron[f], dodecahedrons[dodec_it].pos, dodecahedrons[dodec_it].rot);
                ++world_it;
            }
        }
        // Transform end board faces to world
        for (int f = 0; f < 2; ++f)
        {
            world_faces[world_it] = K3D_TranslateRotate(end_board[f], Vec3Make((float)maze.end.x * maze.cell_size + maze.cell_size / 2, 2.5f, (float)maze.end.y * maze.cell_size + maze.cell_size / 2), Vec3Make(cam.pitch, cam.yaw + PI, 0));
            ++world_it;
        }
        // Copy maze faces to world faces
        // Copy all maze faces
        for (int maze_it = 0; maze_it < num_maze_faces; ++maze_it)
        {
            world_faces[world_it].v0 = maze_faces[maze_it].v0;
            world_faces[world_it].v1 = maze_faces[maze_it].v1;
            world_faces[world_it].v2 = maze_faces[maze_it].v2;
            world_faces[world_it].c = maze_faces[maze_it].c;
            world_faces[world_it].flags = maze_faces[maze_it].flags;
            world_faces[world_it].texture_index = maze_faces[maze_it].texture_index;
            world_faces[world_it].uv[0].u = maze_faces[maze_it].uv[0].u;
            world_faces[world_it].uv[1].u = maze_faces[maze_it].uv[1].u;
            world_faces[world_it].uv[2].u = maze_faces[maze_it].uv[2].u;
            world_faces[world_it].uv[0].v = maze_faces[maze_it].uv[0].v;
            world_faces[world_it].uv[1].v = maze_faces[maze_it].uv[1].v;
            world_faces[world_it].uv[2].v = maze_faces[maze_it].uv[2].v;
            ++world_it;
        }
        /*// Only copy visible faces according to rays
        for(int maze_it = 0; maze_it < num_maze_faces_to_render; ++maze_it) {
            world_faces[world_it].v0 = maze_faces[maze_faces_to_render[maze_it]].v0;
            world_faces[world_it].v1 = maze_faces[maze_faces_to_render[maze_it]].v1;
            world_faces[world_it].v2 = maze_faces[maze_faces_to_render[maze_it]].v2;
            world_faces[world_it].c = maze_faces[maze_faces_to_render[maze_it]].c;
            world_faces[world_it].flags = maze_faces[maze_faces_to_render[maze_it]].flags;
            world_faces[world_it].texture_index = maze_faces[maze_faces_to_render[maze_it]].texture_index;
            world_faces[world_it].uv[0].u = maze_faces[maze_faces_to_render[maze_it]].uv[0].u;
            world_faces[world_it].uv[1].u = maze_faces[maze_faces_to_render[maze_it]].uv[1].u;
            world_faces[world_it].uv[2].u = maze_faces[maze_faces_to_render[maze_it]].uv[2].u;
            world_faces[world_it].uv[0].v = maze_faces[maze_faces_to_render[maze_it]].uv[0].v;
            world_faces[world_it].uv[1].v = maze_faces[maze_faces_to_render[maze_it]].uv[1].v;
            world_faces[world_it].uv[2].v = maze_faces[maze_faces_to_render[maze_it]].uv[2].v;
            ++world_it;
        }*/
        num_world_faces = world_it;
        ProfileTime("Object->World");

        // transform world faces to view space
        int cam_it = world_it = 0;
        face_t world_face_trans, world_face_rot, world_face_clipped[2], temp;
        vec3_t v[4];
        int num_verts_to_clip, verts_to_clip;
        for (; world_it < num_world_faces; ++world_it)
        {
            if (!world_faces[world_it].flags.double_sided)
            {
                // back-face culling
                vec3_t n = Vec3Norm(Vec3Cross(Vec3AToB(world_faces[world_it].v0, world_faces[world_it].v1), Vec3AToB(world_faces[world_it].v0, world_faces[world_it].v2)));
                vec3_t poly_to_cam = Vec3AToB(world_faces[world_it].v0, cam.pos);
                float angle = Vec3Dot(n, poly_to_cam);
                if (angle <= 0)
                    continue;
            }
            world_face_rot = K3D_CameraTranslateRotate(world_faces[world_it], cam.pos, cam.rot);

            if (world_face_rot.v0.z < NEAR_Z && world_face_rot.v1.z < NEAR_Z && world_face_rot.v2.z < NEAR_Z /* || world_face_rot.v0.z > FAR_Z && world_face_rot.v1.z > FAR_Z && world_face_rot.v2.z > FAR_Z*/)
                continue;
            // clip on NEAR_Z plane
            num_verts_to_clip = verts_to_clip = 0;
            if (world_face_rot.v0.z < NEAR_Z)
            {
                ++num_verts_to_clip;
                verts_to_clip |= 0b1;
            }
            if (world_face_rot.v1.z < NEAR_Z)
            {
                ++num_verts_to_clip;
                verts_to_clip |= 0b10;
            }
            if (world_face_rot.v2.z < NEAR_Z)
            {
                ++num_verts_to_clip;
                verts_to_clip |= 0b100;
            }
            vec2_t uv[4] = {
                world_face_rot.uv[0].u,
                world_face_rot.uv[0].v,
                world_face_rot.uv[1].u,
                world_face_rot.uv[1].v,
                world_face_rot.uv[2].u,
                world_face_rot.uv[2].v,
            };
            if (num_verts_to_clip == 0)
            {
                // All verts are >= NEAR_Z. No clipping
                cam_faces[cam_it++] = world_face_rot;
            }
            else if (num_verts_to_clip == 1)
            {
                if (verts_to_clip == 0b1)
                { // Clip v0
                    // Distance along v0->v1 where z == NEAR_Z
                    float t = (NEAR_Z - world_face_rot.v0.z) / (world_face_rot.v1.z - world_face_rot.v0.z);
                    v[0].x = world_face_rot.v0.x + (world_face_rot.v1.x - world_face_rot.v0.x) * t;
                    v[0].y = world_face_rot.v0.y + (world_face_rot.v1.y - world_face_rot.v0.y) * t;
                    v[0].z = NEAR_Z;
                    uv[0].u = world_face_rot.uv[0].u + (world_face_rot.uv[1].u - world_face_rot.uv[0].u) * t;
                    uv[0].v = world_face_rot.uv[0].v + (world_face_rot.uv[1].v - world_face_rot.uv[0].v) * t;
                    // Distance along v0->v2 where z == NEAR_Z
                    t = (NEAR_Z - world_face_rot.v0.z) / (world_face_rot.v2.z - world_face_rot.v0.z);
                    v[3].x = world_face_rot.v0.x + (world_face_rot.v2.x - world_face_rot.v0.x) * t;
                    v[3].y = world_face_rot.v0.y + (world_face_rot.v2.y - world_face_rot.v0.y) * t;
                    v[3].z = NEAR_Z;
                    uv[3].u = world_face_rot.uv[0].u + (world_face_rot.uv[2].u - world_face_rot.uv[0].u) * t;
                    uv[3].v = world_face_rot.uv[0].v + (world_face_rot.uv[2].v - world_face_rot.uv[0].v) * t;
                    v[1] = world_face_rot.v1;
                    v[2] = world_face_rot.v2;
                }
                else if (verts_to_clip == 0b10)
                { // Clip v1
                    // Distance along v1->v2 where z == NEAR_Z
                    float t = (NEAR_Z - world_face_rot.v1.z) / (world_face_rot.v2.z - world_face_rot.v1.z);
                    v[0].x = world_face_rot.v1.x + (world_face_rot.v2.x - world_face_rot.v1.x) * t;
                    v[0].y = world_face_rot.v1.y + (world_face_rot.v2.y - world_face_rot.v1.y) * t;
                    v[0].z = NEAR_Z;
                    uv[0].u = world_face_rot.uv[1].u + (world_face_rot.uv[2].u - world_face_rot.uv[1].u) * t;
                    uv[0].v = world_face_rot.uv[1].v + (world_face_rot.uv[2].v - world_face_rot.uv[1].v) * t;
                    // Distance along v1->v0 where z == NEAR_Z
                    t = (NEAR_Z - world_face_rot.v1.z) / (world_face_rot.v0.z - world_face_rot.v1.z);
                    v[3].x = world_face_rot.v1.x + (world_face_rot.v0.x - world_face_rot.v1.x) * t;
                    v[3].y = world_face_rot.v1.y + (world_face_rot.v0.y - world_face_rot.v1.y) * t;
                    v[3].z = NEAR_Z;
                    uv[3].u = world_face_rot.uv[1].u + (world_face_rot.uv[0].u - world_face_rot.uv[1].u) * t;
                    uv[3].v = world_face_rot.uv[1].v + (world_face_rot.uv[0].v - world_face_rot.uv[1].v) * t;
                    v[1] = world_face_rot.v2;
                    v[2] = world_face_rot.v0;
                    uv[1].u = world_face_rot.uv[2].u;
                    uv[2].u = world_face_rot.uv[0].u;
                    uv[1].v = world_face_rot.uv[2].v;
                    uv[2].v = world_face_rot.uv[0].v;
                }
                else if (verts_to_clip == 0b100)
                { // Clip v2
                    // Distance along v2->v0 where z == NEAR_Z
                    float t = (NEAR_Z - world_face_rot.v2.z) / (world_face_rot.v0.z - world_face_rot.v2.z);
                    v[0].x = world_face_rot.v2.x + (world_face_rot.v0.x - world_face_rot.v2.x) * t;
                    v[0].y = world_face_rot.v2.y + (world_face_rot.v0.y - world_face_rot.v2.y) * t;
                    v[0].z = NEAR_Z;
                    uv[0].u = world_face_rot.uv[2].u + (world_face_rot.uv[0].u - world_face_rot.uv[2].u) * t;
                    uv[0].v = world_face_rot.uv[2].v + (world_face_rot.uv[0].v - world_face_rot.uv[2].v) * t;
                    // Distance along v2->v1 where z == NEAR_Z
                    t = (NEAR_Z - world_face_rot.v2.z) / (world_face_rot.v1.z - world_face_rot.v2.z);
                    v[3].x = world_face_rot.v2.x + (world_face_rot.v1.x - world_face_rot.v2.x) * t;
                    v[3].y = world_face_rot.v2.y + (world_face_rot.v1.y - world_face_rot.v2.y) * t;
                    v[3].z = NEAR_Z;
                    uv[3].u = world_face_rot.uv[2].u + (world_face_rot.uv[1].u - world_face_rot.uv[2].u) * t;
                    uv[3].v = world_face_rot.uv[2].v + (world_face_rot.uv[1].v - world_face_rot.uv[2].v) * t;
                    v[1] = world_face_rot.v0;
                    v[2] = world_face_rot.v1;
                    uv[1].u = world_face_rot.uv[0].u;
                    uv[2].u = world_face_rot.uv[1].u;
                    uv[1].v = world_face_rot.uv[0].v;
                    uv[2].v = world_face_rot.uv[1].v;
                }
                cam_faces[cam_it].c = cam_faces[cam_it + 1].c = world_face_rot.c;
                cam_faces[cam_it].v0 = v[0];
                cam_faces[cam_it].v1 = v[1];
                cam_faces[cam_it].v2 = v[2];
                cam_faces[cam_it].texture_index = world_face_rot.texture_index;
                cam_faces[cam_it].c = world_face_rot.c;
                cam_faces[cam_it].flags = world_face_rot.flags;
                cam_faces[cam_it].uv[0].u = uv[0].u;
                cam_faces[cam_it].uv[1].u = uv[1].u;
                cam_faces[cam_it].uv[2].u = uv[2].u;
                cam_faces[cam_it].uv[0].v = uv[0].v;
                cam_faces[cam_it].uv[1].v = uv[1].v;
                cam_faces[cam_it].uv[2].v = uv[2].v;
                ++cam_it;
                cam_faces[cam_it].v0 = v[0];
                cam_faces[cam_it].v1 = v[2];
                cam_faces[cam_it].v2 = v[3];
                cam_faces[cam_it].texture_index = world_face_rot.texture_index;
                cam_faces[cam_it].c = world_face_rot.c;
                cam_faces[cam_it].flags = world_face_rot.flags;
                cam_faces[cam_it].uv[0].u = uv[0].u;
                cam_faces[cam_it].uv[1].u = uv[2].u;
                cam_faces[cam_it].uv[2].u = uv[3].u;
                cam_faces[cam_it].uv[0].v = uv[0].v;
                cam_faces[cam_it].uv[1].v = uv[2].v;
                cam_faces[cam_it].uv[2].v = uv[3].v;
                ++cam_it;
            }
            else if (num_verts_to_clip == 2)
            {
                if (verts_to_clip & 0b1)
                { // Clip v0
                    if (verts_to_clip & 0b10)
                    {                                                                                           // v1 will be clipped so use v2
                        float t = (NEAR_Z - world_face_rot.v0.z) / (world_face_rot.v2.z - world_face_rot.v0.z); // Distance along v0->v2 where z == NEAR_Z
                        v[0].x = world_face_rot.v0.x + (world_face_rot.v2.x - world_face_rot.v0.x) * t;
                        v[0].y = world_face_rot.v0.y + (world_face_rot.v2.y - world_face_rot.v0.y) * t;
                        v[0].z = NEAR_Z;
                        uv[0].u = world_face_rot.uv[0].u + (world_face_rot.uv[2].u - world_face_rot.uv[0].u) * t;
                        uv[0].v = world_face_rot.uv[0].v + (world_face_rot.uv[2].v - world_face_rot.uv[0].v) * t;
                    }
                    else
                    {                                                                                           // v2 will be clipped so use v1
                        float t = (NEAR_Z - world_face_rot.v0.z) / (world_face_rot.v1.z - world_face_rot.v0.z); // Distance along v0->v1 where z == NEAR_Z
                        v[0].x = world_face_rot.v0.x + (world_face_rot.v1.x - world_face_rot.v0.x) * t;
                        v[0].y = world_face_rot.v0.y + (world_face_rot.v1.y - world_face_rot.v0.y) * t;
                        v[0].z = NEAR_Z;
                        uv[0].u = world_face_rot.uv[0].u + (world_face_rot.uv[1].u - world_face_rot.uv[0].u) * t;
                        uv[0].v = world_face_rot.uv[0].v + (world_face_rot.uv[1].v - world_face_rot.uv[0].v) * t;
                    }
                }
                else
                {
                    v[0] = world_face_rot.v0;
                }
                if (verts_to_clip & 0b10)
                { // Clip v1
                    if (verts_to_clip & 0b1)
                    {                                                                                           // v0 will be clipped so use v2
                        float t = (NEAR_Z - world_face_rot.v1.z) / (world_face_rot.v2.z - world_face_rot.v1.z); // Distance along v1->v2 where z == NEAR_Z
                        v[1].x = world_face_rot.v1.x + (world_face_rot.v2.x - world_face_rot.v1.x) * t;
                        v[1].y = world_face_rot.v1.y + (world_face_rot.v2.y - world_face_rot.v1.y) * t;
                        v[1].z = NEAR_Z;
                        uv[1].u = world_face_rot.uv[1].u + (world_face_rot.uv[2].u - world_face_rot.uv[1].u) * t;
                        uv[1].v = world_face_rot.uv[1].v + (world_face_rot.uv[2].v - world_face_rot.uv[1].v) * t;
                    }
                    else
                    {                                                                                           // v2 will be clipped so use v0
                        float t = (NEAR_Z - world_face_rot.v1.z) / (world_face_rot.v0.z - world_face_rot.v1.z); // Distance along v1->v0 where z == NEAR_Z
                        v[1].x = world_face_rot.v1.x + (world_face_rot.v0.x - world_face_rot.v1.x) * t;
                        v[1].y = world_face_rot.v1.y + (world_face_rot.v0.y - world_face_rot.v1.y) * t;
                        v[1].z = NEAR_Z;
                        uv[1].u = world_face_rot.uv[1].u + (world_face_rot.uv[0].u - world_face_rot.uv[1].u) * t;
                        uv[1].v = world_face_rot.uv[1].v + (world_face_rot.uv[0].v - world_face_rot.uv[1].v) * t;
                    }
                }
                else
                {
                    v[1] = world_face_rot.v1;
                }
                if (verts_to_clip & 0b100)
                { // Clip v2
                    if (verts_to_clip & 0b1)
                    {                                                                                           // v0 will be clipped so use v1
                        float t = (NEAR_Z - world_face_rot.v2.z) / (world_face_rot.v1.z - world_face_rot.v2.z); // Distance along v2->v1 where z == NEAR_Z
                        v[2].x = world_face_rot.v2.x + (world_face_rot.v1.x - world_face_rot.v2.x) * t;
                        v[2].y = world_face_rot.v2.y + (world_face_rot.v1.y - world_face_rot.v2.y) * t;
                        v[2].z = NEAR_Z;
                        uv[2].u = world_face_rot.uv[2].u + (world_face_rot.uv[1].u - world_face_rot.uv[2].u) * t;
                        uv[2].v = world_face_rot.uv[2].v + (world_face_rot.uv[1].v - world_face_rot.uv[2].v) * t;
                    }
                    else
                    {                                                                                           // v2 will be clipped so use v0
                        float t = (NEAR_Z - world_face_rot.v2.z) / (world_face_rot.v0.z - world_face_rot.v2.z); // Distance along v1->v0 where z == NEAR_Z
                        v[2].x = world_face_rot.v2.x + (world_face_rot.v0.x - world_face_rot.v2.x) * t;
                        v[2].y = world_face_rot.v2.y + (world_face_rot.v0.y - world_face_rot.v2.y) * t;
                        v[2].z = NEAR_Z;
                        uv[2].u = world_face_rot.uv[2].u + (world_face_rot.uv[0].u - world_face_rot.uv[2].u) * t;
                        uv[2].v = world_face_rot.uv[2].v + (world_face_rot.uv[0].v - world_face_rot.uv[2].v) * t;
                    }
                }
                else
                {
                    v[2] = world_face_rot.v2;
                }
                cam_faces[cam_it].v0 = v[0];
                cam_faces[cam_it].v1 = v[1];
                cam_faces[cam_it].v2 = v[2];
                cam_faces[cam_it].c = world_face_rot.c;
                cam_faces[cam_it].flags = world_face_rot.flags;
                cam_faces[cam_it].texture_index = world_face_rot.texture_index;
                cam_faces[cam_it].uv[0].u = uv[0].u;
                cam_faces[cam_it].uv[1].u = uv[1].u;
                cam_faces[cam_it].uv[2].u = uv[2].u;
                cam_faces[cam_it].uv[0].v = uv[0].v;
                cam_faces[cam_it].uv[1].v = uv[1].v;
                cam_faces[cam_it].uv[2].v = uv[2].v;
                ++cam_it;
            }
            else
            { // Should have already clipped this before so fail assertion.
                assert(false);
            }
        }
        num_cam_faces = cam_it;
        ProfileTime("World->Cam");

        int view_it = cam_it = 0;
        // transform view verts to screen space
        for (; view_it < num_cam_faces; ++view_it, ++cam_it)
        {
            for (int v = 0; v < 3; ++v)
            {
                view_faces[view_it].v[v].x = (cam_faces[cam_it].v[v].x / cam_faces[cam_it].v[v].z + 1) * internal_resolution_width / 2;
                view_faces[view_it].v[v].y = (aspect_ratio * -cam_faces[cam_it].v[v].y / cam_faces[cam_it].v[v].z + 1) * internal_resolution_height / 2;
                view_faces[view_it].v[v].z = NEAR_Z / cam_faces[cam_it].v[v].z;
            }
            view_faces[view_it].c = cam_faces[cam_it].c;
            view_faces[view_it].flags = cam_faces[cam_it].flags;
            view_faces[view_it].texture_index = cam_faces[cam_it].texture_index;
            view_faces[view_it].uv[0].u = cam_faces[cam_it].uv[0].u / cam_faces[cam_it].v0.z;
            view_faces[view_it].uv[1].u = cam_faces[cam_it].uv[1].u / cam_faces[cam_it].v1.z;
            view_faces[view_it].uv[2].u = cam_faces[cam_it].uv[2].u / cam_faces[cam_it].v2.z;
            view_faces[view_it].uv[0].v = cam_faces[cam_it].uv[0].v / cam_faces[cam_it].v0.z;
            view_faces[view_it].uv[1].v = cam_faces[cam_it].uv[1].v / cam_faces[cam_it].v1.z;
            view_faces[view_it].uv[2].v = cam_faces[cam_it].uv[2].v / cam_faces[cam_it].v2.z;
        }
        num_view_faces = view_it;
        ProfileTime("Cam->View");

        // render wireframe to screen
        for (int i = 0; i < num_view_faces; ++i)
        {
            // K3D_DrawTriangleWire(&frame_buffer, view_faces[i].v0, view_faces[i].v1, view_faces[i].v2, view_faces[i].c);
            if (view_faces[i].texture_index < MAX_TEXTURES)
            {
                K3D_DrawTriangleTextured(&render_frame, depth_buffer, &textures[view_faces[i].texture_index], view_faces[i].v0, view_faces[i].v1, view_faces[i].v2, view_faces[i].uv);
            }
            else
            {
                K3D_DrawTriangle(&render_frame, depth_buffer, view_faces[i].v0, view_faces[i].v1, view_faces[i].v2, view_faces[i].c);
            }
            // KS_DrawTriangle(&frame_buffer, view_faces[i].v0.x, view_faces[i].v0.y, view_faces[i].v1.x, view_faces[i].v1.y, view_faces[i].v2.x, view_faces[i].v2.y, view_faces[i].c);
            // K3D_DrawTrianglef(&frame_buffer, view_faces[i].v0, view_faces[i].v1, view_faces[i].v2, view_faces[i].c);
            /*KS_DrawLine(&frame_buffer, view_faces[i].v0.x, view_faces[i].v0.y, view_faces[i].v1.x, view_faces[i].v1.y, view_faces[i].c, KSSetPixel);
            KS_DrawLine(&frame_buffer, view_faces[i].v1.x, view_faces[i].v1.y, view_faces[i].v2.x, view_faces[i].v2.y, view_faces[i].c, KSSetPixel);
            KS_DrawLine(&frame_buffer, view_faces[i].v2.x, view_faces[i].v2.y, view_faces[i].v0.x, view_faces[i].v0.y, view_faces[i].c, KSSetPixel);*/
        }
        ProfileTime("View->Screen");

        if (draw_minimap)
        {
            KS_BlitScaledSafe(&maze_sprite, &frame_buffer, frame_buffer.w - maze_sprite.w * 5, 0, 5, 5, 0, 0);
            // Draw player on minimap
            KS_DrawRectFilled(&frame_buffer, cam.pos.x * 5 + frame_buffer.w - maze_sprite.w * 5, (maze_sprite.h - 1) * 5 - cam.pos.z * 5, cam.pos.x * 5 + frame_buffer.w - maze_sprite.w * 5 + 5, (maze_sprite.h - 1) * 5 - cam.pos.z * 5 + 5, 0xff000000);
            KS_DrawLine(&frame_buffer, cam.pos.x * 5 + frame_buffer.w - maze_sprite.w * 5 + 2, (maze_sprite.h - 1) * 5 - cam.pos.z * 5 + 2, cam.pos.x * 5 + frame_buffer.w - maze_sprite.w * 5 + 2 + 5 * sin(-cam.yaw + 3.f * PI / 4.f) - 5 * cos(-cam.yaw + 3.f * PI / 4.f), (maze_sprite.h - 1) * 5 - cam.pos.z * 5 + 2 + 5 * cos(-cam.yaw + 3.f * PI / 4.f) + 5 * sin(-cam.yaw + 3.f * PI / 4.f), 0xffffffff);
            ProfileTime("Draw minimap");
        }

        if (draw_profiles)
        {
            profile_frames[current_profile_frame].num_profiles = new_profile_frame.num_profiles;
            for (int profile_it = 0; profile_it < new_profile_frame.num_profiles; ++profile_it)
            {
                profile_frames[current_profile_frame].profiles[profile_it] = new_profile_frame.profiles[profile_it];
            }
            double start_time = new_profile_frame.profiles[0];
            for (int profile_it = 0; profile_it < profile_frames[current_profile_frame].num_profiles; ++profile_it)
            {
                char final_string[128];
                sprintf(final_string, "%.2f %s", profile_frames[current_profile_frame].profiles[profile_it] - (profile_it > 0 ? profile_frames[current_profile_frame].profiles[profile_it - 1] : start_time), profile_time_names[profile_it]);
                KF_DrawColored(&font, &render_frame, MAX_PROFILE_FRAMES * 2, profile_it * 16, final_string, profile_colours[profile_it]);
            }
            char final_string[128];
            sprintf(final_string, "%.2f %s", profile_frames[current_profile_frame].profiles[profile_frames[current_profile_frame].num_profiles - 1] - profile_frames[current_profile_frame].profiles[0], "Frame time");
            KF_Draw(&font, &render_frame, MAX_PROFILE_FRAMES * 2, profile_frames[current_profile_frame].num_profiles * 16, final_string);
            for (int profile_frame_it = 0; profile_frame_it < MAX_PROFILE_FRAMES; ++profile_frame_it)
            {
                double start_time = profile_frames[profile_frame_it].profiles[0];
                for (int profile_frame_profile_it = 1; profile_frame_profile_it < profile_frames[profile_frame_it].num_profiles; ++profile_frame_profile_it)
                {
                    KS_DrawLineVerticalSafe(&render_frame, profile_frame_it * 2, (profile_frames[profile_frame_it].profiles[profile_frame_profile_it - 1] - start_time) * 10, (profile_frames[profile_frame_it].profiles[profile_frame_profile_it] - start_time) * 10, profile_colours[profile_frame_profile_it]);
                }
            }
            ProfileTime("Draw profiles");
        }

#if 0
        char str[256];
        sprintf(str, "%d", (int)(cam.yaw/PI*180.f));
        KF_Draw(&font, &render_frame, 0, 0, str);
#endif

        float frame_scale = Min((float)frame_buffer.h / (float)render_frame.h, (float)frame_buffer.w / (float)render_frame.w);
        KS_BlitScaled(&render_frame, &frame_buffer, frame_buffer.w / 2, frame_buffer.h / 2, frame_scale, frame_scale, render_frame.w / 2, render_frame.h / 2);

        if (draw_framerate)
        {
            char str[256];
            sprintf(str, "%.0f", 1.f / platform.delta);
            KF_Draw(&font, &frame_buffer, frame_buffer.w - 48, 0, str);
        }

        for (int i = 0; i < top_message; ++i)
        {
            player_messages[i].time -= platform.delta;
            if (player_messages[i].time < 0)
            {
                --top_message;
                for (int j = i; j < top_message; ++j)
                {
                    player_messages[j].time = player_messages[j + 1].time;
                    for (int k = 0; k < 127 && (player_messages[j].text[k] = player_messages[j + 1].text[k]); ++k)
                        ;
                }
            }
            else
            {
                KF_Draw(&font, &frame_buffer, 0, i * 16, player_messages[i].text);
            }
        }

        KP_Flip(&platform);

        if (auto_launch_menu)
        {
            Menu();
        }

        ProfileTime("Flip");
        new_profile_frame.num_profiles = current_profile_time;
        for (int profile_it = 0; profile_it < current_profile_time; ++profile_it)
        {
            new_profile_frame.profiles[profile_it] = profile_times[profile_it];
        }
        current_profile_frame = (current_profile_frame + 1) % MAX_PROFILE_FRAMES;
        current_profile_time = 0;
    }

    return 0;
}
