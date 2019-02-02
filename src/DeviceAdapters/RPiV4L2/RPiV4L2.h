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

#ifndef _RPiV4L2_H_
#define _RPiV4L2_H_

#include <linux/videodev2.h>
#include <string>
#include <vector>

#include "DeviceBase.h"

#include "refactor.h" // TODO remove after refactoring

class RPiV4L2 : public CCameraBase<RPiV4L2>
{
public:
  RPiV4L2();
  ~RPiV4L2();

  // MMDevice API
  int Initialize();
  int Shutdown();
  bool Busy();
  int ClearROI();
  int IsExposureSequenceable(bool &isSequenceable) const;
  int GetBinning() const;
  unsigned GetBitDepth() const;
  double GetExposure() const;
  const unsigned char* GetImageBuffer();
  long GetImageBufferSize() const;
  unsigned GetImageBytesPerPixel() const;
  unsigned GetImageHeight() const;
  unsigned GetImageWidth() const;
  void GetName(char* name) const;
  int GetROI(unsigned &x, unsigned &y,unsigned &xSize, unsigned &ySize);
  int SetBinning(int binSize);
  void SetExposure(double exp);
  int SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize);
  int SnapImage();

  // Action handlers
  int OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnDevice(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnExposure(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnGain(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnPixelType(MM::PropertyBase* pProp, MM::ActionType eAct);

  // Constants
  static const unsigned int MAX_HEIGHT = 32768;
  static const unsigned int MAX_WIDTH = 32768;

  // TODO: Remove after refactor
  bool VideoInit(State *state);

private:
  void FindVideoDeviceFiles(std::vector<std::string> &devices);
  void GetVideoDeviceFormatDescription();
  int OpenVideoDevice();
  int SetVideoDeviceFormat();
  static int xioctl(int fd, int request, void *arg);

  std::string current_device_;              // The current device managed by this device adapter
  std::vector< std::string > devices_;      // A list of devices found on the system
  int fd_;                                  // File descriptor for the video device
  std::vector< v4l2_fmtdesc > fmtdescs_;    // Set of format descriptions supported by the device
  unsigned int height_;                     // The current height of the image
  unsigned int width_;                      // The current width of the image

  // MM API
  bool initialized_;

  // Error codes
  const int ERR_NO_VIDEO_DEVICE_FILES = 101;   // No /dev/video* files exist
  const int ERR_DEVICE_CHANGE_FORBIDDEN = 102; // Cannot change /dev file after initialization

  // TODO: Remove after refactor
  State state[1];
  unsigned char *image;
};

#endif //_RPiV4L2_H_
