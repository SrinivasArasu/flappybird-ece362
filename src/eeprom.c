#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c1
#define SDA_PIN 26
#define SCL_PIN 27
#define EEPROM_ADDR 0x50 //because a0a1a2 are all grounded

void init_i2c()
{
    i2c_init(I2C_PORT, 100000);

    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN); //game does not go from death state to restart state after button press if these two arent pulled up
    gpio_pull_up(SCL_PIN);
}

void save_high_score(int score)
{
    uint8_t data[4];

    printf("Saving HS: %d\n", score); //debug print

    data[0] = 0x00;
    data[1] = 0x00;

    // actual data (big endian) (little end >>> but whatever)
    data[2] = (score >> 8) & 0xFF; 
    data[3] = score & 0xFF;

    int ret = i2c_write_blocking(I2C_PORT, EEPROM_ADDR, data, 4, false); // writes the formatted buffer to the eeprom, returns the number of bytes written

    printf("Write ret: %d\n", ret); //debug print to make sure writing happens

    sleep_ms(20); // write cycle wait time
}

int load_high_score()
{
    uint8_t addr[2] = {0x00, 0x00}; // 2-byte address
    uint8_t data[2];

    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, addr, 2, true);
    i2c_read_blocking(I2C_PORT, EEPROM_ADDR, data, 2, false);

    int score = (data[0] << 8) | data[1];

    if(score < 0 || score > 1000) //to prevent bad things
        score = 0;

    return score;
}
