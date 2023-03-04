#include "icache_sim.h"

ICacheSim** icacheSim = NULL;

FILE* infofp = NULL;

static u_int64_t cycle;

char refillTypeStr[2][20] = {
  "MISSUNIT",
  "IPF",
};

/* ---------------------------- tools func begin ---------------------------- */
char* printRefillType(uint8_t t)
{
  return refillTypeStr[t];
}

void printRefillTrace(icache_sim_refill_event_t* refillEvent)
{
  icache_sim_refill_event_t* d = refillEvent;
  icachesim_info("<%ld>, %s:move data to meta sram:ptag=0x%lx,pidx=0x%lx,waymask=0x%x\n",\
  d->time, printRefillType(d->write_master), d->ptag, d->pidx, d->waymask);
}

u_int64_t getAlignPaddrFromPaddr(u_int64_t paddr)
{
  return (paddr >> BLOCK_OFFSET_BITS) << BLOCK_OFFSET_BITS;
}

u_int64_t getPidxFromPaddr(u_int64_t paddr) 
{
  return (paddr >> BLOCK_OFFSET_BITS) & PIDX_MASK;
}

u_int64_t getPtagFromPaddr(u_int64_t paddr)
{
  return (paddr >> (BLOCK_OFFSET_BITS + PIDX_OFFSET_BITS));
}

/* ---------------------------- tools func end ---------------------------- */

int ICacheSim::doRead(icache_sim_read_event_t* readEvent) 
{
  icache_sim_read_event_t* e = readEvent;
  for (int i = 0; i < ICACHE_READ_PORT_WIDTH; i++) {
    auto read = e->port[i];
    if (!read.valid) { continue; }
    totalReadReqNum++;
    if (read.hit_in_array || read.hit_in_ipf || read.hit_in_piq) {
      totalReadReqHitNum++;
    }
    if (!read.hit_in_array && read.hit_in_ipf) { 
      totalReadHitInIpfNum++; 
      struct IpfBufferItem* ipfItem = ipfBuffer->getIpfBufferItemByPaddr(read.hit_paddr);
      if (ipfItem != NULL) { 
        if (ipfItem->valid && !ipfItem->hasUsed) {
          totalReadFirstHitInIpfNum++;
          ipfItem->hasUsed = true;
          ipfItem->firstUseTime = read.time;
          accumulateIpfFirstHitInterval += (ipfItem->firstUseTime - ipfItem->refillTime);
        }
      }
    } else if (read.hit_in_array) {
      u_int64_t ptag = getPtagFromPaddr(read.hit_paddr);
      u_int64_t pidx = getPidxFromPaddr(read.hit_paddr);
      struct MetaArrayItem* arrayItem = metaArray->getMetaArrayItemByPidxAndPtag(pidx, ptag);
      if (arrayItem != NULL && arrayItem->valid) {
        if (arrayItem->lastRefillMaster == ICacheRefillMaster::IPF_IRM) {
          metaArray->accumulateRefillFromIpfHitNum++;
        }
      }
    }

  }
  return 0;
}

int ICacheSim::step()
{
  cycle++;
#ifdef ENABLE_PERF_COUNTER
  doRead(&(dut_ptr->icache_sim_read));
  metaArray->checkMultiHit(&(dut_ptr->icache_sim_refill));
  metaArray->doRefill(&(dut_ptr->icache_sim_refill));
  ipfBuffer->doRefill(&(dut_ptr->icache_sim_ipf_refill));
#endif
  return 0;
}

int ICacheSim::dump()
{
  icachesim_info("totalReadReqNum = %ld\ntotalReadReqHitNum = %ld\ntotalReadHitInIpfNum = %ld\n",\
    totalReadReqNum, totalReadReqHitNum, totalReadHitInIpfNum);
  icachesim_info("totalReadFirstHitInIpfNum = %ld\naccumulateIpfFirstHitInterval = %ld\n",\
    totalReadFirstHitInIpfNum, accumulateIpfFirstHitInterval);
  icachesim_info("prefetch timeliness (cycle) = %f\n", \
    (double)accumulateIpfFirstHitInterval / (double)totalReadFirstHitInIpfNum);
  icachesim_info("accumulateRefillFromIpfNum = %ld\naccumulateRefillFromIpfHitNum = %ld\n",\
    metaArray->accumulateRefillFromIpfNum, metaArray->accumulateRefillFromIpfHitNum);
  icachesim_info("ipfRefillHitTimeAverage = %f\n", \
    (double)(metaArray->accumulateRefillFromIpfHitNum) / (double)(metaArray->accumulateRefillFromIpfNum));
  icachesim_info("ipfneverUsedItemsNum = %ld\n", ipfBuffer->neverUsedItemsNum);

  return 0;
}

