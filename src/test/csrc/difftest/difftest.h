/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __DIFFTEST_H__
#define __DIFFTEST_H__

#include "common.h"
#include "refproxy.h"
#include "icache_sim.h"

#define DIFFTEST_CORE_NUMBER  NUM_CORES

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };
enum { REF_TO_DUT, DUT_TO_REF };
enum { REF_TO_DIFFTEST, DUT_TO_DIFFTEST };
enum { ICACHEID, DCACHEID, PAGECACHEID };
enum { ITLBID, LDTLBID, STTLBID};
// DIFFTEST_TO_DUT ~ REF_TO_DUT ~ REF_TO_DIFFTEST
// DIFFTEST_TO_REF ~ DUT_TO_REF ~ DUT_TO_DIFFTEST
#define CP printf("%s: %d\n", __FILE__, __LINE__);fflush( stdout );

#define DEBUG_MEM_REGION(v, f) (f <= (DEBUG_MEM_BASE + 0x1000) && \
        f >= DEBUG_MEM_BASE && \
        v)
#define IS_LOAD_STORE(instr) (((instr & 0x7f) == 0x03) || ((instr & 0x7f) == 0x23))
#define IS_TRIGGERCSR(instr) (((instr & 0x7f) == 0x73) && ((instr & (0xff0 << 20)) == (0x7a0 << 20)))
#define IS_DEBUGCSR(instr) (((instr & 0x7f) == 0x73) && ((instr & (0xffe << 20)) == (0x7b0 << 20))) // 7b0 and 7b1
#ifdef DEBUG_MODE_DIFF
#define DEBUG_MODE_SKIP(v, f, instr) DEBUG_MEM_REGION(v, f) && \
(IS_LOAD_STORE(instr) || IS_TRIGGERCSR(instr))
#else
#define DEBUG_MODE_SKIP(v, f, instr) false
#endif

// Difftest structures
// trap events: self-defined traps
typedef struct {
  uint8_t  valid = 0;
  uint8_t  code = 0;
  uint64_t pc = 0;
  uint64_t cycleCnt = 0;
  uint64_t instrCnt = 0;
  uint8_t  hasWFI = 0;
} trap_event_t;

// architectural events: interrupts and exceptions
// whose priority should be higher than normal commits
typedef struct {
  uint32_t interrupt = 0;
  uint32_t exception = 0;
  uint64_t exceptionPC = 0;
  uint32_t exceptionInst = 0;
} arch_event_t;

typedef struct {
  uint8_t  valid = 0;
  uint64_t pc;
  uint32_t inst;
  uint8_t  skip;
  uint8_t  isRVC;
  uint8_t  fused;
  uint8_t  rfwen;
  uint8_t  fpwen;
  uint32_t wpdest;
  uint8_t  wdest;
  uint8_t  lqidx;
  uint8_t  sqidx;
  uint16_t robidx;
  uint8_t  isLoad;
  uint8_t  isStore;
} instr_commit_t;

typedef struct {
  uint64_t gpr[32];
  uint64_t fpr[32];
} arch_reg_state_t;

typedef struct __attribute__((packed)) {
  uint64_t this_pc;
  uint64_t mstatus;
  uint64_t mcause;
  uint64_t mepc;
  uint64_t sstatus;
  uint64_t scause;
  uint64_t sepc;
  uint64_t satp;
  uint64_t mip;
  uint64_t mie;
  uint64_t mscratch;
  uint64_t sscratch;
  uint64_t mideleg;
  uint64_t medeleg;
  uint64_t mtval;
  uint64_t stval;
  uint64_t mtvec;
  uint64_t stvec;
  uint64_t priviledgeMode;
} arch_csr_state_t;

typedef struct __attribute__((packed)) {
  uint64_t debugMode;
  uint64_t dcsr;
  uint64_t dpc;
  uint64_t dscratch0;
  uint64_t dscratch1;
} debug_mode_t;

#ifndef DEBUG_MODE_DIFF
const int DIFFTEST_NR_REG = (sizeof(arch_reg_state_t) + sizeof(arch_csr_state_t)) / sizeof(uint64_t);
#else
const int DIFFTEST_NR_REG = (sizeof(arch_reg_state_t) + sizeof(arch_csr_state_t) + sizeof(debug_mode_t)) / sizeof(uint64_t);
#endif

typedef struct {
  uint8_t  resp = 0;
  uint64_t addr;
  uint8_t  data[64];
  uint64_t mask;
} sbuffer_state_t;

typedef struct {
  uint8_t  valid = 0;
  uint64_t addr;
  uint64_t data;
  uint8_t  mask;
} store_event_t;

typedef struct {
  uint8_t  valid = 0;
  uint64_t paddr;
  uint8_t  fuType;
  uint8_t  opType;
} load_event_t;

