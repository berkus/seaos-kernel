#ifndef __ASM_SYSTEM_H
#define __ASM_SYSTEM_H
#define sti() if(tables) __asm__ __volatile__ ("sti": : :"memory")
#define cli() __asm__ __volatile__ ("cli")
#define nop() __asm__ __volatile__ ("nop")
extern char tables;

#define BS8(x) (x)
#define BS16(x) (((x&0xFF00)>>8)|((x&0x00FF)<<8))
#define BS32(x) (((x&0xFF000000)>>24)|((x&0x00FF0000)>>8)|((x&0x0000FF00)<<8)|((x&0x000000FF)<<24))
#define BS64(x) (x)
#define LITTLE_ENDIAN
#ifdef LITTLE_ENDIAN

#define LITTLE_TO_HOST8(x) (x)
#define LITTLE_TO_HOST16(x) (x)
#define LITTLE_TO_HOST32(x) (x)
#define LITTLE_TO_HOST64(x) (x)

#define HOST_TO_LITTLE8(x) (x)
#define HOST_TO_LITTLE16(x) (x)
#define HOST_TO_LITTLE32(x) (x)
#define HOST_TO_LITTLE64(x) (x)

#define BIG_TO_HOST8(x) BS8((x))
#define BIG_TO_HOST16(x) BS16((x))
#define BIG_TO_HOST32(x) BS32((x))
#define BIG_TO_HOST64(x) BS64((x))

#define HOST_TO_BIG8(x) BS8((x))
#define HOST_TO_BIG16(x) BS16((x))
#define HOST_TO_BIG32(x) BS32((x))
#define HOST_TO_BIG64(x) BS64((x))

#else // else Big endian

#define BIG_TO_HOST8(x) (x)
#define BIG_TO_HOST16(x) (x)
#define BIG_TO_HOST32(x) (x)
#define BIG_TO_HOST64(x) (x)

#define HOST_TO_BIG8(x) (x)
#define HOST_TO_BIG16(x) (x)
#define HOST_TO_BIG32(x) (x)
#define HOST_TO_BIG64(x) (x)

#define LITTLE_TO_HOST8(x) BS8((x))
#define LITTLE_TO_HOST16(x) BS16((x))
#define LITTLE_TO_HOST32(x) BS32((x))
#define LITTLE_TO_HOST64(x) BS64((x))

#define HOST_TO_LITTLE8(x) BS8((x))
#define HOST_TO_LITTLE16(x) BS16((x))
#define HOST_TO_LITTLE32(x) BS32((x))
#define HOST_TO_LITTLE64(x) BS64((x))

#endif

#endif
