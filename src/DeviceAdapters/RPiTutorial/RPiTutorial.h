/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * Tutorial Micro-Manager device adapter for the Raspberry Pi.
 */

#ifndef _RASPBERRYPI_H_
#define _RASPBERRYPI_H_

#include "DeviceBase.h"

class RPiTutorial : public CGenericBase<RPiTutorial>
{
public:
  RPiTutorial();
  ~RPiTutorial();

  // MMDevice API
  int Initialize();
  int Shutdown();

  void GetName(char* name) const;
  bool Busy() {return false;};

  // Settable Properties
  // -------------------
  int OnSwitchOnOff(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
  bool initialized_;
  bool switch_;
};

#endif //_RASPBERRYPI_H_
