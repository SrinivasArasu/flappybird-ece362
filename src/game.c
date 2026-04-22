#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#define SCREEN_WIDTH 320 //correctorder
#define SCREEN_HEIGHT 240

#define BIRD_SIZE 8
#define PIPE_WIDTH 20
#define PIPE_GAP 80
#define PIPE_SPEED 6

#define GRAVITY 1 //decimals wont work

#define PIPE_COUNT 3

#define COLOR_BG    0xBFDF //colors thoroughly vetted :)
#define COLOR_BIRD  0xFFE0
#define COLOR_PIPE  0x07A0
#define COLOR_RED   0xF800
#define COLOR_WHITE 0xFFFF
#define COLOR_BLACK 0x0000

#define HUD_HEIGHT 28 //little red bar on top - atharva's idea

int load_high_score(void);
void save_high_score(int score);
void set_bgm_playing(bool play);
extern void tft_fill_screen(uint16_t color);
extern void draw_rect(int x,int y,int w,int h,uint16_t color);

typedef struct { //struct for game data makes it easier to keep track of things ece264++
    int x, y, velocity;
    int old_x, old_y;
} Bird;

typedef struct {
    int x, gap_y;
    int old_x, old_gap_y;
    bool scored;
} Pipe;

Bird bird;
Pipe pipes[PIPE_COUNT];

bool game_over = false;
bool game_over_screen_drawn = false;

int gravity_delay = 25;
int score = 0;
int high_score = 0;

int last_drawn_score = -1; //stops from 
int last_drawn_high_score = -1;

