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

#include "gpio.h"

/**
 Initializes the memory map to the GPIO registers.
 */
void pioInit(gpio_registers* registers) {
  int mem_fd;
  void *reg_map;
  volatile unsigned int *gpio;

  if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
    printf("Error: cannot open /dev/gpiomem \n");
    exit(-1);
  }

  reg_map = mmap(
    NULL,
    BLOCK_SIZE,
    PROT_READ|PROT_WRITE,
    MAP_SHARED,
    mem_fd,
    GPIOMEM_BASE);

  if (reg_map == MAP_FAILED) {
    printf("GPIO memory map failed: error %ld\n", (long) reg_map);
    close(mem_fd);
    exit(-1);
  }

  gpio = (volatile unsigned *)reg_map;
  
  (*registers)._memory_map = (volatile unsigned *)gpio;
  (*registers).fsel = (volatile unsigned int *)(gpio + GPFSEL);
  (*registers).set  = (volatile unsigned int *)(gpio + GPSET);
  (*registers).clr  = (volatile unsigned int *)(gpio + GPCLR);
  (*registers).lev  = (volatile unsigned int *)(gpio + GPLEV);
}

/**
 Sets the function of a GPIO pin.
 */
void pinMode(gpio_registers *registers, const int pin, int function) {
  unsigned offset, shift;
  offset = pin / 10;
  shift = (pin % 10) * 3;

  // Clears the bits for the particular pin
  (*registers).fsel[offset] &= ~(0b111 << shift);

  // Sets the bits
  (*registers).fsel[offset] |= (function << shift);
}

/**
 Writes a value to a GPIO pin.
 */
void digitalWrite(gpio_registers *registers, const int pin, int val) {
  // Determines which register to use
  int offset = (pin < 32) ? 0 : 1;

  // Set the value on the corresponding bit
  if (val) {
    (*registers).set[offset] = 0x1 << (pin % 32);
  } else {
    (*registers).clr[offset] = 0x1 << (pin % 32);
  }
}

/**
 Reads the value from a GPIO pin.
 */
int digitalRead(gpio_registers *registers, const int pin) {
  // Determine which of two registers to use, read the value on the
  // corresponding bit
  if (pin < 32) {
    return (*registers).lev[0] &= (0x1 << (pin % 32));
  } else {
    return (*registers).lev[1] &= (0x1 << (pin % 32));
  }
}


