#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include <stdint.h>

#define AUDIO_PIN 36
#define SAMPLE_RATE 16000 // 16 kHz sample rate

// 256-step Sine Wave Lookup Table (Values 0-255) resorted to this cuz we tried to play freq by freq (tone by tone) and it was BAD
static const uint8_t sine_table[256] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 156, 159, 162, 165, 168, 171, 174,
    176, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216,
    218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 240, 242, 243, 245,
    246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 254, 254, 253, 252, 252, 251, 250, 249, 248, 247, 246,
    245, 243, 242, 240, 239, 237, 236, 234, 232, 230, 228, 226, 224, 222, 220, 218,
    216, 213, 211, 209, 206, 204, 201, 199, 196, 193, 191, 188, 185, 182, 179, 176,
    174, 171, 168, 165, 162, 159, 156, 152, 149, 146, 143, 140, 137, 134, 131, 128,
    124, 121, 118, 115, 112, 109, 106, 103,  99,  96,  93,  90,  87,  84,  81,  79,
     76,  73,  70,  67,  64,  62,  59,  56,  54,  51,  49,  46,  44,  42,  39,  37,
     35,  33,  31,  29,  27,  25,  23,  21,  19,  18,  16,  15,  13,  12,  10,   9,
      8,   7,   6,   5,   4,   3,   3,   2,   1,   1,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   1,   1,   2,   3,   3,   4,   5,   6,   7,   8,   9,  10,
     12,  13,  15,  16,  18,  19,  21,  23,  25,  27,  29,  31,  33,  35,  37,  39,
     42,  44,  46,  49,  51,  54,  56,  59,  62,  64,  67,  70,  73,  76,  79,  81,
     84,  87,  90,  93,  96,  99, 103, 106, 109, 112, 115, 118, 121, 124, 128
};


static uint slice_num;
static struct repeating_timer audio_timer;


static uint16_t sfx_phase = 0; //for jump death whatever
static uint16_t sfx_phase_inc = 0;
static uint8_t  sfx_volume = 100;

//Background Music
static uint16_t bgm_phase = 0;
static uint16_t bgm_phase_inc = 0;
static uint8_t  bgm_volume = 60; // Keep BGM lower

typedef enum { SND_NONE, SND_JUMP, SND_DEATH, SND_HIGHSCORE } SoundEffect;
static SoundEffect current_sfx = SND_NONE;
static int sfx_timer = 0;
static bool bgm_playing = false;

// BGM Notes and Durations
static const int bgm_notes[] = { 261, 329, 392, 523, 392, 329, 261, 0 };
static const int bgm_durations[] = { 8, 8, 8, 12, 8, 8, 8, 8 };
static int bgm_step = 0;
static int bgm_timer = 0;

// ---------------- INTERRUPT TIMER ----------------
// This runs 16,000 times per second to push the wavetable to the PWM
bool audio_timer_callback(struct repeating_timer *t) {
    // ++ logic
    sfx_phase += sfx_phase_inc;
    bgm_phase += bgm_phase_inc;

    //Lookup the sine value (use top 8 bits of phase)
    uint8_t sfx_sample = sine_table[sfx_phase >> 8];
    uint8_t bgm_sample = sine_table[bgm_phase >> 8];

    // Apply volume const i defined
    uint32_t sfx_out = (sfx_sample * sfx_volume) >> 8;
    uint32_t bgm_out = (bgm_sample * bgm_volume) >> 8;

    //Mmix channels and send to pwm
    uint32_t mix = (sfx_out + bgm_out) >> 1; 
    pwm_set_gpio_level(AUDIO_PIN, mix);

    return true;
}

// ---------------- HELPER: SET FREQUENCY ----------------
static void set_sfx_freq(int freq, int volume) {
    sfx_volume = volume;
    if (freq == 0) sfx_phase_inc = 0;
    else sfx_phase_inc = (freq * 65536) / SAMPLE_RATE;
}

static void set_bgm_freq(int freq) {
    if (freq == 0) bgm_phase_inc = 0;
    else bgm_phase_inc = (freq * 65536) / SAMPLE_RATE;
}

// init
void audio_init(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);

    //debugged
    pwm_set_clkdiv(slice_num, 1.0f);
    pwm_set_wrap(slice_num, 255);
    pwm_set_enabled(slice_num, true);

    add_repeating_timer_us(-62, audio_timer_callback, NULL, &audio_timer);
}

// AUDIO update (Game Loop)
void audio_update(void) {
    // ---------- SFX ----------
    if (current_sfx != SND_NONE) {
        if (current_sfx == SND_JUMP) {
            int freq = 600 + sfx_timer * 120;
            int vol = 255 - (sfx_timer * 30); // Fade out
            if (vol < 0) vol = 0;
            
            set_sfx_freq(freq, vol);

            if (sfx_timer > 8) {
                set_sfx_freq(0, 0);
                current_sfx = SND_NONE;
            }
        }
        else if (current_sfx == SND_DEATH) {
            int freq = 400 - sfx_timer * 10;
            if (freq < 100) freq = 100;
            
            int vol = 255 - (sfx_timer * 6); // Slow fade
            if (vol < 0) vol = 0;

            set_sfx_freq(freq, vol);

            if (sfx_timer > 40) {
                set_sfx_freq(0, 0);
                current_sfx = SND_NONE;
            }
        }
        else if (current_sfx == SND_HIGHSCORE) {
            int notes[] = {523, 659, 784, 1046}; //random notes change if you want to
            int step = sfx_timer / 8;

            if (step < 4) {
                int local_timer = sfx_timer % 8;
                int vol = 255 - (local_timer * 20);
                if (vol < 0) vol = 0;
                
                set_sfx_freq(notes[step], vol);
            }
            else {
                set_sfx_freq(0, 0);
                current_sfx = SND_NONE;
            }
        }
        sfx_timer++;
    }

    // background music!
    if (bgm_playing) {
        set_bgm_freq(bgm_notes[bgm_step]); 

        bgm_timer++;
        if (bgm_timer >= bgm_durations[bgm_step]) {
            bgm_timer = 0;
            bgm_step++;
            if (bgm_step >= (sizeof(bgm_notes) / sizeof(bgm_notes[0])))
                bgm_step = 0;
        }
    } else {
        set_bgm_freq(0);
    }
}


void play_jump_sound(void) { current_sfx = SND_JUMP; sfx_timer = 0; }
void play_death_sound(void) { current_sfx = SND_DEATH; sfx_timer = 0; }
void play_highscore_sound(void) { current_sfx = SND_HIGHSCORE; sfx_timer = 0; }

void set_bgm_playing(bool play) {
    if (bgm_playing == play) return;
    bgm_playing = play;
    if (!play) set_bgm_freq(0);
    else { bgm_step = 0; bgm_timer = 0; }
}
