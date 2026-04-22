#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#define AUDIO_PIN 15 

void audio_init(void);
void audio_update(void);
void play_jump_sound(void);
void play_death_sound(void);
void play_highscore_sound(void);
void set_bgm_playing(bool play);

#endif