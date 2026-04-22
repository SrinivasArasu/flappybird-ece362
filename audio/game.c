#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
//// #include "audio.h"

#define SCREEN_WIDTH 320 // tried randomly and this works
#define SCREEN_HEIGHT 240 //xxx

#define BIRD_SIZE 8
#define PIPE_WIDTH 20
#define PIPE_GAP 80
#define PIPE_SPEED 5

#define GRAVITY 1
#define JUMP_FORCE -8

#define PIPE_COUNT 3 

#define COLOR_BG 0x0000
#define COLOR_BIRD 0xFFE0
#define COLOR_PIPE 0x07E0

extern void tft_fill_screen(uint16_t color);
extern void draw_rect(int x,int y,int w,int h,uint16_t color);

typedef struct
{
    int x;
    int y;
    int velocity;
} Bird;

typedef struct
{
    int x;
    int gap_y;
} Pipe;

Bird bird;
Pipe pipes[PIPE_COUNT];

bool game_over=false;

void game_init()
{
    bird.x=60;
    bird.y=160;
    bird.velocity=0;

    for(int i=0;i<PIPE_COUNT;i++)
    {
        pipes[i].x=SCREEN_WIDTH+i*120;
        pipes[i].gap_y = rand() % (SCREEN_HEIGHT - PIPE_GAP - 20) + 10; //do not change this it breaks
    }

    ////score = 0;
    ////set_bgm_playing(true);
    game_over=false;
}

void bird_jump()
{
    if(!game_over)
        bird.velocity=JUMP_FORCE;
        ////play_jump_sound();
}

void update_bird()
{
    bird.velocity+=GRAVITY;
    bird.y+=bird.velocity;

    if(bird.y<0)
        bird.y=0;

    if(bird.y>SCREEN_HEIGHT-BIRD_SIZE)
        game_over=true;
}

void update_pipes()
{
    for(int i=0;i<PIPE_COUNT;i++)
    {
        pipes[i].x-=PIPE_SPEED;

        ////if (pipes[i].x == bird.x) score++;

        if(pipes[i].x<-PIPE_WIDTH)
        {
            pipes[i].x=SCREEN_WIDTH;
            pipes[i].gap_y = rand() % (SCREEN_HEIGHT - PIPE_GAP - 20) + 10; //do not change this it breaks
        }
    }
}

bool check_collision()
{
    for(int i=0;i<PIPE_COUNT;i++)
    {
        if(bird.x+BIRD_SIZE>pipes[i].x &&
           bird.x<pipes[i].x+PIPE_WIDTH)
        {
            if(bird.y<pipes[i].gap_y ||
               bird.y+BIRD_SIZE>pipes[i].gap_y+PIPE_GAP)
            {
                return true;
            }
        }
    }

    return false;
}

void game_update()
{
    if(game_over) return;

    update_bird();
    update_pipes();

    if(check_collision())
    {
        game_over=true;
        ////set_bgm_playing(false); // Stop music on death
        
        ////if(update_high_score(score)) {
            ////play_highscore_sound();
        ////} else {
            ////play_death_sound();
        ////}
    }
}

void draw_bird()
{
    draw_rect(bird.x,bird.y,BIRD_SIZE,BIRD_SIZE,COLOR_BIRD);
}

void draw_pipes()
{
    for(int i=0;i<PIPE_COUNT;i++)
    {
        int x=pipes[i].x;
        int gap=pipes[i].gap_y;

        draw_rect(x,0,PIPE_WIDTH,gap,COLOR_PIPE);

        draw_rect(x,
                  gap+PIPE_GAP,
                  PIPE_WIDTH,
                  SCREEN_HEIGHT-(gap+PIPE_GAP),
                  COLOR_PIPE);
    }
}

void game_draw()
{
    tft_fill_screen(COLOR_BG);

    draw_pipes();
    draw_bird();
}