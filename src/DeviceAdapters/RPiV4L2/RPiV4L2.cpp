/**
 * Kyle M. Douglass, 2019
 * kyle.m.douglass@gmail.com
 *
 * Micro-Manager device adapter for a Video 4 Linux version 2 device.
 *
 * This device adapter is based on the original V4L2 device adapter for Micro-Manager by Martin
 * Kielhorn.
 */

// LICENSE:       This file is distributed under the "LGPL" license.
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.

#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <exception>
#include <iostream>
#include <string>
#include <math.h>
#include "MMDevice.h"
#include "ModuleInterface.h"
#include <sstream>
#include <map>
#include <vector>

#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <pthread.h>

#include "RPiV4L2.h"
#include "refactor.h" // TODO remove after refactoring

using namespace std;

const char
  *g_DeviceName="RPiV4L2",
  *g_Description="Rasperry Pi Video4Linux2 camera device adapter";

///////////////////////////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * List all supported hardware devices here
 */
MODULE_API void InitializeModuleData()
{
  RegisterDevice(g_DeviceName, MM::CameraDevice, g_Description);
}

MODULE_API MM::Device* CreateDevice(const char*deviceName)
{
  if (deviceName == 0)
    return 0;
  
  if (strcmp(deviceName, g_DeviceName) == 0)
    return new RPiV4L2();
  return 0;
}

