#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define BTN_RESET 16 //joystick reset button

static bool last_reset = false; //like histroy byte?

void display_init_spi();
void tft_cmd(uint8_t cmd);
void tft_data(uint8_t data);

void game_init();
void game_update(uint16_t adc_val);
void game_draw();

// ADC functions
void adc_setup();
uint16_t adc_read_joystick();



void audio_init(void);
void audio_update(void);
void play_jump_sound(void);
void play_death_sound(void);
void play_highscore_sound(void);
void set_bgm_playing(bool play);


void init_i2c(); //eeprom stuff


int main()
{
    stdio_init_all();
    init_i2c();   // for EEPROM
    display_init_spi();

    audio_init(); //for audio

    // TFT INIT
    //tft cmd - do this cmd
    //tft data - data for said command
    tft_cmd(0x01); //reset
    sleep_ms(150);

    tft_cmd(0x11); //sleep
    sleep_ms(150);

    tft_cmd(0x3A); //pixel format
    tft_data(0x55); // 16 bit color

    tft_cmd(0x36);
    tft_data(0xE8); // correct orientation

    tft_cmd(0x29); // display on

    // RESET BUTTON
    gpio_init(BTN_RESET);
    gpio_set_dir(BTN_RESET, GPIO_IN);
    gpio_pull_down(BTN_RESET);

    // ADC INIT
    adc_setup();

    game_init();

    while (1)
    {
        bool reset_pressed = gpio_get(BTN_RESET);   // invert? nah this works leave it

        if(reset_pressed && !last_reset)
        {
            game_init();
            sleep_ms(200);
        }

        last_reset = reset_pressed;

        uint16_t joy = adc_read_joystick();

        game_update(joy);
        game_draw();

        audio_update(); //audio stuff
        sleep_ms(20); // ~50 fps?
    }
}
