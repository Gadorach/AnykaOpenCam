/* Explicit little-endian stock ARM32 wire shapes, not host timeval ABI. */
#ifndef CR_WIRE_H
#define CR_WIRE_H
#include <stddef.h>
#include <stdint.h>
enum { CR_BLOB=69716, CR_SLOT=2097152, CR_MAP=8388608, CR_PAGES=2048,
       CR_REF_BYTES=0x384000, CR_REF_PAGES=900,
       CR_MAIN=1382400, CR_EXTENT=1729024, CR_Y=921600, CR_THUMB=14400,
       CR_MAX_DQ=512, CR_OUTPUT=61440, CR_REGS=12 };
struct cr_buffer {
    uint32_t index,type,bytesused,flags,field;
    int32_t sec,usec;
    uint8_t timecode[16];
    uint32_t sequence,memory,userptr,length,input,reserved;
};
struct cr_reqbuf { uint32_t count,type,memory,reserved[2]; };
struct cr_format { uint32_t type; uint8_t data[200]; };
struct cr_parm { uint32_t type; uint8_t data[200]; };
struct cr_alloc { uint32_t len,align,mask,flags,handle; };
struct cr_export { uint32_t handle; int32_t fd; };
struct cr_sensor { uint32_t count,ptr; };
struct cr_reference { uint32_t words[6]; }; /* address,length pairs, not six addresses */
struct cr_crop { uint32_t type; int32_t left,top; uint32_t width,height; };
#define CR_REFERENCE_SET 0x40044967UL
#define CR_REFERENCE_GET 0x80044968UL
#define CR_S_FMT 0xc0cc5605UL
#define CR_G_FMT 0xc0cc5604UL
#define CR_S_PARM 0xc0cc5616UL
#define CR_S_CROP 0x4014563cUL
#define CR_G_CROP 0xc014563bUL
#define CR_ISP_PHYSICAL_BASE 0x20000000U
#define CR_ISP_CAPTURE_GATE (1U<<20)
#define CR_REQBUFS 0xc0145608UL
#define CR_QBUF 0xc044560fUL
#define CR_DQBUF 0xc0445611UL
#define CR_STREAMON 0x40045612UL
#define CR_STREAMOFF 0x40045613UL
#define CR_SENSOR_LOAD 0x40044969UL
#define CR_SENSOR_GET 0x8004496cUL
#define CR_ION_ALLOC 0xc0144900UL
#define CR_ION_MAP 0xc0084902UL
_Static_assert(sizeof(struct cr_buffer)==68, "stock buffer68");
_Static_assert(offsetof(struct cr_buffer,sec)==20 && offsetof(struct cr_buffer,sequence)==44, "stock timestamps/sequence");
_Static_assert(offsetof(struct cr_buffer,memory)==48 && offsetof(struct cr_buffer,userptr)==52 && offsetof(struct cr_buffer,length)==56, "stock USERPTR");
_Static_assert(sizeof(struct cr_reqbuf)==20 && sizeof(struct cr_format)==204 && sizeof(struct cr_parm)==204, "stock wire lengths");
_Static_assert(sizeof(struct cr_alloc)==20 && sizeof(struct cr_export)==8 && sizeof(struct cr_sensor)==8, "ION/sensor lengths");
_Static_assert(sizeof(struct cr_reference)==24 && sizeof(struct cr_crop)==20, "reference/crop wire");
#endif
