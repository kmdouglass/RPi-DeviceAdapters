/**
 * Kyle M. Douglass, 2018
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
  *g_DeviceName="Video4Linux2",
  *g_Description="video4linux2 camera device adapter";

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

RPiV4L2::RPiV4L2() :
  initialized_ (0),
  image (nullptr)
{
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
  if (initialized_)
    return DEVICE_OK;

  CreateProperty(MM::g_Keyword_Name,g_DeviceName, MM::String, true);
  CreateProperty(MM::g_Keyword_Description, g_Description, MM::String, true);

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
  assert(nRet == DEVICE_OK);
  
  // Exposure
  pAct = new CPropertyAction (this, &RPiV4L2::OnExposure);
  nRet = CreateProperty(MM::g_Keyword_Exposure, "0.0", MM::Float, false, pAct);
  assert(nRet == DEVICE_OK);
    
  image = (unsigned char*) malloc(state->W * state->H);
  if (VideoInit(state))
  {
    initialized_=true;
    return DEVICE_OK;
  }
  else
  {
    initialized_=false;
    return DEVICE_ERR;
  }
  //VideoRunThread(state,image);
}

int RPiV4L2::Shutdown()
{
  if(initialized_)
    VideoClose(state);

  initialized_ = false;

  return DEVICE_OK;
}
  
///////////////////////////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////////////////////////
int RPiV4L2::OnExposure(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  if (eAct == MM::BeforeGet)
  {
    //pProp->Set(get_exposure()); // FIXME
    //on_exposure();
  }
  else if (eAct == MM::AfterSet)
  {
    double exp;
    pProp->Get(exp);
    //set_exposure(exp); // FIXME
  }
    return DEVICE_OK;
  }

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
  
int RPiV4L2::OnPixelType(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  return DEVICE_OK;
}

int RPiV4L2::OnGain(MM::PropertyBase* pProp, MM::ActionType eAct)
{
  if (eAct == MM::BeforeGet){
    //on_gain(); // FIXME
  }
  else if(eAct == MM::AfterSet)
  {
  }
  return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * TODO: implement if possible
 */
int RPiV4L2::IsExposureSequenceable(bool& isSequenceable) const 
{
   isSequenceable = false; 
   return DEVICE_OK;
}

bool RPiV4L2::VideoInit(State*state)
{
  state->fd = open("/dev/video0", O_RDWR);
  //// this should probably be a property

  if (-1 == state-> fd)
  {
    LogMessage("v4l2: could not open the video device");
    return false;
  }

  sleep(3); // let it settle; there is probably an ioctl for this

  state->buf=(struct v4l2_buffer*) malloc(sizeof(struct v4l2_buffer));

  struct v4l2_format format; 
  memset(&format,0,sizeof(format));
  format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.width=state->W;
  format.fmt.pix.height=state->H;
  format.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;
  if(-1==ioctl(state->fd,VIDIOC_S_FMT,&format))
    {
    LogMessage("v4l2: could not set YUYV format");
    return false;
    }

  state->W=format.fmt.pix.width;
  state->H=format.fmt.pix.height;
  //printf("size %dx%d\n",format.fmt.pix.width,format.fmt.pix.height);
 
  struct v4l2_requestbuffers reqbuf;
  memset(&reqbuf,0,sizeof(reqbuf));
  reqbuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory=V4L2_MEMORY_MMAP;
  reqbuf.count=4;
  assert(-1!=ioctl(state->fd,VIDIOC_REQBUFS,&reqbuf));
  //printf("number of buffers: %d\n",reqbuf.count);
 
  state->buffers=(struct VidBuffer*)calloc(reqbuf.count,sizeof(*(state->buffers)));
  if(!state->buffers)
    {
    LogMessage("v4l2: could not allocate buffer(s)");
    return false;
    }
  unsigned int i;
  for(i=0;i<reqbuf.count;i++){
    struct v4l2_buffer buf;
    memset(&buf,0,sizeof(buf));
 
    buf.type=reqbuf.type;
    buf.memory=V4L2_MEMORY_MMAP;
    buf.index=i;
    if(-1==ioctl(state->fd,VIDIOC_QUERYBUF,&buf))
      {
      LogMessage("v4l2: could not query the buffer state");
      return false;
      }
 
    state->buffers[i].length=buf.length; // remember for munmap
    state->buffers[i].start=mmap(NULL,buf.length,
			  PROT_READ|PROT_WRITE,
			  MAP_SHARED,state->fd,buf.m.offset);
    if(state->buffers[i].start==MAP_FAILED)
      {
      LogMessage("v4l2: memory map failed");
      return false;
      }
 
    if(-1==ioctl(state->fd,VIDIOC_QBUF,&buf))
      {
      LogMessage("v4l2: could not enqueue buffer");
      return false;
      }
  }
  state->buffers_count=reqbuf.count;
  int type=V4L2_BUF_TYPE_VIDEO_CAPTURE; 
  if(-1 == ioctl(state->fd, VIDIOC_STREAMON, &type))
  {
    LogMessage("v4l2: could not initialize stream");
    return false;
  }
  return true;
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
  // FIXME
  //clear_roi();
  return DEVICE_OK;
}
