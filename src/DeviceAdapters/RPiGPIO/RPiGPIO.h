/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * Micro-Manager device adapter for the Raspberry Pi.
 */

#ifndef _RASPBERRYPI_H_
#define _RASPBERRYPI_H_

#include <array>

#include "DeviceBase.h"
extern "C" {
  #include "gpio.h"
}

# define NUM_PINS 28

class RPiGPIO : public CGenericBase<RPiGPIO>
{
public:
  RPiGPIO();
  ~RPiGPIO();

  // MMDevice API
  int Initialize();
  int Shutdown();
  void GetName(char* name) const;
  bool Busy() {return false;};

  // Action handlers
  int OnPinNumber(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnPinState(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
  gpio_registers registers;
  static const std::array<long, NUM_PINS> pins;
  long pinNumber_;
  long pinState_;

  void GenerateControlledProperties();
  void SetPinState(long pinState);

  // MM API
  bool initialized_;

  // Error codes
  const int ERR_PIN_CHANGE_FORBIDDEN = 101;
};

#endif //_RASPBERRYPI_H_
