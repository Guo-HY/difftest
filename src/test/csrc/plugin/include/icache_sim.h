#ifndef ICACHE_SIM_H
#define ICACHE_SIM_H

#include "common.h"
#include "difftest.h"
#include <vector>
#include <unordered_map>
#include <string>

// #define ENABLE_PERF_COUNTER 1
#define ENABLE_IDEAL_ICACHE 1

// TODO this is ugly
// #define SET_NUM 128
// #define PIDX_OFFSET_BITS 7
// #define PIDX_MASK 0x7f

#define SET_NUM 64
#define PIDX_OFFSET_BITS 6
#define PIDX_MASK 0x3f

#define WAY_NUM 8
#define BLOCK_OFFSET_BITS 6

#define ICACHE_READ_PORT_WIDTH 2

#define IPF_BUFFER_ENTRY_NUM 64

#define CACHELINE_BEAT_NUM 8

class ICacheSim;
class MetaArray;
class IpfBuffer;

enum ICacheRefillMaster {
  MISSUNIT_IRM,
  IPF_IRM,
};


class ICacheSim {
  public:
  MetaArray* metaArray = NULL;
  IpfBuffer* ipfBuffer = NULL;

  u_int64_t totalReadReqNum = 0;
  u_int64_t totalReadReqHitNum = 0;
  u_int64_t totalReadHitInIpfNum = 0;
  u_int64_t totalReadFirstHitInIpfNum = 0;
  u_int64_t accumulateIpfFirstHitInterval = 0;

  int doRead(icache_sim_read_event_t* readEvent);

  int step();
  int dump();
  int clearPerfInfo();

  difftest_core_state_t* dut_ptr = NULL;

  /* ideal cache */
  std::unordered_map<uint64_t, u_int64_t*> paddr2cacheline;
  int refillIdealCache(icache_sim_ideal_refill_t* event);
  int doReadIdealCache(icache_sim_ideal_read_t* event, int index);

};

struct MetaArrayItem {
  bool valid;
  u_int64_t lastUpdateTime;
  ICacheRefillMaster lastRefillMaster;
  u_int64_t ptag;
};

class MetaArray {
  public:

  struct MetaArrayItem metaArray[SET_NUM][WAY_NUM];

  u_int64_t accumulateRefillFromIpfNum = 0;
  u_int64_t accumulateRefillFromIpfHitNum = 0;

  struct MetaArrayItem* getMetaArrayItemByPidxAndWay(u_int64_t pidx, int wayId); 
  struct MetaArrayItem* getMetaArrayItemByPidxAndPtag(u_int64_t pidx, u_int64_t ptag);

  int checkMultiHit(icache_sim_refill_event_t* debugEvent);
  int doRefill(icache_sim_refill_event_t* debugEvent);
  int initMetaArray();

};

struct IpfBufferItem {
  bool valid;
  u_int64_t refillTime;
  u_int64_t alignPaddr;
  bool hasUsed;
  u_int64_t firstUseTime;
};

class IpfBuffer {
  public:

  u_int64_t neverUsedItemsNum = 0;

  struct IpfBufferItem ipfBuffer[IPF_BUFFER_ENTRY_NUM];
  std::unordered_map<u_int64_t, IpfBufferItem*> paddrToIpfItem;
  
  struct IpfBufferItem* getIpfBufferItemByPaddr(u_int64_t paddr);
  int doRefill(icache_sim_ipf_refill_event_t* refillEvent);
  int initIpfBuffer();

};

extern ICacheSim** icacheSim;
int icache_sim_init();
int icache_sim_step();
int icache_sim_dump();
int icache_sim_clean_perf_info();

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