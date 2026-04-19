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

//////////////////////////////////////////////////////////////////////////////

void init_i2c() {
    i2c_init(i2c1, 400000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
}


void eeprom_write(uint16_t loc, const char* data, uint8_t len) {
    if (len > 32) {
        printf("Error: EEPROM page write max is 32 bytes\n");
        return;
    }

    uint8_t buf[34];

    buf[0] = (loc >> 8) & 0xFF;
    buf[1] = loc & 0xFF;

    for (uint8_t i = 0; i < len; i++) {
        buf[i + 2] = data[i];
    }

    int result = i2c_write_blocking(i2c1, EEPROM_ADDR, buf, len + 2, false);

    if (result == len + 2) {
        printf("EEPROM write ACKed, wrote %d bytes at 0x%04X\n", len, loc);
    } else {
        printf("EEPROM write failed, result = %d\n", result);
    }
}

void eeprom_read(uint16_t loc, char data[], uint8_t len) {
    if (len > 32) {
        printf("Error: EEPROM read max is 32 bytes\r\n");
        return;
    }

    uint8_t addr_buf[2];
    addr_buf[0] = (loc >> 8) & 0xFF;
    addr_buf[1] = loc & 0xFF;

    // First: send the EEPROM memory address as a write, but do not release the bus
    int write_result = i2c_write_blocking(i2c1, EEPROM_ADDR, addr_buf, 2, true);

    if (write_result != 2) {
        printf("EEPROM read address set failed, result = %d\r\n", write_result);
        return;
    }

    // Then: read back the requested bytes
    int read_result = i2c_read_blocking(i2c1, EEPROM_ADDR, (uint8_t *)data, len, false);

    if (read_result != len) {
        printf("EEPROM read failed, result = %d\r\n", read_result);
        return;
    }

    // Optional: null-terminate if you are treating the result like a string
    if (len < 32) {
        data[len] = '\0';
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
