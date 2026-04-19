#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define BTN_JUMP 21
#define BTN_RESET 26

static bool last_jump = false;
static bool last_reset = false;

void display_init_spi();
void tft_cmd(uint8_t cmd);
void tft_data(uint8_t data);

void game_init();
void game_update();
void game_draw();
void bird_jump();

int main()
{
    stdio_init_all();

    display_init_spi();

    // TFT INIT
    tft_cmd(0x01);
    sleep_ms(150);

    tft_cmd(0x11);
    sleep_ms(150);

    tft_cmd(0x3A);
    tft_data(0x55); // 16 bit color

    tft_cmd(0x36);
    tft_data(0xE8); // correct orientation nothing else workls

    tft_cmd(0x29); // display on

    // BUTTONS
    gpio_init(BTN_JUMP);
    gpio_set_dir(BTN_JUMP, GPIO_IN);
    gpio_pull_down(BTN_JUMP);
    gpio_init(BTN_RESET);
    gpio_set_dir(BTN_RESET, GPIO_IN);
    gpio_pull_down(BTN_RESET);

    game_init();

    while (1)
    {
        bool jump_pressed = gpio_get(BTN_JUMP);
        bool reset_pressed = gpio_get(BTN_RESET);

        if(jump_pressed && !last_jump)
            bird_jump();

        if(reset_pressed && !last_reset)
            game_init();

        last_jump = jump_pressed;
        last_reset = reset_pressed;

        game_update();
        game_draw();

        sleep_ms(20); // 15fps
    }
}