typedef struct {
  uint8_t  resp = 0;
  uint64_t addr;
  uint64_t data;
  uint8_t  mask;
  uint8_t  fuop;
  uint64_t out;
} atomic_event_t;

typedef struct {
  uint8_t  valid = 0;
  uint64_t satp;
  uint64_t vpn;
  uint64_t ppn;
} l1tlb_event_t;

typedef struct {
  uint8_t  valid = 0;
  uint64_t satp;
  uint64_t vpn;
  uint64_t ppn;
  uint8_t perm;
  uint8_t level;
  uint8_t pf;
} l2tlb_event_t;

typedef struct {
  uint8_t  valid = 0;
  uint64_t addr;
  uint64_t data[8];
} refill_event_t;

typedef struct {
  uint8_t valid = 0;
  uint8_t success;
} lr_sc_evevnt_t;

typedef struct {
  uint8_t  valid = 0;
  uint8_t  branch = 0;
  uint8_t  may_replay = 0;
  uint64_t pc;
  uint64_t checkpoint_id;
} run_ahead_event_t;

typedef struct {
  uint8_t  valid = 0;
  uint8_t  branch = 0;
  uint64_t pc;
} run_ahead_commit_event_t;

typedef struct {
  uint8_t  valid = 0;
  uint64_t pc;
  uint64_t target_pc;
  uint64_t checkpoint_id;
} run_ahead_redirect_event_t;

typedef struct {
  uint8_t  valid = 0;
  uint8_t  is_load;
  uint8_t  need_wait;
  uint64_t pc;
  uint64_t oracle_vaddr;
} run_ahead_memdep_pred_t;

typedef struct {
  uint64_t gpr[DIFFTEST_MAX_PRF_SIZE];
  uint64_t fpr[DIFFTEST_MAX_PRF_SIZE];
} physical_reg_state_t;

typedef struct {
  trap_event_t      trap;
  arch_event_t      event;
  instr_commit_t    commit[DIFFTEST_COMMIT_WIDTH];
  arch_reg_state_t  regs;
  arch_csr_state_t  csr;
  debug_mode_t      dmregs;
  sbuffer_state_t   sbuffer[DIFFTEST_SBUFFER_RESP_WIDTH];
  store_event_t     store[DIFFTEST_STORE_WIDTH];
  load_event_t      load[DIFFTEST_COMMIT_WIDTH];
  atomic_event_t    atomic;
  l1tlb_event_t     itlb[DIFFTEST_ITLB_WIDTH];
  l1tlb_event_t     ldtlb[DIFFTEST_LDTLB_WIDTH];
  l1tlb_event_t     sttlb[DIFFTEST_STTLB_WIDTH];
  l2tlb_event_t     l2tlb[DIFFTEST_PTW_WIDTH];
  refill_event_t    d_refill;
  refill_event_t    i_refill;
  refill_event_t    ptw_refill;
  lr_sc_evevnt_t    lrsc;
  run_ahead_event_t runahead[DIFFTEST_RUNAHEAD_WIDTH];
  run_ahead_commit_event_t runahead_commit[DIFFTEST_RUNAHEAD_WIDTH];
  run_ahead_redirect_event_t runahead_redirect;
  run_ahead_memdep_pred_t runahead_memdep_pred[DIFFTEST_RUNAHEAD_WIDTH];
  physical_reg_state_t pregs;
  icache_sim_ipf_refill_event_t icache_sim_ipf_refill;
  icache_sim_read_event_t icache_sim_read;
  icache_sim_refill_event_t icache_sim_refill;
  icache_sim_req_event_t icache_sim_req;
  icache_sim_resp_event_t icache_sim_resp;
  icache_sim_ideal_refill_t icache_sim_ideal_refill;
  icache_sim_ideal_read_t icache_sim_ideal_read;
} difftest_core_state_t;

enum retire_inst_type {
  RET_NORMAL=0,
  RET_INT,
  RET_EXC
};

enum retire_mem_type {
  RET_OTHER=0,
  RET_LOAD,
  RET_STORE
};

