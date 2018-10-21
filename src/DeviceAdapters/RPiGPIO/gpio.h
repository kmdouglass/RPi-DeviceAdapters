/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * GPIO register driver for the Raspberry Pi.
 *
 * Based on implementations by Gert van Loo & Dom and the Muddy
 * Engineer.
 *  - https://elinux.org/RPi_GPIO_Code_Samples#Direct_register_access -
 *  - https://www.muddyengineer.com/2016/11/c-programming-raspberry-pi-2-gpio-driver/
 */

#ifndef GPIO_H
#define GPIO_H

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// Physical addresses
#define BCM2837_PERIPHERAL_BASE   0x3F000000
#define GPIO_BASE                 (BCM2837_PERIPHERAL_BASE + 0x200000)
#define GPIOMEM_BASE              0
#define SYSTIME_BASE              (BCM2837_PERIPHERAL_BASE + 0x003000)
#define BLOCK_SIZE                (4 * 1024)

// Functions for the different pins
#define INPUT  0
#define OUTPUT 1
#define ALT0   4
#define ALT1   5
#define ALT2   6
#define ALT3   7
#define ALT4   3
#define ALT5   2

// Offsets from the GPIO base for the different registers
#define GPFSEL 0
#define GPSET  7
#define GPCLR  10
#define GPLEV  13

/**
 Provides access to the Raspberry Pi's GPIO registers. All accesses
 are 32-bit.
 */
typedef struct gpio_registers {
  volatile unsigned int *_memory_map;
  volatile unsigned int *fsel;
  volatile unsigned int *set;
  volatile unsigned int *clr;
  volatile unsigned int *lev;
} gpio_registers;

/**
 Initializes the memory map to the GPIO peripherals.
 */
void pioInit(gpio_registers* registers);

/**
 Sets the function of a GPIO pin. function should take a value between
 0 and 7:
   0 - input
   1 - output
   2-7 - alternate function
 */
void pinMode(gpio_registers* registers, const int pin, int function);

/**
 Writes val to the GPIO pin, where val is either 0 or 1.
 */
void digitalWrite(gpio_registers* registers, const int pin, int val);

/**
 Reads the value on the GPIO pin.
 */
int digitalRead(gpio_registers* registers, const int pin);

#endif // GPIO_H
