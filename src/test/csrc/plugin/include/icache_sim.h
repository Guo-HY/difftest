#ifndef ICACHE_SIM_H
#define ICACHE_SIM_H

#include "common.h"
#include "difftest.h"

// TODO this is ugly
#define SET_NUM 128
#define PIDX_MASK 0x7f
#define WAY_NUM 8
#define BLOCK_OFFSET_BITS 6

class ICacheSim;
class MetaArray;

enum ICacheRefillMaster {
  MISSUNIT_IRM,
  IPF_IRM,
};


class ICacheSim {
  public:
  MetaArray* metaArray = NULL;

  ICacheSim();
  ~ICacheSim();

  int step();

  difftest_core_state_t* dut_ptr = NULL;

};

struct MetaArrayItem {
  bool valid;
  u_int64_t lastUpdateTime;
  ICacheRefillMaster lastRefillMaster;
  u_int32_t ptag;
};

class MetaArray {
  public:

  struct MetaArrayItem metaArray[SET_NUM][WAY_NUM];

  MetaArray();
  ~MetaArray();

  struct MetaArrayItem* getMetaArrayItem(int pidx, int wayId); 

  int checkMultiHit(icache_sim_debug_event_t* debugEvent);
  int doRefill(icache_sim_debug_event_t* debugEvent);
  int clearMetaArray();

};

extern ICacheSim** icacheSim;
int icache_sim_init();
int icache_sim_step();

extern FILE* infofp;

#define ICACHE_SIM_DEBUG 1
#ifdef ICACHE_SIM_DEBUG
#define icachesim_debug(...) \
  do { \
    eprintf(__VA_ARGS__); \
  }while(0)
#else
#define icachesim_debug(...)
#endif

#define icachesim_info(...) fprintf(infofp, ## __VA_ARGS__)

#endif