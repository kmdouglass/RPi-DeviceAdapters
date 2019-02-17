/**
 * Kyle M. Douglass, 2019
 * kyle.m.douglass@gmail.com
 *
 * Micro-Manager device adapter for a Video 4 Linux version 2 device.
 *
 * Notable properties of this device adapter include:
 *
 * - Only single plane video capture formats are used (V4L2_BUF_TYPE_VIDEO_CAPTURE)
 *
 * - GetNumberOfComponents returns the number of color channels per pixel. This can be confusing
 *   for some non-packed, single plane formats such as YUYV where each pixel contains only two
 *   color components.
 *
 * - In general, this device adapter is not compatible with the Micro-Manager Java wrapper. The
 *   reason for this is that the number of supported pixel formats are most limited in the Java
 *   layer, and the device adapter has no knowledge of what layer it is being called from.
 *
 * This device adapter is based on the original V4L2 device adapter for Micro-Manager by Martin
 * Kielhorn.
 */

#ifndef _RPiV4L2_H_
#define _RPiV4L2_H_

#include <linux/videodev2.h>
#include <string>
#include <vector>

#include "DeviceBase.h"
#include "ImgBuffer.h"

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
  unsigned GetNumberOfComponents() const;
  int GetROI(unsigned &x, unsigned &y,unsigned &xSize, unsigned &ySize);
  int SetBinning(int binSize);
  void SetExposure(double exp);
  int SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize);
  int SnapImage();

  // Action handlers
  int OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnDevice(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnExposure(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnField(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnFormatDescription(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnFormatFlags(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnGain(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnHeight(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnPixelType(MM::PropertyBase* pProp, MM::ActionType eAct);
  int OnWidth(MM::PropertyBase* pProp, MM::ActionType eAct);

  // Constants
  static const unsigned int BIT_DEPTH = 8;
  static const v4l2_buf_type BUF_TYPE = V4L2_BUF_TYPE_VIDEO_CAPTURE; // Single plane formats only
  static const __u32 FIELD = V4L2_FIELD_NONE;   // TODO Make this settable
  static const unsigned int MAX_HEIGHT = 32768;
  static const unsigned int MAX_WIDTH = 32768;
  static const unsigned int UNKNOWN_NUMBER_OF_COMPONENTS = 0;

  // TODO: Remove after refactor
  bool VideoInit(State *state);

private:
  int DequeueBuffer();
  void FindVideoDeviceFiles(std::vector<std::string> &devices);
  void GenerateReadOnlyProperties();
  void GetVideoDeviceFormatDescription();
  int InitMMAP();
  int InsertImage();
  int OpenVideoDevice();
  int PollDevice();
  int SetVideoDeviceFormat(unsigned int width, unsigned int height);
  int StartCapturing();
  int StopCapturing();
  static int xioctl(int fd, int request, void *arg);

  // V4L2 Buffers
  // These buffers are used to pass data between the device and device adapter. They are not
  // exposed to the public device adapter API. The public API accesses the ImgBuffer img_ instead.
  struct v4l2_buffer buffer_;               // The most recently accessed V4L2 buffer
  struct buffers {
    void *start;
    size_t length;
  } *buffers_;                              // Pointer to the V4L2 image buffers

  std::string current_device_;              // The current device managed by this device adapter
  std::vector< std::string > devices_;      // A list of devices found on the system
  int fd_;                                  // File descriptor for the video device
  struct v4l2_format fmt_;                  // The current capture format
  std::vector< v4l2_fmtdesc > fmtdescs_;    // Set of format descriptions supported by the device
  ImgBuffer img_;                           // The image buffer used by the MM API
  MMThreadLock imgLock_;                    // Lock for the image buffer
  unsigned int num_buffers_;                // The number of allocated image buffers
  struct v4l2_requestbuffers reqbuf_;       // Set of buffers requested from the device
  bool stopOnOverflow_;                     // Stop acquisition when the circular buffer if full?

  // MM API
  bool initialized_;

  // Error codes
  const int ERR_NO_VIDEO_DEVICE_FILES = 101;   // No /dev/video* files exist
  const int ERR_DEVICE_CHANGE_FORBIDDEN = 102; // Cannot change /dev file after initialization

  // Messages
  const int MSG_NEW_IMAGE_IN_BUFFER = 501;
  const int MSG_NO_NEW_IMAGE_IN_BUFFER = 502;

  // TODO: Remove after refactor
  State state[1];
  unsigned char *image;
};

#endif //_RPiV4L2_H_
