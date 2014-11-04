#ifndef __NHELPER_H__
#define __NHELPER_H__

#define GET8(p)    (*((uint8_t*)(p)))
#define SET8(p, v) (*((uint8_t*)(p))) = (uint8_t)(v)

#ifdef BIG_ENDIAN_MACHINE
#define GET16(p)   ((uint16_t)(GET8(p)<<8 | GET8((p)+1)))
#define GET32(p)   ((uint32_t)(GET16(p)<<16 | GET16((p)+2)))
#define SET16(p,v) SET8(p,(v)>>8); SET8((p)+1,v)
#define SET32(p,v) SET16(p,(v)>>16); SET16((p)+2,v)
#else
#define GET16(p)   ((uint16_t)(GET8(p) | GET8((p)+1)<<8))
#define GET32(p)   ((uint32_t)(GET16(p) | GET16((p)+2)<<16))
#define SET16(p,v) SET8(p,v); SET8((p)+1,((v)>>8))
#define SET32(p,v) SET16(p,v); SET16((p)+2, (v)>>16)
#endif

typedef int32_t ia_t;
typedef int32_t lsockid_t;
typedef uint32_t lsocktype_t;
typedef int32_t l_status_t;

#endif
