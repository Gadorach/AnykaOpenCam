/* Compile-only comparison against original3.4 header and pinned5.4 sysroot. */
#include <stdint.h>
#include <stddef.h>
#include "stock_wire.h"
#ifdef ORIGINAL_V4L2_HEADER
#include ORIGINAL_V4L2_HEADER
#else
#include <linux/videodev2.h>
#endif
_Static_assert(sizeof(void *) == 4 && sizeof(int) == 4, "ARM32 comparison only");
_Static_assert(sizeof(struct v4l2_capability) == 104, "header cap");
_Static_assert(sizeof(struct v4l2_fmtdesc) == 64, "header enum");
_Static_assert(sizeof(struct v4l2_format) == 204, "header format");
_Static_assert(offsetof(struct v4l2_format, fmt) == 4, "header format union offset");
_Static_assert(offsetof(struct v4l2_fmtdesc, pixelformat) == 44, "header enum fourcc");
_Static_assert(offsetof(struct v4l2_pix_format, bytesperline) == 16, "header stride");
_Static_assert(offsetof(struct v4l2_pix_format, sizeimage) == 20, "header sizeimage");
_Static_assert(offsetof(struct v4l2_capability, version) == 80, "header cap version");
_Static_assert(VIDIOC_QUERYCAP == STOCK_QUERYCAP, "header QUERYCAP request");
_Static_assert(VIDIOC_G_FMT == STOCK_G_FMT, "header G_FMT request");
_Static_assert(VIDIOC_ENUM_FMT == STOCK_ENUM_FMT, "header ENUM_FMT request");
_Static_assert(V4L2_BUF_TYPE_VIDEO_CAPTURE == 1, "header capture enum");
const uint32_t header_values[] = {
    sizeof(struct v4l2_capability), sizeof(struct v4l2_format),
    sizeof(struct v4l2_fmtdesc), sizeof(struct v4l2_pix_format),
    VIDIOC_QUERYCAP, VIDIOC_G_FMT, VIDIOC_ENUM_FMT,
    offsetof(struct v4l2_format, fmt), offsetof(struct v4l2_fmtdesc, pixelformat)
};
