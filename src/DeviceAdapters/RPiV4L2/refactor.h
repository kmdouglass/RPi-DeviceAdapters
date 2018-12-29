#ifndef _REFACTOR_H_
#define _REFACTOR_H_

#include <linux/videodev2.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct State State;
struct State {
  int W, H, fd;
  struct VidBuffer *buffers;
  unsigned int buffers_count;
  struct v4l2_buffer *buf;
};

const int gWidth=640, gHeight=480;

struct VidBuffer {
  void*start;
  size_t length;
};

/*  has to be followed by a call to videoreturnbuffer */
unsigned char* VideoTakeBuffer(State* state)
{
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(state->fd,&fds);
  struct timeval tv;
  tv.tv_sec=10;
  tv.tv_usec=0;
  assert(0<select(state->fd+1,&fds,NULL,NULL,&tv));
 
  memset(state->buf,0,sizeof(struct v4l2_buffer));
  state->buf->type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
  state->buf->memory=V4L2_MEMORY_MMAP;
  // http://v4l2spec.bytesex.org/spec/r12878.htm: By default
  // VIDIOC_DQBUF blocks when no buffer is in the outgoing queue
  assert(-1!=ioctl(state->fd,VIDIOC_DQBUF,state->buf));

  assert(state->buf->index<state->buffers_count);

  return (unsigned char*)state->buffers[state->buf->index].start;
}

void VideoReturnBuffer(State*state)
{
  assert(-1!=ioctl(state->fd,VIDIOC_QBUF,state->buf));
}

void VideoClose(State*state)
{
  int type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
  assert(-1!=ioctl(state->fd,VIDIOC_STREAMOFF,&type));
  unsigned int i;
  for(i=0;i<state->buffers_count;i++)
    munmap(state->buffers[i].start,state->buffers[i].length);
  close(state->fd);
  free(state->buf);
}


#endif // _REFACTOR_H_
