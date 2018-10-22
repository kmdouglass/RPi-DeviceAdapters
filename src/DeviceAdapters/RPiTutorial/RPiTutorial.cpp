/**
 * Kyle M. Douglass, 2018
 * kyle.m.douglass@gmail.com
 *
 * Tutorial Micro-Manager device adapter for the Raspberry Pi.
 */

#include "RPiTutorial.h"
#include "ModuleInterface.h"

using namespace std;

const char* g_DeviceName = "RPiTutorial";

//////////////////////////////////////////////////////////////////////
// Exported MMDevice API
//////////////////////////////////////////////////////////////////////

/**
 * List all supported hardware devices here
 */
MODULE_API void InitializeModuleData()
{
  RegisterDevice(
    g_DeviceName,
    MM::GenericDevice,
    "Control of the Raspberry Pi GPIO pins."
  );
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
  if (deviceName == 0)
     return 0;

  // decide which device class to create based on the deviceName parameter
  if (strcmp(deviceName, g_DeviceName) == 0)
  {
     // create the test device
     return new RPiTutorial();
  }
  // ...supplied name not recognized
  return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
  delete pDevice;
}

//////////////////////////////////////////////////////////////////////
// RPiTutorial implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * RPiTutorial constructor.
 *
 * Setup default all variables and create device properties required to exist before
 * intialization. In this case, no such properties were required. All properties will be created in
 * the Initialize() method.
 *
 * As a general guideline Micro-Manager devices do not access hardware in the the constructor. We
 * should do as little as possible in the constructor and perform most of the initialization in the
 * Initialize() method.
 */
RPiTutorial::RPiTutorial() :
  initialized_ (false),
  switch_ (true)
{
  // call the base class method to set-up default error codes/messages
  InitializeDefaultErrorMessages();
}

/**
 * RPiTutorial destructor.
 *
 * If this device used as intended within the Micro-Manager system, Shutdown() will be always
 * called before the destructor. But in any case we need to make sure that all resources are
 * properly released even if Shutdown() was not called.
 */
RPiTutorial::~RPiTutorial()
{
  if (initialized_)
     Shutdown();
}

/**
 * Obtains device name.  Required by the MM::Device API.
 */
void RPiTutorial::GetName(char* name) const
{
  // We just return the name we use for referring to this device adapter.
  CDeviceUtils::CopyLimitedString(name, g_DeviceName);
}

/**
 * Intializes the hardware.
 * 
 * Typically we access and initialize hardware at this point. Device properties are typically
 * created here as well. Required by the MM::Device API.
 */
int RPiTutorial::Initialize()
{
  if (initialized_)
    return DEVICE_OK;

  // set property list
  // -----------------
  // Name
  int ret = CreateStringProperty(MM::g_Keyword_Name, "RPiTutorial device adapter", true);
  assert(ret == DEVICE_OK);
  
  // Description property
  ret = CreateStringProperty(MM::g_Keyword_Description, "A test device adapter", true);
  assert(ret == DEVICE_OK);

  // On/Off switch
  CPropertyAction* pAct = new CPropertyAction (this, &RPiTutorial::OnSwitchOnOff);
  CreateProperty("Switch On/Off", "Off", MM::String, false, pAct);    
  std::vector<std::string> commands;
  commands.push_back("Off");
  commands.push_back("On");
  SetAllowedValues("Switch On/Off", commands);

  // synchronize all properties
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
 * Ideally this method will completely unload the device and release
 * all resources. Shutdown() may be called multiple times in a row.
 * Required by the MM::Device API.
 */
int RPiTutorial::Shutdown()
{
  initialized_ = false;
  return DEVICE_OK;
}

/**
 * Callback function for on/off switch.
 */
int RPiTutorial::OnSwitchOnOff(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  std::string state;
  if (eAct == MM::BeforeGet) {
    if (switch_) { pProp->Set("On"); }
    else { pProp->Set("Off"); }
  } else if (eAct == MM::AfterSet) {
      pProp->Get(state);
      if (state == "Off") { switch_ = false; }
      else if (state == "On") { switch_ = true; }
      else { return DEVICE_ERR; }
  }

  return DEVICE_OK;
}
