#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include <math.h>
#include <string.h>
#include "hardware/i2c.h"

//////////////////////////////////////////////////////////////////////////////

#define I2C_SDA 26
#define I2C_SCL 27

#if (I2C_SDA == 0) || (I2C_SCL == 0)
#error "Error: Set your I2C_SDA and I2C_SCL pins at the top of main.c."
#endif

// If you use different A2-A0 pin values, change EEPROM_ADDR accordingly.
// 0x50 requires that A2-A0 are all 0.
#define EEPROM_ADDR 0x50
#define EEPROM_PAGE_SIZE 32
#define HIGHSCORE_ADDR 0x0000

//////////////////////////////////////////////////////////////////////////////

void init_i2c() {
    i2c_init(i2c1, 400000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
}


void eeprom_write(uint16_t loc, const char* data, uint8_t len) {
    // if the length of the data is longer than the size of the eeprom page, an error message is thrown
    if (len > 32) {
        printf("Error: EEPROM page write max is 32 bytes\n");
        return;
    }
    // buf is 34 elements, 2 for the address, 32 for the data
    uint8_t buf[34];

    // shifts the upper 8 bits of the 16 bit address into buf 0
    buf[0] = (loc >> 8) & 0xFF;
    // shifts the lower 8 bits to buf 1
    buf[1] = loc & 0xFF;
    // writes the data to the buffer beyond the address from 0 to len
    for (uint8_t i = 0; i < len; i++) {
        buf[i + 2] = data[i];
    }
    // writes the formatted buffer to the eeprom, returns the number of bytes written
    int result = i2c_write_blocking(i2c1, EEPROM_ADDR, buf, len + 2, false);
    // if the result is equal to the length plus the two address bytes, then the eeprom write worked
    if (result == len + 2) {
        printf("EEPROM write ACKed, wrote %d bytes at 0x%04X\n", len, loc);
    // otherwise it failed
    } else {
        printf("EEPROM write failed, result = %d\n", result);
    }
}

void eeprom_read(uint16_t loc, char data[], uint8_t len) {
    // same as write function
    if (len > 32) {
        printf("Error: EEPROM read max is 32 bytes\r\n");
        return;
    }
    // same as write function
    uint8_t addr_buf[2];
    addr_buf[0] = (loc >> 8) & 0xFF;
    addr_buf[1] = loc & 0xFF;


    // Sets the EEPROMS internal read pointer "zero byte write" 
    int write_result = i2c_write_blocking(i2c1, EEPROM_ADDR, addr_buf, 2, true);

    // checks to see if both address bytes were written
    if (write_result != 2) {
        printf("EEPROM read address set failed, result = %d\r\n", write_result);
        return;
    }

    // then reads back the requested bytes
    int read_result = i2c_read_blocking(i2c1, EEPROM_ADDR, (uint8_t *)data, len, false);

    // same as write function
    if (read_result != len) {
        printf("EEPROM read failed, result = %d\r\n", read_result);
        return;
    }

    // Optional: null-terminate if you are treating the result like a string
    if (len < 32) {
        data[len] = '\0';
    } 
}

void save_high_score(int score) {
    //creates a blank page to be used to write data to the eeprom, size 32
    char page[EEPROM_PAGE_SIZE];
    // converts the integer score to text, with the destination being page and format being a decimal
    snprintf(page, sizeof(page), "%d", score);

    int used = strlen(page);
    // following the score, the rest of the space in the page is filled with spaces, so the output is clean and not messy beyond the score
    for (int i = used; i < EEPROM_PAGE_SIZE - 1; i++) {
        page[i] = ' ';
    }
    // null terminator is added to end the string
    page[EEPROM_PAGE_SIZE - 1] = '\0';

    // page is written to the highscore address in the eprom
    eeprom_write(HIGHSCORE_ADDR, page, EEPROM_PAGE_SIZE);
    sleep_ms(10);   // EEPROM internal write cycle
}

int load_high_score(void) {
    // creates a blank page to store the retreived score
    char page[EEPROM_PAGE_SIZE + 1];
    // score is retrieved from address in EEPROM
    eeprom_read(HIGHSCORE_ADDR, page, EEPROM_PAGE_SIZE);
    // null terminator is added to end string
    page[EEPROM_PAGE_SIZE] = '\0';

    // If EEPROM is blank or garbage, atoi() will return 0 for nonnumeric start.
    return atoi(page);
}

void update_high_score(int current_score) {
    // loads the high score value in
    int saved_high = load_high_score();
    // if the current score is higher than the previous, new high score is loaded in 
    if (current_score > saved_high) {
        save_high_score(current_score);
        printf("New high score saved: %d\r\n", current_score);
    }
}

//////////////////////////////////////////////////////////////////////////////

// Provided for you.

void write(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: write <addr> <data>\n");
        printf("Ensure the address is a hexadecimal number.  No need to include 0x.\n");
        return;
    }
    uint32_t addr = strtol(argv[1], NULL, 16); 
    // concatenate all args from argv[2], until empty string is found, to a string
    char data[32] = "";
    int i = 0;
    int j = 2;
    // Concatenate all args from argv[2] onwards into data, separated by spaces
    for (j = 2; j < argc && i < 32; j++) {
        int arg_len = strlen(argv[j]);
        int copy_len = (i + arg_len < 32) ? arg_len : (32 - i);
        strncpy(&data[i], argv[j], copy_len);
        i += copy_len;
        if (j < argc - 1 && i < 32) {
            data[i++] = ' ';
        }
    }
    data[i] = '\0';
    // ensure addr is a multiple of 32
    if ((addr % 32) != 0) {
        printf("Address 0x%lx is not evenly divisible by 32 (0x20).  Your address must be a hexadecimal value.\n", addr);
        return;
    }
    int msglen = strlen(data);
    if (msglen > 32) {
        printf("Data is too long. Max length is 32.\n");
        return;
    }
    else {
        // pad with spaces so it overwrites the entire 32 bytes
        for (int k = msglen; k < 31; k++) {
            data[k] = ' ';
        }
        data[31] = '\0';
    }
    printf("Writing to address 0x%lx: %s\n", addr, data);
    eeprom_write(addr, data, 32);
}

