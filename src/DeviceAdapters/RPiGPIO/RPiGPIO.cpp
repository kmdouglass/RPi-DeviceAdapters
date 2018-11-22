/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * Micro-Manager device adapter for a Raspberry Pi GPIO pin.
 */

#include <string.h>
#include <vector>

#include "RPiGPIO.h"
#include "ModuleInterface.h"

using namespace std;

const char* g_DeviceName = "RPiGPIO";

///////////////////////////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * List all supported hardware devices here
 */
MODULE_API void InitializeModuleData()
{
  RegisterDevice(
    g_DeviceName,
    MM::GenericDevice,
    "Control of a single Raspberry Pi GPIO pin."
  );
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
  if (deviceName == 0)
     return 0;

  // Decide which device class to create based on the deviceName parameter
  if (strcmp(deviceName, g_DeviceName) == 0)
  {
     // Create the Raspberry Pi device...
     return new RPiGPIO();
  }
  // ...supplied name not recognized
  return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
  delete pDevice;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RPiGPIO implementation
// ~~~~~~~~~~~~~~~~~~~~~~

/**
 * GPIO pin ids on the Raspberry Pi.
 *
 * Pin numbers follow the Broadcom numbering scheme.
 */
const std::array<long, NUM_PINS> RPiGPIO::pins = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27
};

/**
 * RPiGPIO constructor.
 *
 */
RPiGPIO::RPiGPIO() :
  pinNumber_ (0),
  pinState_ (0),
  initialized_ (false)
{
  SetErrorText(ERR_PIN_CHANGE_FORBIDDEN, "Cannot change pin number after initialization.");
  InitializeDefaultErrorMessages();

  // Setup the GPIO pin number property.
  CPropertyAction* pAct = new CPropertyAction(this, &RPiGPIO::OnPinNumber);
  CreateIntegerProperty("Pin", pinNumber_, false, pAct, true);
  for (const auto &pin : pins) {
    std::string pin_str = std::to_string(pin);
    AddAllowedValue("Pin", pin_str.c_str());
  }
}

/**
 * RPiGPIO destructor.
 *
 * If this device used as intended within the Micro-Manager system, Shutdown() will be always
 * called before the destructor. But in any case we need to make sure that all resources are
 * properly released even if Shutdown() was not called.
 */
RPiGPIO::~RPiGPIO()
{
  if (initialized_)
     Shutdown();
}

/**
 * Obtains device name. Required by the MM::Device API.
 */
void RPiGPIO::GetName(char* name) const
{
  // We just return the name we use for referring to this device adapter.
  CDeviceUtils::CopyLimitedString(name, g_DeviceName);
}

/**
 * Intializes the hardware.
 *
 * Typically we access and initialize hardware at this point.  Device properties are typically
 * created here as well. Required by the MM::Device API.
 */
int RPiGPIO::Initialize()
{
  if (initialized_)
    return DEVICE_OK;

  // Initialize the GPIO registers.
  pioInit(&registers);
  pinMode(&registers, pinNumber_, OUTPUT);

  // Set read-only properties
  // ------------------------
  // Name
  int ret = CreateStringProperty(MM::g_Keyword_Name, g_DeviceName, true);
  if (DEVICE_OK != ret)
    return ret;

  // Description
  ret = CreateStringProperty(MM::g_Keyword_Description, "Controls a Raspberry Pi GPIO pin", true);
  if (DEVICE_OK != ret)
    return ret;

  // Set property list
  // -----------------
  GenerateControlledProperties();

  // Synchronize all properties
  // --------------------------
  ret = UpdateStatus();
  if (ret != DEVICE_OK)
    return ret;

  initialized_ = true;
  return DEVICE_OK;
}

/**
 * Shuts down (unloads) the device.
 * 
 * Ideally this method will completely unload the device and release all resources.  Shutdown() may
 * be called multiple times in a row.  Required by the MM::Device API.
 */
int RPiGPIO::Shutdown()
{
  initialized_ = false;
  return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Property Generators
///////////////////////////////////////////////////////////////////////////////////////////////////

void RPiGPIO::GenerateControlledProperties()
{
  // Turn on/off the GPIO pin
  CPropertyAction* pAct = new CPropertyAction(this, &RPiGPIO::OnPinState);
  CreateIntegerProperty("State", 0, false, pAct);
  std::vector<std::string> states;
  states.push_back("0");
  states.push_back("1");
  SetAllowedValues("State", states);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////////////////////////
int RPiGPIO::OnPinNumber(MM::PropertyBase* pProp,MM::ActionType eAct)
{
  if (eAct == MM::BeforeGet)
  {
    pProp->Set(pinNumber_);
  }
  else if (eAct == MM::AfterSet) {

    if (initialized_)
    {
      // Revert the pin number
      pProp->Set(pinNumber_);
      return ERR_PIN_CHANGE_FORBIDDEN;
    }
    pProp->Get(pinNumber_);
    
  }

  return DEVICE_OK;
}

int RPiGPIO::OnPinState(MM::PropertyBase* pProp, MM::ActionType eAct)
{

  if (eAct == MM::BeforeGet)
  {
    pProp->Set(pinState_);
  }
  else if (eAct == MM::AfterSet)
  {
    pProp->Get(pinState_);
    this->SetPinState(pinState_);
  }

  return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Sets the state of the pin (on/off).
 */
void RPiGPIO::SetPinState(long pinState)
{
  int state = static_cast<int>(pinState);
  digitalWrite(&registers, pinNumber_, state);
}
