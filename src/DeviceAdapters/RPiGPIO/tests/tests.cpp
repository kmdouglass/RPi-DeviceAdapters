/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * Unit tests for the Raspberry Pi GPIO driver.
 */

#include <stdint.h>
#include <gtest/gtest.h>
#include "../gpio.h"

class GPIOTests : public ::testing::Test {
public:

  gpio_registers mock_registers;

  void SetUp() {
    // Create an array to serve as the mock memory map
    // Divide block_size by 4 becauses accesses are 32-bit
    uint32_t mock_memory[BLOCK_SIZE / 4] = { 0 };

    this->mock_registers = {
      ._memory_map = (volatile unsigned *)mock_memory,
      .fsel = (volatile unsigned int *)(mock_memory + GPFSEL),
      .set  = (volatile unsigned int *)(mock_memory + GPSET),
      .clr  = (volatile unsigned int *)(mock_memory + GPCLR),
      .lev  = (volatile unsigned int *)(mock_memory + GPLEV),
    };
  }
};

TEST_F(GPIOTests, SET_PIN_MODE) {
  int pin = 0;
  int function = OUTPUT;
  pinMode(&mock_registers, pin, function);

  ASSERT_EQ(1, mock_registers.fsel[0]);
}

TEST_F(GPIOTests, SET_PIN_MODE_ALT) {
  int pin = 1;
  int function = ALT0;
  pinMode(&mock_registers, pin, function);

  ASSERT_EQ(32, mock_registers.fsel[0]);
}

TEST_F(GPIOTests, DIGITAL_WRITE_SET) {
  int pin = 0;
  int val = 1;
  digitalWrite(&mock_registers, pin, val);

  ASSERT_EQ(1, mock_registers.set[0]);
}

TEST_F(GPIOTests, DIGITAL_WRITE_CLEAR) {
  int pin = 0;
  int val = 0;
  digitalWrite(&mock_registers, pin, val);

  ASSERT_EQ(1, mock_registers.clr[0]);
}

TEST_F(GPIOTests, DIGITAL_READ_FIRST_ADDRESS) {
  int pin = 0;
  int val = digitalRead(&mock_registers, pin);

  ASSERT_EQ(0, val);
}

TEST_F(GPIOTests, DIGITAL_READ_SECOND_ADDRESS) {
  int pin = 32;
  int val = digitalRead(&mock_registers, pin);

  ASSERT_EQ(0, val);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
