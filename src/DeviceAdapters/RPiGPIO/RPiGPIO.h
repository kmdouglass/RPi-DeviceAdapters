/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * Micro-Manager device adapter for the Raspberry Pi.
 */

#ifndef _RASPBERRYPI_H_
#define _RASPBERRYPI_H_

#include "DeviceBase.h"
extern "C" {
  #include "gpio.h"
}

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

  // Settable properties
  int OnPinState(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
  gpio_registers registers;
  int pinState_;

  void GenerateControlledProperties();
  void SetPinState(int pinState);

  // MM API
  bool initialized_;
};

#endif //_RASPBERRYPI_H_