// ---------------- INIT ----------------
void game_init()
{   
    //save_high_score(0); //to reset eeprom when demoing
    bird.x = 60;
    bird.y = 160;
    bird.velocity = 0;
    bird.old_x = bird.x;
    bird.old_y = bird.y;

    gravity_delay = 25; // to give users some time to jumpa nd doesnt autoimaticcally pull the bird down when game starts
    score = 0;
    high_score = load_high_score();   // loads score
    printf("Loaded HS: %d\n", high_score); //debug line

    last_drawn_score = -1;
    last_drawn_high_score = -1;

    for(int i=0;i<PIPE_COUNT;i++)
    {
        pipes[i].x = SCREEN_WIDTH + i*120;
        //random gap position v
        pipes[i].gap_y = rand() % (SCREEN_HEIGHT - PIPE_GAP - HUD_HEIGHT - 20) + HUD_HEIGHT + 10; //please dont touch i dont know how to fix
        pipes[i].old_x = pipes[i].x;
        pipes[i].old_gap_y = pipes[i].gap_y;
        pipes[i].scored = false;
    }

    score = 0;
    set_bgm_playing(true); //background music

    game_over = false;
    
    game_over_screen_drawn = false;

    tft_fill_screen(COLOR_BG); //clear screen (the flash you see when i reset the game)
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxBIRD (stick + grav)xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void update_bird_from_adc(uint16_t adc_val)
{
    bird.old_x = bird.x;
    bird.old_y = bird.y;

    int center = 3000;
    int deadzone = 150; //accounting for deadzone

    int diff = adc_val - center;

    if(diff > deadzone || diff < -deadzone)
        bird.velocity += diff / 600;

    if(gravity_delay > 0)
        gravity_delay--;
    else
        bird.velocity += GRAVITY;

    if(bird.velocity > 6) bird.velocity = 6; //cap vel
    if(bird.velocity < -6) bird.velocity = -6;

    bird.y += bird.velocity;

    if(bird.y < HUD_HEIGHT) //hud is limit so stop there
    {
        bird.y = HUD_HEIGHT;
        bird.velocity = 0;
        //do you guys want to die when you hit the ceiling?
    }

    if(bird.y > SCREEN_HEIGHT - BIRD_SIZE) //bird falling to ground = dead
    {
        bird.y = SCREEN_HEIGHT - BIRD_SIZE;
        game_over = true;
    }
}

// xxxxxxxxxxxxxxxxxxxx PIPES xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void update_pipes()
{
    for(int i=0;i<PIPE_COUNT;i++)
    {
        pipes[i].old_x = pipes[i].x;
        pipes[i].old_gap_y = pipes[i].gap_y;

        pipes[i].x -= PIPE_SPEED;

        if(pipes[i].x < -PIPE_WIDTH)
        {
            pipes[i].x = SCREEN_WIDTH;
            pipes[i].gap_y = rand() % (SCREEN_HEIGHT - PIPE_GAP - HUD_HEIGHT - 20) + HUD_HEIGHT + 10; //please dont touch i dont know how to fix 
            pipes[i].scored = false;
        }
    }
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx COLLISION xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
bool check_collision()
{
    for(int i=0;i<PIPE_COUNT;i++)
    {
        if(bird.x+BIRD_SIZE > pipes[i].x &&
           bird.x < pipes[i].x+PIPE_WIDTH)
        {
            if(bird.y < pipes[i].gap_y || bird.y+BIRD_SIZE > pipes[i].gap_y+PIPE_GAP)
                return true;
        }
    }
    return false;
}

// game logic update function
void game_update(uint16_t adc_val)
{
    if(game_over) return;

    update_bird_from_adc(adc_val);
    update_pipes();

    //add to score when passing pipes
    for(int i=0;i<PIPE_COUNT;i++)
    {
        if(!pipes[i].scored && pipes[i].x+PIPE_WIDTH < bird.x)
        {
            pipes[i].scored = true;
            score++;
            if(score > high_score)
            {
                high_score = score;
                save_high_score(high_score);
            }
        }
    }

    if(check_collision()) //die logic
    {
        game_over = true;
    
        if(score > high_score)
        {
            high_score = score;
            save_high_score(high_score);
        }
    }
}

void draw_bird()
{
    draw_rect(bird.x,bird.y,BIRD_SIZE,BIRD_SIZE,COLOR_BIRD);
}

void erase_bird()
{
    draw_rect(bird.old_x,bird.old_y,BIRD_SIZE,BIRD_SIZE,COLOR_BG);
}

void draw_pipes()
{
    for(int i=0;i<PIPE_COUNT;i++)
    {
        int x = pipes[i].x;
        int g = pipes[i].gap_y;

        if(g > HUD_HEIGHT) //top
            draw_rect(x,HUD_HEIGHT,PIPE_WIDTH,g-HUD_HEIGHT,COLOR_PIPE);

        draw_rect(x,g,PIPE_WIDTH,PIPE_GAP,COLOR_BG); //gap

        draw_rect(x,g+PIPE_GAP,PIPE_WIDTH, SCREEN_HEIGHT-(g+PIPE_GAP),COLOR_PIPE); //bottom part of pipe
    }
}

void erase_pipes()
{
    for(int i=0;i<PIPE_COUNT;i++)
    {
        int x = pipes[i].x + PIPE_WIDTH;
        int w = PIPE_SPEED;

        if(x < 0 || x >= SCREEN_WIDTH) continue;

        if(x+w > SCREEN_WIDTH) w = SCREEN_WIDTH-x;

        draw_rect(x,HUD_HEIGHT,w,SCREEN_HEIGHT-HUD_HEIGHT,COLOR_BG);
    }
}

// TEXT DISPLAYING
static void draw_digit(int x,int y,int d,int s,uint16_t c) //for score
{
    static const uint8_t f[10][5]={
        {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},
        {5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},
        {7,5,7,5,7},{7,5,7,1,7}
    };

    for(int r=0;r<5;r++)
        for(int col=0;col<3;col++)
            if(f[d][r]&(1<<(2-col)))
                draw_rect(x+col*s,y+r*s,s,s,c);
}

static void draw_number(int x,int y,int v,int s,uint16_t c)
{
    if(v==0){draw_digit(x,y,0,s,c);return;}

    int d[10],n=0;
    while(v){d[n++]=v%10;v/=10;}

    for(int i=n-1;i>=0;i--){
        draw_digit(x,y,d[i],s,c);
        x+=s*4;
    }
}

static void draw_char(int x,int y,char c,int scale,uint16_t color)
{
    // same as old file
    //for displaying game over, score, high
    int s = scale;

    switch(c)
    {
        case 'A':
            draw_rect(x, y + s, s, 5*s, color);
            draw_rect(x + 4*s, y + s, s, 5*s, color);
            draw_rect(x + s, y, 3*s, s, color);
            draw_rect(x + s, y + 3*s, 3*s, s, color);
            break;

        case 'C':
            draw_rect(x + s, y, 4*s, s, color);
            draw_rect(x, y + s, s, 4*s, color);
            draw_rect(x + s, y + 5*s, 4*s, s, color);
            break;

        case 'E':
            draw_rect(x, y, s, 6*s, color);
            draw_rect(x + s, y, 4*s, s, color);
            draw_rect(x + s, y + 3*s, 3*s, s, color);
            draw_rect(x + s, y + 5*s, 4*s, s, color);
            break;

        case 'G':
            draw_rect(x + s, y, 4*s, s, color);
            draw_rect(x, y + s, s, 4*s, color);
            draw_rect(x + s, y + 5*s, 4*s, s, color);
            draw_rect(x + 3*s, y + 3*s, 2*s, s, color);
            draw_rect(x + 4*s, y + 3*s, s, 2*s, color);
            break;

        case 'H':
            draw_rect(x, y, s, 6*s, color);
            draw_rect(x + 4*s, y, s, 6*s, color);
            draw_rect(x + s, y + 3*s, 3*s, s, color);
            break;

        case 'I':
            draw_rect(x, y, 5*s, s, color);
            draw_rect(x + 2*s, y, s, 6*s, color);
            draw_rect(x, y + 5*s, 5*s, s, color);
            break;

        case 'M':
            draw_rect(x, y, s, 6*s, color);
            draw_rect(x + 4*s, y, s, 6*s, color);
            draw_rect(x + s, y + s, s, s, color);
            draw_rect(x + 3*s, y + s, s, s, color);
            draw_rect(x + 2*s, y + 2*s, s, s, color);
            break;

        case 'O':
            draw_rect(x + s, y, 3*s, s, color);
            draw_rect(x, y + s, s, 4*s, color);
            draw_rect(x + 4*s, y + s, s, 4*s, color);
            draw_rect(x + s, y + 5*s, 3*s, s, color);
            break;

        case 'R':
            draw_rect(x, y, s, 6*s, color);
            draw_rect(x + s, y, 3*s, s, color);
            draw_rect(x + 4*s, y + s, s, 2*s, color);
            draw_rect(x + s, y + 3*s, 3*s, s, color);
            draw_rect(x + 3*s, y + 4*s, s, s, color);
            draw_rect(x + 4*s, y + 5*s, s, s, color);
            break;

        case 'S':
            draw_rect(x + s, y, 4*s, s, color);
            draw_rect(x, y + s, s, 2*s, color);
            draw_rect(x + s, y + 3*s, 3*s, s, color);
            draw_rect(x + 4*s, y + 4*s, s, s, color);
            draw_rect(x, y + 5*s, 4*s, s, color);
            break;

        case 'V':
            draw_rect(x, y, s, 4*s, color);
            draw_rect(x + 4*s, y, s, 4*s, color);
            draw_rect(x + s, y + 4*s, s, s, color);
            draw_rect(x + 3*s, y + 4*s, s, s, color);
            draw_rect(x + 2*s, y + 5*s, s, s, color);
            break;

        case ' ':
            break;
    }
}

static void draw_text(int x,int y,const char *t,int s,uint16_t c)
{
    while(*t){draw_char(x,y,*t,s,c);x+=s*7;t++;}
}

// hud
static void draw_hud(void)
{
    if(score==last_drawn_score && high_score==last_drawn_high_score)
        return;

    draw_rect(0,0,SCREEN_WIDTH,HUD_HEIGHT,COLOR_RED);

    draw_number(6,6,high_score,3,COLOR_WHITE);

    int temp=score,digits=1;
    while(temp>=10){temp/=10;digits++;}

    int width = digits*(3*3+3);
    draw_number(SCREEN_WIDTH-width-6,6,score,3,COLOR_WHITE);

    last_drawn_score=score;
    last_drawn_high_score=high_score;
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx GAME OVER xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
static void draw_game_over_screen(void)
{
    tft_fill_screen(COLOR_RED);

    draw_text(60,30,"GAME OVER",3,COLOR_WHITE);

    draw_text(60,100,"SCORE",2,COLOR_WHITE);
    draw_number(160,100,score,3,COLOR_BLACK);

    draw_text(60,160,"HIGH",2,COLOR_WHITE);
    draw_number(160,160,high_score,3,COLOR_BLACK);
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx game draw  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void game_draw()
{
    if(game_over)
    {
        if(!game_over_screen_drawn)
        {
            draw_game_over_screen();
            game_over_screen_drawn = true;
        }
        return;
    }

    erase_pipes();
    erase_bird();
    draw_pipes();
    draw_bird();
    draw_hud();
}