int ICacheSim::clearPerfInfo()
{
  this->totalReadReqNum = 0;
  this->totalReadReqHitNum = 0;
  this->totalReadHitInIpfNum = 0;
  this->totalReadFirstHitInIpfNum = 0;
  this->accumulateIpfFirstHitInterval = 0;
  this->metaArray->accumulateRefillFromIpfHitNum = 0;
  this->metaArray->accumulateRefillFromIpfNum = 0;
  this->ipfBuffer->neverUsedItemsNum = 0;
  return 0;
}

struct MetaArrayItem* MetaArray::getMetaArrayItemByPidxAndWay(u_int64_t pidx, int wayId)
{
  return &metaArray[pidx][wayId];
}

struct MetaArrayItem* MetaArray::getMetaArrayItemByPidxAndPtag(u_int64_t pidx, u_int64_t ptag) 
{
  for (int i = 0 ; i < WAY_NUM; i++) {
    if (metaArray[pidx][i].ptag == ptag && metaArray[pidx][i].valid) {
      return &metaArray[pidx][i];
    }
  }
  return NULL;
}

int MetaArray::checkMultiHit(icache_sim_refill_event_t* refillEvent)
{
  if (!refillEvent->valid) {
    return 0;
  }
  int hasMultiHit = 0;
  icache_sim_refill_event_t* d = refillEvent;
  for (int i = 0; i < WAY_NUM; i++) {
    struct MetaArrayItem* item = getMetaArrayItemByPidxAndWay(d->pidx, i);
    if (item->valid && item->ptag == d->ptag) {
      hasMultiHit = 1;
      icachesim_info("multiHitTime=%ld, rType=%d, pIdx=0x%lx,ptag=0x%lx,lastRTime=%ld, lastRType=%d, matchWay=0x%x\n",\
        d->time, d->write_master, d->pidx, d->ptag, item->lastUpdateTime,\
        item->lastRefillMaster, i);
    }
  }
  return hasMultiHit;
}

int MetaArray::doRefill(icache_sim_refill_event_t* refillEvent)
{
  int way;
  icache_sim_refill_event_t* d = refillEvent;
  if (!d->valid) { 
    return 0;
  }
  switch (d->waymask)
  {
    case 0x1: way = 0; break;
    case 0x2: way = 1; break;
    case 0x4: way = 2; break;
    case 0x8: way = 3; break;
    case 0x10: way = 4; break;
    case 0x20: way = 5; break;
    case 0x40: way = 6; break;
    case 0x80: way = 7; break;
    default:
      icachesim_debug("way error\n");
      return -1;
  }
  metaArray[d->pidx][way].lastRefillMaster = d->write_master == 0 ? MISSUNIT_IRM : IPF_IRM;
  metaArray[d->pidx][way].lastUpdateTime = d->time;
  metaArray[d->pidx][way].ptag = d->ptag;
  metaArray[d->pidx][way].valid = true;
  if (d->write_master == 1) {
    accumulateRefillFromIpfNum++;
  }
  return 0;
}

int MetaArray::initMetaArray()
{
  for (int i = 0; i < SET_NUM; i++) {
    for (int j = 0; j < WAY_NUM; j++) {
      metaArray[i][j].valid = false;
    }
  }
  accumulateRefillFromIpfNum = 0;
  accumulateRefillFromIpfHitNum = 0;
  return 0;
}

/* TODO : */
int IpfBuffer::doRefill(icache_sim_ipf_refill_event_t* refillEvent)
{
  icache_sim_ipf_refill_event_t* e = refillEvent;
  if (!e->valid) { return 0; }
  uint64_t alignPaddr = getAlignPaddrFromPaddr(e->paddr);
  if (ipfBuffer[e->write_ptr].valid != false) {
    if (ipfBuffer[e->write_ptr].hasUsed == false) {
      neverUsedItemsNum++;
    }
    paddrToIpfItem.erase(ipfBuffer[e->write_ptr].alignPaddr);
  }
  ipfBuffer[e->write_ptr].valid = true;
  ipfBuffer[e->write_ptr].alignPaddr = alignPaddr;
  ipfBuffer[e->write_ptr].hasUsed = false;
  ipfBuffer[e->write_ptr].refillTime = e->time;
  paddrToIpfItem[alignPaddr] = &ipfBuffer[e->write_ptr];
  return 0;
}

