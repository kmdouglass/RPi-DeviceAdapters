AM_CXXFLAGS = $(MMDEVAPI_CXXFLAGS) -std=c++11
deviceadapter_LTLIBRARIES = libmmgr_dal_RPiV4L2.la
libmmgr_dal_RPiV4L2_la_SOURCES = RPiV4L2.cpp RPiV4L2.h refactor.h \
  ../../MMDevice/MMDevice.h ../../MMDevice/DeviceBase.h
libmmgr_dal_RPiV4L2_la_LIBADD = $(MMDEVAPI_LIBADD)
libmmgr_dal_RPiV4L2_la_LDFLAGS = $(MMDEVAPI_LDFLAGS) -lboost_regex -lboost_filesystem
