#include "icache_sim.h"

ICacheSim** icacheSim = NULL;

FILE* infofp = NULL;

char refillTypeStr[2][20] = {
  "MISSUNIT",
  "IPF",
};

/* ---------------------------- tools func begin ---------------------------- */
char* printRefillType(uint8_t t)
{
  return refillTypeStr[t];
}

void printRefillTrace(icache_sim_debug_event_t* debugEvent)
{
  icache_sim_debug_event_t* d = debugEvent;
  icachesim_info("<%ld>, %s:move data to meta sram:ptag=0x%lx,pidx=0x%lx,waymask=0x%x\n",\
  d->time, printRefillType(d->write_master), d->ptag, d->pidx, d->waymask);
}

/* ---------------------------- tools func end ---------------------------- */

ICacheSim::ICacheSim()
{
}

ICacheSim::~ICacheSim()
{
}

int ICacheSim::step()
{
  if (!dut_ptr->icache_sim_debug_event.write_en) {
    return 0;
  }
  int r;
  r = metaArray->checkMultiHit(&(dut_ptr->icache_sim_debug_event));
  r = metaArray->doRefill(&(dut_ptr->icache_sim_debug_event));
  printRefillTrace(&(dut_ptr->icache_sim_debug_event));
  return 0;
}

MetaArray::MetaArray()
{
}

MetaArray::~MetaArray()
{
}

struct MetaArrayItem* MetaArray::getMetaArrayItem(int pidx, int wayId)
{
  return &metaArray[pidx][wayId];
}

int MetaArray::checkMultiHit(icache_sim_debug_event_t* debugEvent)
{
  int hasMultiHit = 0;
  icache_sim_debug_event_t* d = debugEvent;
  for (int i = 0; i < WAY_NUM; i++) {
    struct MetaArrayItem* item = getMetaArrayItem(debugEvent->pidx, i);
    if (item->valid && item->ptag == debugEvent->ptag) {
      hasMultiHit = 1;
      icachesim_info("multiHitTime=%ld, rType=%d, pIdx=0x%lx,ptag=0x%lx,lastRTime=%ld, lastRType=%d, matchWay=0x%x\n",\
        d->time, d->write_master, d->pidx, d->ptag, item->lastUpdateTime,\
        item->lastRefillMaster, i);
    }
  }
  return hasMultiHit;
}

int MetaArray::doRefill(icache_sim_debug_event_t* debugEvent)
{
  int way;
  switch (debugEvent->waymask)
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
  metaArray[debugEvent->pidx][way].lastRefillMaster = 
    debugEvent->write_master == 0 ? MISSUNIT_IRM : IPF_IRM;
  metaArray[debugEvent->pidx][way].lastUpdateTime = debugEvent->time;
  metaArray[debugEvent->pidx][way].ptag = debugEvent->ptag;
  metaArray[debugEvent->pidx][way].valid = true;
  return 0;
}

int MetaArray::clearMetaArray()
{
  for (int i = 0; i < SET_NUM; i++) {
    for (int j = 0; j < WAY_NUM; j++) {
      metaArray[i][j].valid = false;
    }
  }
  return 0;
}

int icache_sim_init()
{
  icacheSim = new ICacheSim*[NUM_CORES];
  assert(difftest);
  for (int i = 0; i < NUM_CORES; i++) {
    icacheSim[i] = new ICacheSim();
    icacheSim[i]->metaArray = new MetaArray();
    icacheSim[i]->metaArray->clearMetaArray();
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