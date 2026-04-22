#include "pico/stdlib.h"
#include "hardware/adc.h"

#define JOY_ADC_PIN 40
#define JOY_ADC_CHAN 0

void adc_setup() //init function
{
    adc_init();
    adc_gpio_init(JOY_ADC_PIN);
    adc_select_input(JOY_ADC_CHAN);
}

uint16_t adc_read_joystick()
{
    return adc_read(); //0v means up, 2000ish means idle 4000 means down
}