MODULE_API void DeleteDevice(MM::Device*pDevice)
{
  delete pDevice;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RPiV4L2 implementation
// ~~~~~~~~~~~~~~~~~~~~~~

/**
 * Constructor
 *
 */
RPiV4L2::RPiV4L2() :
  devices_ {},
  fd_ (-1),
  fmt_ {0},
  fmtdescs_ {},
  initialized_ (0),
  image (nullptr)
  {
  SetErrorText(ERR_NO_VIDEO_DEVICE_FILES, "No video device files present on the system.");
  SetErrorText(ERR_DEVICE_CHANGE_FORBIDDEN, "Cannot change video device after initialization.");
  InitializeDefaultErrorMessages();

  // Video device files; do not continue if none can be found.
  FindVideoDeviceFiles(devices_);

  if ( !devices_.empty() ) {
    CPropertyAction* pAct = new CPropertyAction(this, &RPiV4L2::OnDevice);
    CreateStringProperty("Video Device", devices_[0].c_str(), false, pAct, true); // PreInit prop
    for (const auto &device: devices_) {
      AddAllowedValue("Video Device", device.c_str());
    }
  }
  else {
    LogMessage("No video device files found.");
  }

  // Refactor
  state->W = gWidth;
  state->H = gHeight;
}

RPiV4L2::~RPiV4L2() {
  Shutdown();
  if (image) {
    free(image);
    image=nullptr;
  }
}

void RPiV4L2::GetName(char*name) const
{
  CDeviceUtils::CopyLimitedString(name,g_DeviceName);
}

int RPiV4L2::Initialize()
{
  if ( initialized_ )
    return DEVICE_OK;

  if ( devices_.empty() )
    return ERR_NO_VIDEO_DEVICE_FILES;

  // Property generators
  CreateProperty(MM::g_Keyword_Name,g_DeviceName, MM::String, true);
  CreateProperty(MM::g_Keyword_Description, g_Description, MM::String, true);

  GenerateReadOnlyProperties();

  // Binning
  CPropertyAction* pAct = new CPropertyAction(this, &RPiV4L2::OnBinning);
  int nRet = CreateProperty(MM::g_Keyword_Binning, "1", MM::Integer, false, pAct);
  if (nRet != DEVICE_OK)
    return nRet;
  
  // Pixel type
  pAct = new CPropertyAction (this, &RPiV4L2::OnPixelType);
  nRet = CreateProperty(MM::g_Keyword_PixelType, "8bit", MM::String,true, pAct);
  if (nRet != DEVICE_OK)
    return nRet;
  
  // Gain
  pAct = new CPropertyAction (this, &RPiV4L2::OnGain);
  nRet = CreateProperty(MM::g_Keyword_Gain, "0", MM::Integer, false, pAct);
  if (nRet != DEVICE_OK)
    return nRet;
  
  // Exposure
  pAct = new CPropertyAction (this, &RPiV4L2::OnExposure);
  nRet = CreateProperty(MM::g_Keyword_Exposure, "0.0", MM::Float, false, pAct);
  if (nRet != DEVICE_OK)
    return nRet;

  // Intialize video devices
  nRet = OpenVideoDevice();
  if (nRet != DEVICE_OK)
    return nRet;

  GetVideoDeviceFormatDescription();

  nRet = SetVideoDeviceFormat(MAX_WIDTH, MAX_HEIGHT);
  if ( nRet != DEVICE_OK )
    return nRet;

  initialized_ = true;

  return DEVICE_OK;

}

int RPiV4L2::Shutdown()
{
  if(initialized_)
    close(fd_);

  initialized_ = false;

  return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Property Generators
///////////////////////////////////////////////////////////////////////////////////////////////////

void RPiV4L2::GenerateReadOnlyProperties()
{
  // Height of the image in pixels
  CPropertyAction* pAct = new CPropertyAction(this, &RPiV4L2::OnHeight);
  CreateIntegerProperty("Height", static_cast< long >(MAX_HEIGHT), true, pAct);

  // Width of the image in pixels
  pAct = new CPropertyAction(this, &RPiV4L2::OnWidth);
  CreateIntegerProperty("Width", static_cast< long >(MAX_WIDTH), true, pAct);
}
  
///////////////////////////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////////////////////////

int RPiV4L2::OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  if (eAct == MM::BeforeGet)
  {
    //on_binning(); // FIXME
  }
  else if (eAct == MM::AfterSet)
  {
  }
  return DEVICE_OK;
}

int RPiV4L2::OnDevice(MM::PropertyBase* pProp, MM::ActionType eAct)
{

  if (eAct == MM::BeforeGet)
  {
    pProp->Set(current_device_.c_str());
  }
  else if (eAct == MM::AfterSet)
  {
    if ( initialized_ ) {
      // Revert the device
      pProp->Set(current_device_.c_str());
      LogMessage("Cannot change video device after device has been initialiezd.");
      return ERR_DEVICE_CHANGE_FORBIDDEN;
    }
    pProp->Get(current_device_);

  }
  return DEVICE_OK;
}

int RPiV4L2::OnExposure(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  if ( eAct == MM::BeforeGet )
  {
    //pProp->Set(get_exposure()); // FIXME
    //on_exposure();
  }
  else if ( eAct == MM::AfterSet )
  {
    double exp;
    pProp->Get(exp);
    //set_exposure(exp); // FIXME
  }
    return DEVICE_OK;
}

int RPiV4L2::OnGain(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  if ( eAct == MM::BeforeGet )
  {
    //on_gain(); // FIXME
  }
  else if( eAct == MM::AfterSet )
  {
  }
  return DEVICE_OK;
}

/**
 * Image height in pixels
 */
int RPiV4L2::OnHeight(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  if ( eAct == MM::BeforeGet )
  {
    pProp->Set( static_cast< long >(fmt_.fmt.pix.height) );
  }

  return DEVICE_OK;
}

int RPiV4L2::OnPixelType(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  return DEVICE_OK;
}

/**
 * Image width in pixels
 */
int RPiV4L2::OnWidth(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  if ( eAct == MM::BeforeGet )
  {
    pProp->Set( static_cast< long >(fmt_.fmt.pix.width) );
  }

  return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * TODO: implement
 */
bool RPiV4L2::Busy() {
  return false;
}

/**
 * TODO: implement if possible
 */
int RPiV4L2::IsExposureSequenceable(bool& isSequenceable) const 
{
   isSequenceable = false; 
   return DEVICE_OK;
}

// blocks until exposure is finished
int RPiV4L2::SnapImage()
{
  unsigned char*data=VideoTakeBuffer(state);
  int i,j;
  for(j=0;j<state->H;j++){
    int wj=state->W*j;
    for(i=0;i<state->W;i++){
      image[i+wj]=data[2*i+2*wj];
    }
  }
  VideoReturnBuffer(state);
  return DEVICE_OK;
}

// waits for camera readout
const unsigned char* RPiV4L2::GetImageBuffer()
{
  return image;
}

// changes only if binning, pixel type, ... properties are set
long RPiV4L2::GetImageBufferSize() const
{
  return state->W*state->H;
}

unsigned RPiV4L2::GetImageWidth() const 
{
  return state->W;
}

unsigned RPiV4L2::GetImageHeight() const 
{
  return state->H;
}

unsigned RPiV4L2::GetImageBytesPerPixel() const
{
  // FIXME
  return 1; //get_image_bytes_per_pixel();
}

unsigned RPiV4L2::GetBitDepth() const 
{
  // FIXME
  return 8;// get_bit_depth();
}

int RPiV4L2::SetROI(unsigned x,unsigned y,unsigned xSize,unsigned ySize)
{
  (void) x; // get rid of warning
  (void) y;
  (void) xSize;
  (void) ySize;
  // FIXME
  // set_roi(x,y,xSize,ySize);
  return DEVICE_OK;
}

int RPiV4L2::GetBinning() const 
{
  // FIXME
  return 1;// get_binning();
}
  
int RPiV4L2::SetBinning(int binSize)
{
  // FIXME  set_binning(binSize);
  SetProperty(MM::g_Keyword_Binning, CDeviceUtils::ConvertToString(binSize));
  return DEVICE_OK;
}

double RPiV4L2::GetExposure() const 
{
  // FIXME
  return 1.0; //get_exposure();
}

void RPiV4L2::SetExposure(double exp)
{
  // FIXME
  // set_exposure(exp);
  SetProperty(MM::g_Keyword_Exposure, CDeviceUtils::ConvertToString(exp));
}

int RPiV4L2::GetROI(unsigned &x, unsigned &y, unsigned&xSize, unsigned &ySize)
{
  // FIXME
  // get_roi(&x,&y,&xSize,&ySize);
  x=0; y=0; xSize=state->W; ySize=state->H;
  return DEVICE_OK;
}

int RPiV4L2::ClearROI()
{
  // V4L2 should automatically handle adjust heights/widths that are too large.
  return SetVideoDeviceFormat(MAX_WIDTH, MAX_HEIGHT); // TODO Change this so that only the size is changed
}

/**
 * Find all the video device files present on the system.
 *
 */
void RPiV4L2::FindVideoDeviceFiles(std::vector<std::string> &devices) {
  const boost::filesystem::path DEVICE_FOLDER("/dev");
  const boost::regex VIDEO_DEV_FILTER("^/dev/video\\d+$");

  boost::system::error_code ec;
  if ( boost::filesystem::exists(DEVICE_FOLDER, ec) ) {

    if ( ec != 0 ) return;

    // Loop over all files in the directory
    std::string current_file;
    boost::filesystem::directory_iterator it{DEVICE_FOLDER};
    for (const boost::filesystem::directory_entry &entry : it) {

      // Skip if it doesn't match the video device filter regex
      if ( !boost::regex_match( entry.path().string(), VIDEO_DEV_FILTER ) ) continue;

      // Found a match; add it to the vector
      current_file = boost::filesystem::canonical(entry.path(), ec).string();

      if ( ec != 0) continue;
      devices.push_back(current_file);
    }
  }
}

/**
 * Opens the video device file for ioctl.
 */
int RPiV4L2::OpenVideoDevice() {
  fd_ = open(current_device_.c_str(), O_RDWR | O_NONBLOCK);

  if ( fd_ < 0 ) {
    LogMessage("Failed to obtain video device file descriptor.");
    return DEVICE_ERR;
  }

  return DEVICE_OK;
}

/**
 * Query the device for its possible format descriptions.
 */
void RPiV4L2::GetVideoDeviceFormatDescription() {
  struct v4l2_fmtdesc fmtdesc = {0};
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  // Get the format with the largest index and use it
  while(0 == xioctl(fd_, VIDIOC_ENUM_FMT, &fmtdesc)) {
    fmtdescs_.push_back(fmtdesc);
    fmtdesc.index++;
  }
}

/**
 * Set the device's format, including its width, height, and pixel format.
 *
 * TODO FINISH ME
 */
int RPiV4L2::SetVideoDeviceFormat(unsigned int width, unsigned int height) {
  struct v4l2_fmtdesc fmtdesc = fmtdescs_.back(); // TODO Make this settable
  fmt_ = {0};
  fmt_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  fmt_.fmt.pix.width = width;
  fmt_.fmt.pix.height = height;
  fmt_.fmt.pix.pixelformat = fmtdesc.pixelformat;
  fmt_.fmt.pix.field = V4L2_FIELD_NONE;

  if (-1 == xioctl(fd_, VIDIOC_S_FMT, &fmt_)) {
    LogMessage("ioctl error: VIDIOC_S_FMT");
    return DEVICE_ERR;
  }

  printf("\nUsing format: %s\n", fmtdesc.description);
  char format_code[5];
  strncpy(format_code, (char*)&fmt_.fmt.pix.pixelformat, 5);
  printf(
    "Set format:\n"
    " Width: %d\n"
    " Height: %d\n"
    " Pixel format: %s\n"
    " Field: %d\n\n",
    fmt_.fmt.pix.width,
    fmt_.fmt.pix.height,
    format_code,
    fmt_.fmt.pix.field
  );

  // TODO Setup the mmap here or in a new method

  return DEVICE_OK;
}

/**
 * Wrapper around ioctl system call for error handling.
 */
int RPiV4L2::xioctl(int fd, int request, void *arg) {
  int r;

  do {
    r = ioctl(fd, request, arg);
  } while (-1 == r && EINTR == errno);

  return r;
}
