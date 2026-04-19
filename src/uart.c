#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
// copy in defns for BUFSIZE, serbuf, seridx, newline_seen
// from UART lab, as well as functions below.

void init_uart() {
    // fill in
    // fill in
    // pg 339, uart_init takes in UART instance and the baud rate
    uart_init(uart0, 115200);
    
    // sets gpio pin 0 to be UART TX
    gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));
    // sets gpio pin 1 to be UART RX
    gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1));
    
    // sets the format to be 8 bits with 1 stop bit, and no parity bit
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);

    // enables fifo for use in step 3
    uart_set_fifo_enabled(uart0, true);
}

#define BUFSIZE 32
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

// add this here so that compiler does not complain about implicit function
void uart_rx_handler();

void init_uart_irq() {
    // fill in
    // disables fifo
    uart_set_fifo_enabled(uart0, false);
    // pg 979 of the datasheet
    // ISMC is the interrupt mask set/clear register
    // bit 4 is the RX bit, setting to 1 enables it
    uart0_hw->imsc |= (1 << 4);
    // uart0 irq number is 33, from pg 82 in chapter 3.2
    irq_set_exclusive_handler(33,uart_rx_handler);
    irq_set_enabled(33, true);
}

void uart_rx_handler() {
    // fill in
    // pg 981, icr register is the interrupt clear register
    // bit 4 corresponds to RX acknowledgement
    uart0_hw ->icr = (1 >> 4);
    if(seridx == BUFSIZE){
        return;
    }
    // pg 973, dr is just the data register
    char c = uart0_hw ->dr;
    if (c == 0x0A){
        newline_seen = 1;
    }
    if( c == 8 && seridx > 0){

        uart_putc_raw(uart0, 8);
        uart_putc_raw(uart0, 32);
        uart_putc_raw(uart0, 8);
        seridx--;
        serbuf[seridx] = '\0';

    }
    if(c != 8){

        uart_putc_raw(uart0, c);
        serbuf[seridx] = c;
        seridx++;
    }
}


int _read(__unused int handle, char *buffer, int length) {
    // fill in
    while(newline_seen == 0){
        sleep_ms(5);
    }
    if(newline_seen != 0){
        newline_seen = 0;
    }
    for (int i = 0; i < seridx; i++){
        buffer[i] = serbuf[i];
    }

    seridx = 0;

    return length;
}

int _write(__unused int handle, char *buffer, int length) {
    // fill in
     for (int i = 0; i < length; i++){
        uart_putc_raw(uart0, buffer[i]);
    }

    return length;
}