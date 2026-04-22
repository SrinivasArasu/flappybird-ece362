#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdint.h>

const int TFT_SCK = 18;
const int TFT_CS = 17;
const int TFT_TX = 19;
const int TFT_DC = 20; //data or cmd select pin only thing we didnt use in labs

void display_init_spi()
{
    gpio_set_function(TFT_SCK, GPIO_FUNC_SPI);
    gpio_set_function(TFT_TX, GPIO_FUNC_SPI);

    spi_init(spi0, 30000000); // fastr SPI cant figure out flickering isssue tho
    spi_set_format(spi0, 8, 0, 0, SPI_MSB_FIRST);

    gpio_init(TFT_CS);
    gpio_set_dir(TFT_CS, GPIO_OUT);
    gpio_put(TFT_CS, 1); //not selected has to be 0 to be selected just left idle for now since not transmitting

    gpio_init(TFT_DC);
    gpio_set_dir(TFT_DC, GPIO_OUT);
    gpio_put(TFT_DC, 1);
}

void tft_cmd(uint8_t cmd) //for sending the command
{
    gpio_put(TFT_DC, 0);
    gpio_put(TFT_CS, 0);

    spi_write_blocking(spi0, &cmd, 1);

    gpio_put(TFT_CS, 1); //deselect again
}

void tft_data(uint8_t data) //for sending data for command
{
    gpio_put(TFT_DC, 1);
    gpio_put(TFT_CS, 0);

    spi_write_blocking(spi0, &data, 1); //1 byte write

    gpio_put(TFT_CS, 1); //deselect again
}

void tft_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    tft_cmd(0x2A);

    tft_data(x0 >> 8);
    tft_data(x0 & 0xFF);
    tft_data(x1 >> 8);
    tft_data(x1 & 0xFF);

    tft_cmd(0x2B);

    tft_data(y0 >> 8);
    tft_data(y0 & 0xFF);
    tft_data(y1 >> 8);
    tft_data(y1 & 0xFF);

    tft_cmd(0x2C);
}

void draw_rect(int x,int y,int w,int h,uint16_t color)
{
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    
    if (x >= 320 || y >= 240) return; //out of border so return

    if (x + w > 320) w = 320 - x; //clipping logic
    if (y + h > 240) h = 240 - y;

    if (w <= 0 || h <= 0) return;
    tft_set_addr_window(x,y,x+w-1,y+h-1);

    gpio_put(TFT_DC,1);
    gpio_put(TFT_CS,0);

    uint8_t data[2] = {color>>8,color&0xFF};

    for(int i=0;i<w*h;i++) //filling the area
    {
        spi_write_blocking(spi0,data,2);
    }

    gpio_put(TFT_CS,1);
}

void tft_fill_screen(uint16_t color) 
{
    draw_rect(0,0,320,240,color);
}
