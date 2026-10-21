#include "wire.h"
#include ORIGINAL_V4L2_HEADER
_Static_assert(sizeof(void*)==4, "compile ARM only");
_Static_assert(sizeof(struct v4l2_buffer)==68, "original stock time32 buffer");
_Static_assert(sizeof(struct v4l2_requestbuffers)==20, "original reqbuf");
_Static_assert(sizeof(struct v4l2_format)==204 && sizeof(struct v4l2_streamparm)==204, "original format/parm");
_Static_assert(offsetof(struct v4l2_buffer,timestamp)==20 && offsetof(struct v4l2_buffer,sequence)==44, "stock offsets");
_Static_assert(offsetof(struct v4l2_buffer,m)==52 && offsetof(struct v4l2_buffer,length)==56, "stock pointer/length");
_Static_assert(VIDIOC_QBUF==CR_QBUF && VIDIOC_DQBUF==CR_DQBUF && VIDIOC_REQBUFS==CR_REQBUFS, "stock queue requests");
_Static_assert(VIDIOC_STREAMON==CR_STREAMON && VIDIOC_STREAMOFF==CR_STREAMOFF, "stock stream requests");
_Static_assert(sizeof(struct v4l2_crop)==20 && VIDIOC_S_CROP==CR_S_CROP && VIDIOC_G_CROP==CR_G_CROP, "stock crop request");
_Static_assert(VIDIOC_S_FMT==CR_S_FMT && VIDIOC_G_FMT==CR_G_FMT && VIDIOC_S_PARM==CR_S_PARM, "stock geometry requests");
_Static_assert(V4L2_MEMORY_USERPTR==2 && V4L2_BUF_TYPE_VIDEO_CAPTURE==1 && V4L2_BUF_FLAG_ERROR==0x40, "stock enums");
const uint32_t cr_abi_values[]={sizeof(struct v4l2_buffer),VIDIOC_QBUF,VIDIOC_DQBUF,VIDIOC_REQBUFS,VIDIOC_STREAMON,VIDIOC_STREAMOFF};
