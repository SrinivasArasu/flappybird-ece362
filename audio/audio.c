#include "audio.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

typedef enum {
    SND_NONE,
    SND_JUMP,
    SND_DEATH,
    SND_HIGHSCORE
} SoundEffect;

static SoundEffect current_sfx = SND_NONE;
static int sfx_timer = 0;
static bool bgm_playing = false;

// Simple BGM melody 
static const int bgm_notes[] = {261, 0, 261, 329, 392, 0, 392, 0}; 
// Duration in 20ms frames 
static const int bgm_durations[] = {10, 5, 10, 10, 15, 5, 15, 20}; 
static int bgm_step = 0;
static int bgm_timer = 0;

void audio_init(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    
    pwm_set_clkdiv(slice_num, 125.0f); 
}

static void play_tone(int freq) {
    uint slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    if (freq == 0) {
        pwm_set_gpio_level(AUDIO_PIN, 0);
        pwm_set_enabled(slice_num, false);
    } else {
        uint32_t wrap = 1000000 / freq;
        pwm_set_wrap(slice_num, wrap);
        pwm_set_gpio_level(AUDIO_PIN, wrap / 2); 
        pwm_set_enabled(slice_num, true);
    }
}

void audio_update(void) {
    // Sound Effects override BGM
    if (current_sfx != SND_NONE) {
        if (current_sfx == SND_JUMP) {
            if (sfx_timer == 0) play_tone(600);
            if (sfx_timer == 2) play_tone(800);
            if (sfx_timer == 5) { play_tone(0); current_sfx = SND_NONE; }
        }
        else if (current_sfx == SND_DEATH) {
            if (sfx_timer == 0) play_tone(300);
            if (sfx_timer == 10) play_tone(250);
            if (sfx_timer == 20) play_tone(200);
            if (sfx_timer == 30) play_tone(150);
            if (sfx_timer == 40) { play_tone(0); current_sfx = SND_NONE; }
        }
        else if (current_sfx == SND_HIGHSCORE) {
            if (sfx_timer == 0) play_tone(523);  
            if (sfx_timer == 8) play_tone(659); 
            if (sfx_timer == 16) play_tone(784); 
            if (sfx_timer == 24) play_tone(1046);
            if (sfx_timer == 40) { play_tone(0); current_sfx = SND_NONE; }
        }
        sfx_timer++;
        return; // Skip BGM processing while SFX is active
    }

    // Background Music Loop
    if (bgm_playing) {
        if (bgm_timer == 0) {
            play_tone(bgm_notes[bgm_step]);
        }
        bgm_timer++;
        
        if (bgm_timer >= bgm_durations[bgm_step]) {
            bgm_timer = 0;
            bgm_step++;
            if (bgm_step >= (sizeof(bgm_notes) / sizeof(bgm_notes[0]))) {
                bgm_step = 0; // Loop melody
            }
        }
    } else {
        play_tone(0);
    }
}

void play_jump_sound(void) { current_sfx = SND_JUMP; sfx_timer = 0; }
void play_death_sound(void) { current_sfx = SND_DEATH; sfx_timer = 0; }
void play_highscore_sound(void) { current_sfx = SND_HIGHSCORE; sfx_timer = 0; }

void set_bgm_playing(bool play) { 
    if (bgm_playing == play) return; 
    bgm_playing = play; 
    if (!play) play_tone(0); 
    else { bgm_step = 0; bgm_timer = 0; } 
}