struct IpfBufferItem* IpfBuffer::getIpfBufferItemByPaddr(u_int64_t paddr)
{
  u_int64_t alignPaddr = getAlignPaddrFromPaddr(paddr);
  if (paddrToIpfItem.count(alignPaddr) == 0) {
    return NULL;
  }
  return paddrToIpfItem[alignPaddr];
}

int IpfBuffer::initIpfBuffer()
{
  neverUsedItemsNum = 0;
  for (int i = 0; i < IPF_BUFFER_ENTRY_NUM; i++) {
    ipfBuffer[i].valid = false;
    ipfBuffer[i].hasUsed = false;
  }
  paddrToIpfItem.clear();
  return 0;
}


int icache_sim_init()
{
  icacheSim = new ICacheSim*[NUM_CORES];
  assert(difftest);
  for (int i = 0; i < NUM_CORES; i++) {
    icacheSim[i] = new ICacheSim();
    icacheSim[i]->totalReadHitInIpfNum = 0;
    icacheSim[i]->totalReadReqNum = 0;
    icacheSim[i]->totalReadReqHitNum = 0;
    icacheSim[i]->accumulateIpfFirstHitInterval = 0;
    icacheSim[i]->metaArray = new MetaArray();
    icacheSim[i]->metaArray->initMetaArray();
    icacheSim[i]->ipfBuffer = new IpfBuffer();
    icacheSim[i]->ipfBuffer->initIpfBuffer();
    icacheSim[i]->dut_ptr = difftest[i]->get_dut();
  }
  infofp = fopen("icache_sim_info.log", "w");
  icachesim_debug("icache_sim init success\n");
  return 0;
}

int icache_sim_step()
{
  for (int i = 0; i < NUM_CORES; i++) {
    int ret = icacheSim[i]->step();
    if (ret) {
      return ret;
    }
  }
  return 0;
}

int icache_sim_dump()
{
  icachesim_info("\n\n\n---------------- dump result at cycle %ld begin ---------------------\n", cycle);
  for (int i = 0; i < NUM_CORES; i++) {
    icachesim_info("coreid = %d\n\n", i);
    icacheSim[i]->dump();
  }
  icachesim_info("\n---------------- dump result at cycle %ld done ---------------------\n", cycle);
  return 0;
}

int icache_sim_clean_perf_info()
{
  for (int i = 0; i < NUM_CORES; i++) {
    icacheSim[i]->clearPerfInfo();
  }
  return 0;
}

/* ideal cache */
int ICacheSim::refillIdealCache(icache_sim_ideal_refill_t* event)
{
  if (!event->valid) return 0;
  uint64_t alignPaddr = getAlignPaddrFromPaddr(event->paddr);
  if (paddr2cacheline.count(alignPaddr) != 0) {
    icachesim_info("refill ideal cache error:paddr 0x%lx already in\n", alignPaddr);
  }
  uint64_t* cacheline = (uint64_t*)malloc(sizeof(uint64_t) * CACHELINE_BEAT_NUM);
  for (int i = 0; i < CACHELINE_BEAT_NUM; i++) {
    cacheline[i] = event->data[i];
  }
  paddr2cacheline[alignPaddr] = cacheline;
  return 0;
}

int ICacheSim::doReadIdealCache(icache_sim_ideal_read_t* event, int port)
{
  if (event->valid[port] == 0) {
    event->port[port].isHit = 0;
    return 0;
  }
  uint64_t alignPaddr = getAlignPaddrFromPaddr(event->paddr[port]);
  if (paddr2cacheline.count(alignPaddr) == 0) {
    event->port[port].isHit = 0;
    return 0;
  }
  event->port[port].isHit = 1;
  uint64_t* cacheline = paddr2cacheline[alignPaddr];
  for (int i = 0; i < CACHELINE_BEAT_NUM; i++) {
    event->port[port].hitData[i] = cacheline[i];
  }
  return 0;
}