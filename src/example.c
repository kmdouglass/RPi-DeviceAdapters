/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * Example program for the register driver for the Raspberry Pi.
 *
 */

#include <time.h>
#include "gpio.h"

#define LOW      0
#define HIGH     1
#define TEST_PIN 4

int main (int argc, char **argv) {

  // Initialize the memory map for manipulating the registers
  gpio_registers registers;
  pioInit(&registers);

  // Set the mode of the pin, i.e. input, output, or an alt mode
  pinMode(&registers, TEST_PIN, OUTPUT);

  // Set pin output to high and wait two seconds
  digitalWrite(&registers, TEST_PIN, HIGH);
  unsigned int retTime = time(0) + 2;
  while (time(0) < retTime);

  // Set pin output to low
  digitalWrite(&registers, TEST_PIN, LOW);

  return 0;
}
