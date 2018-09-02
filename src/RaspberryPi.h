/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * Micro-Manager device adapter for the Raspberry Pi.
 */

#ifndef _RASPBERRYPI_H_
#define _RASPBERRYPI_H_

#include "DeviceBase.h"

class RaspberryPi : public CGenericBase<RaspberryPi>
{
public:
  RaspberryPi();
  ~RaspberryPi();

  // MMDevice API
  int Initialize();
  int Shutdown();

  void GetName(char* name) const;
  bool Busy() {return false;};

private:
  bool initialized_;
};

#endif //_RASPBERRYPI_H_
