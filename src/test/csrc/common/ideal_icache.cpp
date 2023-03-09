

#include <unordered_map>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

std::unordered_map<uint64_t, u_int64_t*> paddr2cacheline;

static u_int64_t getAlignPaddrFromPaddr(u_int64_t paddr)
{
  return (paddr >> 6) << 6;
}

extern "C" void readIdealIcache(
  uint64_t gtimer,
  uint8_t valid,
  uint8_t port,
  uint64_t paddr,
  uint8_t* hitInIdealICache,
  uint64_t* hitData_0,
  uint64_t* hitData_1,
  uint64_t* hitData_2,
  uint64_t* hitData_3,
  uint64_t* hitData_4,
  uint64_t* hitData_5,
  uint64_t* hitData_6,
  uint64_t* hitData_7
)
{
  *hitInIdealICache = 0;
  if (valid == 0) {
    return;
  }
  uint64_t alignPaddr = getAlignPaddrFromPaddr(paddr);
  if (paddr2cacheline.count(alignPaddr) != 0) {
    *hitInIdealICache = 1;
    uint64_t* cacheline = paddr2cacheline[alignPaddr];
    *hitData_0 = cacheline[0];
    *hitData_1 = cacheline[1];
    *hitData_2 = cacheline[2];
    *hitData_3 = cacheline[3];
    *hitData_4 = cacheline[4];
    *hitData_5 = cacheline[5];
    *hitData_6 = cacheline[6];
    *hitData_7 = cacheline[7];
    // printf("{%ld} read hit paddr=0x%lx,readdata=0x%lx%lx%lx%lx%lx%lx%lx%lx\n",gtimer, alignPaddr,
    // cacheline[7],cacheline[6],cacheline[5],cacheline[4],cacheline[3],cacheline[2],cacheline[1],cacheline[0]);
  }

}


extern "C" void refillIdealIcache(
  uint64_t gtimer,
  uint8_t valid,
  uint64_t paddr,
  uint64_t data_0,
  uint64_t data_1,
  uint64_t data_2,
  uint64_t data_3,
  uint64_t data_4,
  uint64_t data_5,
  uint64_t data_6,
  uint64_t data_7
)
{
  if (valid == 0) {
    return;
  }
  uint64_t alignPaddr = getAlignPaddrFromPaddr(paddr);
  if (paddr2cacheline.count(alignPaddr) != 0) {
    printf("refill ideal cache error:paddr 0x%lx already in\n", alignPaddr);
    return;
  }
  uint64_t* cacheline = (uint64_t*)malloc(sizeof(uint64_t) * 8);
  cacheline[0] = data_0;
  cacheline[1] = data_1;
  cacheline[2] = data_2;
  cacheline[3] = data_3;
  cacheline[4] = data_4;
  cacheline[5] = data_5;
  cacheline[6] = data_6;
  cacheline[7] = data_7;
  paddr2cacheline[alignPaddr] = cacheline;
  // printf("{%ld} refill paddr=0x%x,data=0x%lx%lx%lx%lx%lx%lx%lx%lx\n",gtimer,alignPaddr,
  // cacheline[7],cacheline[6],cacheline[5],cacheline[4],cacheline[3],cacheline[2],cacheline[1],cacheline[0]);
}