class DiffState {
public:
  DiffState();
  void record_group(uint64_t pc, uint64_t count) {
    retire_group_pc_queue [retire_group_pointer] = pc;
    retire_group_cnt_queue[retire_group_pointer] = count;
    retire_group_pointer = (retire_group_pointer + 1) % DEBUG_GROUP_TRACE_SIZE;
  };
  void record_inst(uint64_t pc, uint32_t inst, uint8_t en, uint8_t dest, uint64_t data, uint8_t lqidx, uint8_t sqidx, uint16_t robidx, uint8_t isLoad, uint8_t isStore, bool skip) {
    retire_inst_pc_queue   [retire_inst_pointer] = pc;
    retire_inst_inst_queue [retire_inst_pointer] = inst;
    retire_inst_wen_queue  [retire_inst_pointer] = en;
    retire_inst_wdst_queue [retire_inst_pointer] = dest;
    retire_inst_wdata_queue[retire_inst_pointer] = data;
    retire_inst_skip_queue[retire_inst_pointer] = skip;
    retire_inst_type_queue[retire_inst_pointer] = RET_NORMAL;
    retire_inst_robidx_queue[retire_inst_pointer] = robidx;
    retire_inst_lqidx_queue[retire_inst_pointer] = lqidx;
    retire_inst_sqidx_queue[retire_inst_pointer] = sqidx;
    retire_inst_mem_type_queue[retire_inst_pointer] = isLoad ? RET_LOAD : (isStore ? RET_STORE : RET_OTHER);
    retire_inst_pointer = (retire_inst_pointer + 1) % DEBUG_INST_TRACE_SIZE;
  };
  void record_abnormal_inst(uint64_t pc, uint32_t inst, uint32_t abnormal_type, uint64_t cause) {
    retire_inst_pc_queue   [retire_inst_pointer] = pc;
    retire_inst_inst_queue [retire_inst_pointer] = inst;
    retire_inst_wdata_queue[retire_inst_pointer] = cause; // write cause to data queue to save space
    retire_inst_type_queue[retire_inst_pointer] = abnormal_type;
    retire_inst_pointer = (retire_inst_pointer + 1) % DEBUG_INST_TRACE_SIZE;
  };
  void display(int coreid);

private:
  int retire_group_pointer = 0;
  uint64_t retire_group_pc_queue[DEBUG_GROUP_TRACE_SIZE] = {0};
  uint32_t retire_group_cnt_queue[DEBUG_GROUP_TRACE_SIZE] = {0};

  int retire_inst_pointer = 0;
  uint64_t retire_inst_pc_queue[DEBUG_INST_TRACE_SIZE] = {0};
  uint32_t retire_inst_inst_queue[DEBUG_INST_TRACE_SIZE] = {0};
  uint64_t retire_inst_wen_queue[DEBUG_INST_TRACE_SIZE] = {0};
  uint32_t retire_inst_wdst_queue[DEBUG_INST_TRACE_SIZE] = {0};
  uint64_t retire_inst_wdata_queue[DEBUG_INST_TRACE_SIZE] = {0};
  uint32_t retire_inst_type_queue[DEBUG_INST_TRACE_SIZE] = {0};
  uint16_t retire_inst_robidx_queue[DEBUG_INST_TRACE_SIZE] = {0};
  uint8_t retire_inst_lqidx_queue[DEBUG_INST_TRACE_SIZE] = {0};
  uint8_t retire_inst_sqidx_queue[DEBUG_INST_TRACE_SIZE] = {0};
  enum retire_mem_type retire_inst_mem_type_queue[DEBUG_INST_TRACE_SIZE] = {RET_OTHER};
  bool retire_inst_skip_queue[DEBUG_INST_TRACE_SIZE] = {0};
};

class Difftest {
public:
  // Difftest public APIs for testbench
  // Its backend should be cross-platform (NEMU, Spike, ...)
  // Initialize difftest environments
  Difftest(int coreid);
  DIFF_PROXY *proxy = NULL;
  uint32_t num_commit = 0; // # of commits if made progress
  bool has_commit = false;
  // Trigger a difftest checking procdure
  virtual int step();
  void update_nemuproxy(int, size_t);
  inline bool get_trap_valid() {
    return dut.trap.valid;
  }
  inline int get_trap_code() {
    return dut.trap.code;
  }
  void display();