void read(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: read <addr>\n");
        printf("Ensure the address is a hexadecimal number.  No need to include 0x.\n");
        return;
    }
    uint32_t addr = strtol(argv[1], NULL, 16); 
    char data[32];
    // ensure addr is a multiple of 32
    if ((addr % 32) != 0) {
        printf("Address 0x%lx is not evenly divisible by 32 (0x20).  Your address must be a hexadecimal value.\n", addr);
        return;
    }
    eeprom_read(addr, data, 32);
    printf("String at address 0x%lx: %s\r\n", addr, data);
}



//////////////////////////////////////////////////////////////////////////////

// Copy in your UART functions from STEP4/5 of the UART lab into uart.c, 
// NOT main.c (this file)!
// These function signatures must not be replaced unless you want warnings!

void init_uart();
void init_uart_irq();

//////////////////////////////////////////////////////////////////////////////

int main() {
    init_uart();
    init_uart_irq();
    init_i2c();

    setbuf(stdout, NULL);

    printf("I2C Command Shell\r\n");
    printf("Type 'write <addr> <data>' to write data to an address on the EEPROM.\r\n");
    printf("Type 'read <addr>' to read from the specified address on the EEPROM.\r\n");

    int argc;
    char *argv[10];
    char input[100];

    for (;;) {
        printf("\r\n> ");
        fgets(input, sizeof(input), stdin);
        fflush(stdin);

        input[strcspn(input, "\r\n")] = '\0';

        char *tok = strtok(input, " ");
        argc = 0;

        while (tok != NULL && argc < 10) {
            argv[argc] = tok;
            argc++;
            tok = strtok(NULL, " ");
        }

        if (argc == 0) {
            continue;
        }

        if (strcmp(argv[0], "write") == 0) {
            write(argc, argv);
        } else if (strcmp(argv[0], "read") == 0) {
            read(argc, argv);
        } else {
            printf("Unknown command: %s\r\n", argv[0]);
        }
    }
}
