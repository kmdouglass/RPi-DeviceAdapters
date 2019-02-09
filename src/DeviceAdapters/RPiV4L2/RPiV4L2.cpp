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
  buffer_ {0},
  buffers_ (nullptr),
  devices_ {},
  fd_ (-1),
  fmt_ {0},
  fmtdescs_ {},
  initialized_ (0),
  image (nullptr), // TODO Remove me after refactoring
  num_buffers_ (0),
  reqbuf_ {0}
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

  nRet = InitMMAP();
  if ( nRet != DEVICE_OK )
    return nRet;

  initialized_ = true;

  return DEVICE_OK;

}

int RPiV4L2::Shutdown()
{
  if(initialized_)
    // Free the memory-mapped image buffers
    for (unsigned int i = 0; i < reqbuf_.count; i++)
      munmap(buffers_[i].start, buffers_[i].length);

    free(buffers_);
    buffers_ = nullptr;
    close(fd_);

  initialized_ = false;

  return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Property Generators
///////////////////////////////////////////////////////////////////////////////////////////////////

void RPiV4L2::GenerateReadOnlyProperties()
{
  // Pixel format flags (compressed/emulated
  CPropertyAction* pAct = new CPropertyAction(this, &RPiV4L2::OnField);
  CreateIntegerProperty("Field", static_cast< long >( FIELD ), true, pAct);

  // Pixel format flags (compressed/emulated
  pAct = new CPropertyAction(this, &RPiV4L2::OnFormatFlags);
  CreateStringProperty("Format Flags", "", true, pAct);

  // Pixel format description
  pAct = new CPropertyAction(this, &RPiV4L2::OnFormatDescription);
  CreateStringProperty("Format Description", "", true, pAct);

  // Height of the image in pixels
  pAct = new CPropertyAction(this, &RPiV4L2::OnHeight);
  CreateIntegerProperty("Height", static_cast< long >( MAX_HEIGHT ), true, pAct);

  // Width of the image in pixels
  pAct = new CPropertyAction(this, &RPiV4L2::OnWidth);
  CreateIntegerProperty("Width", static_cast< long >( MAX_WIDTH ), true, pAct);
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

int RPiV4L2::OnField(MM::PropertyBase* pProp,MM::ActionType eAct)
{
  if ( eAct == MM::BeforeGet )
  {
    pProp->Set( static_cast < long > ( fmt_.fmt.pix.field ));
  }

  return DEVICE_OK;
}

int RPiV4L2::OnFormatFlags(MM::PropertyBase* pProp,MM::ActionType eAct)
{
  std::string c, e, combined;

  if ( eAct == MM::BeforeGet )
  {
    // TODO Make fmtdescs settable
    struct v4l2_fmtdesc fmtdesc = fmtdescs_.back();
    c = fmtdesc.flags & 1 ? "C" : " ";
    e = fmtdesc.flags & 2 ? "E" : " ";
    combined = c + e;
    pProp->Set( combined.c_str() );
  }

  return DEVICE_OK;
}

int RPiV4L2::OnFormatDescription(MM::PropertyBase* pProp,MM::ActionType eAct)
{
  if ( eAct == MM::BeforeGet )
  {
    // TODO Make fmtdescs settable
    std::string descr = reinterpret_cast< char* >( fmtdescs_.back().description );
    pProp->Set( descr.c_str() );
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
// MM Camera API
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


/**
 * Snaps a single image, leaving all buffers dequeued.
 */
int RPiV4L2::SnapImage()
{
  int nRet = StartCapturing();
  if ( nRet != DEVICE_OK )
    return nRet;

  do {
    // Check whether the device is ready for reading
    nRet = PollDevice();
    if ( nRet != DEVICE_OK )
      return nRet;

    // Dequeue buffer from the device
    nRet = DequeueBuffer();
    if ( nRet == DEVICE_ERR )
      return nRet;
  }
  while ( nRet == MSG_NO_NEW_IMAGE_IN_BUFFER );

  nRet = StopCapturing();
  if ( nRet != DEVICE_OK )
    return nRet;

  return DEVICE_OK;
}


/**
 * Returns a pointer to the most recently dequeued buffer.
 */
const unsigned char* RPiV4L2::GetImageBuffer()
{
  if ( initialized_ ) {
    return static_cast <unsigned char*> ( buffers_[buffer_.index].start );
  }
  else {
    return nullptr;
  }
}


long RPiV4L2::GetImageBufferSize() const
{
  return buffer_.length;
}


unsigned RPiV4L2::GetImageWidth() const 
{
  return fmt_.fmt.pix.width;
}


unsigned RPiV4L2::GetImageHeight() const 
{
  return fmt_.fmt.pix.height;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Dequeues a buffer from the device (if available)
 */
int RPiV4L2::DequeueBuffer() {
  memset(&buffer_, 0, sizeof(buffer_));
  buffer_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer_.memory = V4L2_MEMORY_MMAP;

  // Dequeue a buffer
  if (-1 == xioctl(fd_, VIDIOC_DQBUF, &buffer_)) {
    switch(errno) {
    case EAGAIN:
      // No buffer in the outgoing queue
      return MSG_NO_NEW_IMAGE_IN_BUFFER;
    case EIO:
      // fall through
    default:
      LogMessage("ioctl error: VIDIOC_DQBUF");
      return DEVICE_ERR;
    }
  }

  assert(buffer_.index < num_buffers_);

  // Enqueue the buffer again
  if (-1 == xioctl(fd_, VIDIOC_QBUF, &buffer_)) {
    LogMessage("ioctl error: VIDIOC_QBUF");
    return DEVICE_ERR;
  }

  return MSG_NEW_IMAGE_IN_BUFFER;
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
 * Initialize the memory map to the video buffers.
 */
int RPiV4L2::InitMMAP() {
  reqbuf_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf_.memory = V4L2_MEMORY_MMAP;
  reqbuf_.count = 5; // TODO Make this settable

  if (-1 == xioctl(fd_, VIDIOC_REQBUFS, &reqbuf_)) {
    LogMessage("VIDIOC_REQBUFS failed");
    return DEVICE_ERR; // TODO Custom error message
  }

  if (reqbuf_.count < 2){
    LogMessage("Not enough buffer memory");
    return DEVICE_ERR; // TODO Custom error message
  }

  buffers_ = static_cast< struct buffers* >( calloc(reqbuf_.count, sizeof(*buffers_)) );
  assert(buffers_ != NULL);

  num_buffers_ = reqbuf_.count;

  // Create the buffer memory maps
  for (unsigned int i = 0; i < reqbuf_.count; i++) {
    memset(&buffer_, 0, sizeof(buffer_));
    buffer_.type = reqbuf_.type;
    buffer_.memory = V4L2_MEMORY_MMAP;
    buffer_.index = i;

    if (-1 == xioctl(fd_, VIDIOC_QUERYBUF, &buffer_)) {
      LogMessage("VIDIOC_QUERYBUF failed");
      return DEVICE_ERR; // TODO Custom error message
    }

    buffers_[i].length = buffer_.length;
    buffers_[i].start = mmap(
      NULL,
      buffer_.length,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      fd_,
      buffer_.m.offset
    );

    if (MAP_FAILED == buffers_[i].start) {
      LogMessage("Memory mapping failed");
      return DEVICE_ERR; // TODO Custom error message
    }
  }

  return DEVICE_OK;
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
 * Poll the device file to determine whether it's ready to read from.
 */
int RPiV4L2::PollDevice() {

    fd_set fds;
    struct timeval tv;
    int r;
    for (;;) {
      // Clear the set of file descriptors to monitor, then add the fd for our device
      FD_ZERO(&fds);
      FD_SET(fd_, &fds);

      // Set the timeout
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      /**
       * Arguments are
       * - number of file descriptors
       * - set of read fds
       * - set of write fds
       * - set of except fds
       * - timeval struct
       *
       * According to the man page for select, "nfds should be set to the highest-numbered file
       * descriptor in any of the three sets, plus 1."
       */
      r = select(fd_ + 1, &fds, NULL, NULL, &tv);

      if (-1 == r) {
        if (EINTR == errno)
          continue;

	LogMessage("select: general error in PollDevice");
	return DEVICE_ERR; // TODO Custom error code
      }

      if (0 == r) {
        LogMessage("select: timeout");
        return DEVICE_ERR; // TODO Custom error code
      }

      break;
    }

  return DEVICE_OK; // TODO Possibly make this a custom code
}


/**
 * Set the device's format, including its width, height, and pixel format.
 *
 */
int RPiV4L2::SetVideoDeviceFormat(unsigned int width, unsigned int height) {
  struct v4l2_fmtdesc fmtdesc = fmtdescs_.back(); // TODO Make this settable
  fmt_ = {0};
  fmt_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  fmt_.fmt.pix.width = width;
  fmt_.fmt.pix.height = height;
  fmt_.fmt.pix.pixelformat = fmtdesc.pixelformat;
  fmt_.fmt.pix.field = FIELD;

  if (-1 == xioctl(fd_, VIDIOC_S_FMT, &fmt_)) {
    LogMessage("ioctl error: VIDIOC_S_FMT");
    return DEVICE_ERR;
  }

  return DEVICE_OK;
}


/**
 * Enqueue the image buffers and start capturing.
 */
int RPiV4L2::StartCapturing() {
  enum v4l2_buf_type type;
  struct v4l2_buffer buffer;
  for (unsigned int i = 0; i < num_buffers_; i++) {
    /* Note that we set bytesused = 0, which will set it to the buffer length
     * See
     * - https://www.linuxtv.org/downloads/v4l-dvb-apis-new/uapi/v4l/vidioc-qbuf.html?highlight=vidioc_qbuf#description
     * - https://www.linuxtv.org/downloads/v4l-dvb-apis-new/uapi/v4l/buffer.html#c.v4l2_buffer
     */
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = i;

    // Enqueue the buffer with VIDIOC_QBUF
    if (-1 == xioctl(fd_, VIDIOC_QBUF, &buffer)) {
      LogMessage("ioctl error: VIDIOC_QBUF");
      return DEVICE_ERR; // TODO Custom error code
    }
  }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl(fd_, VIDIOC_STREAMON, &type)) {
    LogMessage("ioctl error: VIDIOC_STREAMON");
    return DEVICE_ERR; // TODO Custom error code
  }

  return DEVICE_OK;
}


int RPiV4L2::StopCapturing() {
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl(fd_, VIDIOC_STREAMOFF, &type)) {
    LogMessage("ioctl error: VIDIOC_STREAMOFF");
    return DEVICE_ERR;
  }

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