  // Difftest public APIs for dut: called from DPI-C functions (or testbench)
  // These functions generally do nothing but copy the information to core_state.
  inline trap_event_t *get_trap_event() {
    return &(dut.trap);
  }
  inline arch_event_t *get_arch_event() {
    return &(dut.event);
  }
  inline instr_commit_t *get_instr_commit(uint8_t index) {
    return &(dut.commit[index]);
  }
  inline arch_csr_state_t *get_csr_state() {
    return &(dut.csr);
  }
  inline arch_reg_state_t *get_arch_reg_state() {
    return &(dut.regs);
  }
  inline sbuffer_state_t *get_sbuffer_state(uint8_t index) {
    return &(dut.sbuffer[index]);
  }
  inline store_event_t *get_store_event(uint8_t index) {
    return &(dut.store[index]);
  }
  inline load_event_t *get_load_event(uint8_t index) {
    return &(dut.load[index]);
  }
  inline atomic_event_t *get_atomic_event() {
    return &(dut.atomic);
  }
  inline l1tlb_event_t *get_l1tlb_event(uint8_t l1tlbid, uint8_t index) {
    if (l1tlbid == STTLBID) return &(dut.sttlb[index]);
    else if(l1tlbid == LDTLBID) return &(dut.ldtlb[index]);
    return &(dut.itlb[index]);
  }
  inline l2tlb_event_t *get_l2tlb_event(uint8_t index) {
    return &(dut.l2tlb[index]);
  }
  inline refill_event_t *get_refill_event(uint8_t cacheid) {
    if (cacheid == PAGECACHEID) return &(dut.ptw_refill);
    else if(cacheid == DCACHEID) return &(dut.d_refill);
    return &(dut.i_refill);
  }
  inline lr_sc_evevnt_t *get_lr_sc_event() {
    return &(dut.lrsc);
  }
  inline run_ahead_event_t *get_runahead_event(uint8_t index) {
    return &(dut.runahead[index]);
  }
  inline run_ahead_commit_event_t *get_runahead_commit_event(uint8_t index) {
    return &(dut.runahead_commit[index]);
  }
  inline run_ahead_redirect_event_t *get_runahead_redirect_event() {
    return &(dut.runahead_redirect);
  }
  inline run_ahead_memdep_pred_t *get_runahead_memdep_pred(uint8_t index) {
    return &(dut.runahead_memdep_pred[index]);
  }
  inline difftest_core_state_t *get_dut() {
    return &dut;
  }
  inline difftest_core_state_t *get_ref() {
    return &ref;
  }
  inline physical_reg_state_t *get_physical_reg_state() {
    return &(dut.pregs);
  }
  inline debug_mode_t *get_debug_state() {
    return &(dut.dmregs);
  }

  inline icache_sim_ipf_refill_event_t* get_icache_sim_ipf_refill() {
    return &(dut.icache_sim_ipf_refill);
  }

  inline icache_sim_read_event_t* get_icache_sim_read() {
    return &(dut.icache_sim_read);
  }

  inline icache_sim_refill_event_t* get_icache_sim_refill() {
    return &(dut.icache_sim_refill);
  }

  inline icache_sim_req_event_t* get_icache_sim_req() {
    return &(dut.icache_sim_req);
  }

  inline icache_sim_resp_event_t* get_icache_sim_resp() {
    return &(dut.icache_sim_resp);
  }

  inline icache_sim_ideal_refill_t* get_icache_sim_ideal_refill() {
    return &(dut.icache_sim_ideal_refill);
  }

  inline icache_sim_ideal_read_t* get_icache_sim_ideal_read() {
    return &(dut.icache_sim_ideal_read);
  }

#ifdef DEBUG_REFILL
  void save_track_instr(uint64_t instr) {
    track_instr = instr;
  }
#endif

#ifdef DEBUG_MODE_DIFF
  void debug_mode_copy(uint64_t addr, size_t size, uint32_t data) {
    proxy->debug_mem_sync(addr, &data, size);
  }
#endif

protected:
  const uint64_t firstCommit_limit = 15000;
  const uint64_t stuck_limit = 15000;

  int id;
  difftest_core_state_t dut;
  difftest_core_state_t ref;
  uint64_t *ref_regs_ptr = (uint64_t*)&ref.regs;
  uint64_t *dut_regs_ptr = (uint64_t*)&dut.regs;

  bool progress = false;
  uint64_t ticks = 0;
  uint64_t last_commit = 0;

  uint64_t nemu_this_pc;
  DiffState *state = NULL;
#ifdef DEBUG_REFILL
  uint64_t track_instr = 0;
#endif

  void update_last_commit() { last_commit = ticks; }
  int check_timeout();
  void do_first_instr_commit();
  void do_interrupt();
  void do_exception();
  void do_instr_commit(int index);
  int do_store_check();
  int do_refill_check(int cacheid);
  int do_irefill_check();
  int do_drefill_check();
  int do_ptwrefill_check();
  int do_l1tlb_check(int l1tlbid);
  int do_itlb_check();
  int do_ldtlb_check();
  int do_sttlb_check();
  int do_l2tlb_check();
  int do_golden_memory_update();
  // inline uint64_t *ref_regs_ptr() { return (uint64_t*)&ref.regs; }
  // inline uint64_t *dut_regs_ptr() { return (uint64_t*)&dut.regs; }
  inline uint64_t get_commit_data(int i) {
    uint64_t result = (dut.commit[i].fpwen) ? dut.pregs.fpr[dut.commit[i].wpdest] : dut.pregs.gpr[dut.commit[i].wpdest];
    return result;
  }
  inline bool has_wfi() {
    return dut.trap.hasWFI;
  }

  void raise_trap(int trapCode);
  void clear_step();
};

extern Difftest **difftest;
int difftest_init();
int difftest_step();
int difftest_state();
int init_nemuproxy(size_t);

#endif
