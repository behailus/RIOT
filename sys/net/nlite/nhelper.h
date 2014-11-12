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


#include <assert.h>
#define NB_ASSERT assert

typedef int32_t ia_t;
typedef int32_t lsockid_t;
typedef uint32_t lsocktype_t;
typedef int32_t l_status_t;

typedef unsigned int sid_t;
typedef unsigned int vdid_t;

struct naddr {
  sid_t        sid;     /* service identifier */
  unsigned int port;      /* port of the sid */
  vdid_t       device; /* virtual device id */
};
typedef struct naddr naddr_t;
struct l_in
{

};
typedef struct l_in l_in_t;

struct ld_module {
  int state;
  struct ld_ops *ops; /* ops list, allocated by l_in_down. */
  struct ld *ld;

  int scene_running;

  int ready;
  ld_module_init_func init_func;
  ld_module_destroy_func destroy_func;

  struct l_in *l_in;
  struct ld_module *next;
};
typedef struct ld_module ld_module_t;
#endif
