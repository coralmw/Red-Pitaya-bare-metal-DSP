
//#include "xparameters.h"
//#include <stdio.h>
//#include "xil_mmu.h"
//#include "xil_printf.h"
//#include "xil_exception.h"
//#include "xscugic.h"
//#include "sleep.h"
#include <xpseudo_asm.h>
#include <stdlib.h>
#include <xil_types.h>
#include <xtime_l.h>
#include <xil_io.h>
#include <xil_cache_l.h>

#include "printf.h"

#include "regdefs.h"

#define MAXROWS 500

// TRANSLATED TO THE TOP OF MEMORY?
// 0x1ffffff4 in linux rather than
// 0x1fffff4  in bare metal?

#define RAM_ADDR 0x1e000000
/**************************** Type Definitions *******************************/
void sleep(int secs);

// communication flags, all in the OCM.

//remoteproc control
struct resource_table
{
  u32 ver;
  u32 num;
  u32 reserved[2];
  u32 offset[1];
} __packed;

enum fw_resource_type
{
  RSC_CARVEOUT = 0,
  RSC_DEVMEM = 1,
  RSC_TRACE = 2,
  RSC_VDEV = 3,
  RSC_MMU = 4,
  RSC_LAST = 5,
};

struct fw_rsc_carveout
{
  u32 type;
  u32 da;
  u32 pa;
  u32 len;
  u32 flags;
  u32 reserved;
  u8 name[32];
} __packed;

__attribute__ ((section (".rtable")))
const struct rproc_resource
{
  struct resource_table base;
  struct fw_rsc_carveout code_cout;
} ti_ipc_remoteproc_ResourceTable = {
  .base = { .ver = 1, .num = 1, .reserved = { 0, 0 },
    .offset = { offsetof(struct rproc_resource, code_cout) },
  },
  .code_cout = {
    .type = RSC_CARVEOUT, .da = RAM_ADDR, .pa = RAM_ADDR, .len = 1<<19, //was 1<<19
    .flags = 0, .reserved = 0, .name = "APP_CPU1",
  },
};

typedef struct actionTable {
	u64 nanos; // ulong max 4.9secs, ull max = 584 years
  XTime clocks;
	u32 pins;
  u32 a1;
  u32 a2;
} actionTable_t;

void led(int led, int state){
  // read modify write.
  Xil_Out32(LEDS, Xil_In32(LEDS) || state << led);
}

void pin(int pin, int state){
  Xil_Out32(PINS, Xil_In32(PINS) || state << pin);
}

// global to keep track of state.
int leds = 0;
actionTable_t actiontable[MAXROWS]; // static alloc to get in .bss
XTime now = 0;

void ledstatus(int led){
  leds |= 1 << led;
  Xil_Out32(LEDS, leds);
}

//HACK. OMG
void dumpcache(){
  int i;
  for (i = 0; i<0xFFFF; i += 4){
    (*(volatile unsigned long *)(0xFFFF0000+i)) = (*(volatile unsigned long *)(0xFFFF0000+i));
  }
}

void myPutchar(char c) {
	while(COMM_TX_FLAG) dumpcache();	//wait for cpu0 to consume previous value
  dumpcache();
	COMM_TX_DATA = (volatile unsigned long)c;
	dmb();
	COMM_TX_FLAG = 1;
  //dmb();
  dumpcache();
}

void tinyprintf_putc(void* p, char c){
  myPutchar(c);
}

void printm(char string[], int len){
  int i;
  for (i = 0; i < len; i++){
    myPutchar(string[i]);
  }
}


void getActionTable(actionTable_t* actiontable){
  int i;
  for (i = 0; i < COMM_RX_AT_ROWS; i++){
    ledstatus(2);
    printf("waiting for row %u\n", i);
    COMM_RX_AT_FLAG = 1; // bring high to signal for data
    while (COMM_RX_AT_FLAG == 1) dumpcache(); //wait for row to be ready
    printf("got row\n");
    ledstatus(3);
    dumpcache();
    actiontable[i].nanos = COMM_RX_AT_NANOS;
    actiontable[i].pins = COMM_RX_AT_PINS;
    printf("got at row time:0x%08x with pins 0x%08x\n", actiontable[i].nanos, actiontable[i].pins);
    ledstatus(4);
    dumpcache();
  }
  ledstatus(5);
}

void getFActionTable(actionTable_t* actiontable){
  int i;
  actionTable_t dummyATup;
  actionTable_t dummyATdown;
  dummyATup.pins = 0xFFFFFFFF;
  dummyATdown.pins = 0x00000000;
  unsigned long nanos = 0;

  for (i = 0; i < COMM_RX_AT_ROWS; i++){
    if (i % 2 == 0){
      actiontable[i] = dummyATup;
    } else {
      actiontable[i] = dummyATdown;
    }
    actiontable[i].nanos = nanos;
    nanos += 10000000;
  }
}

void findClockOffset(actionTable_t* actiontable){
  int i;
  for (i = 0; i < COMM_RX_AT_ROWS; i++){
    actiontable[i].clocks = actiontable[i].nanos * 100000000 / COUNTS_PER_SECOND;
  }
}


int fill(){
  int i;
  for (i = 0; i < OCM_SIZE; i = i+4){
    *(volatile u32 *)(COMM_BASE+i) = i;
    //*(volatile uint32_t *)(COMM_BASE+i+4) = 0x4F0A0000;
  }
}

int zero_ocm(){
  int i;
  for (i = 0; i < OCM_SIZE; i = i+4){
    *(u32 *)(COMM_BASE+i) = 0x00000000U;
  }
}

void sleep(int secs){
  int waitime = COUNTS_PER_SECOND * secs;
  XTime_GetTime(&now);
  XTime startTime = now;
  while ( now - startTime < waitime ) XTime_GetTime(&now);
}

void usleep(int usecs){
  int waitime = COUNTS_PER_SECOND/1000000 * usecs;
  XTime_GetTime(&now);
  XTime startTime = now;
  while ( now - startTime < waitime ) XTime_GetTime(&now);
}




// ALL HAIL COPYPASTA

extern u32 MMUTable; // defined in boot.S (i really, really hope) IT IS
#include <bsp/ps7_cortexa9_1/libsrc/standalone_v3_11_a/src/xil_cache.c>
// COPYPASTA out of xil_mmu.c - beacuse that does not want to compile
// into the lib for some reason?
void Xil_SetTlbAttributes(u32 addr, u32 attrib)
{
	u32 *ptr;
	u32 section;

	section = addr / 0x100000;
	ptr = &MMUTable + section;
	*ptr = (addr & 0xFFF00000) | attrib;

	Xil_DCacheFlush();

	mtcp(XREG_CP15_INVAL_UTLB_UNLOCKED, 0);
	/* Invalidate all branch predictors */
	mtcp(XREG_CP15_INVAL_BRANCH_ARRAY, 0);

	dsb(); /* ensure completion of the BP and TLB invalidation */
  isb(); /* synchronize context on this processor */
}

// COPYPASTA from https://github.com/Architech-Silica/Zynq-MMU-Caches-Control/
int adjust_mmu_mode(unsigned int start_of_1MB_address_region, unsigned int features)
{
	unsigned int mmu_attributes = 0;

	/* Declare the part of the page table value that gets written to the MMU Table, which is always fixed. */
	/* NS = b0, Bit 18 = b0, TEX(2) = b1, Bit 9 = b0, Domain = b1111, Bits(1:0) = b10 ... 0x41e2 */
	/* Fiddle with this at your own risk.  If you're not sure of what you're doing, it won't end well!! */
	const unsigned int fixed_values = 0x41e2;

	// Calculate the value that will be written to the MMU Page Table
	mmu_attributes = fixed_values + features;

	// Write the value to the TLB
	Xil_SetTlbAttributes(start_of_1MB_address_region, mmu_attributes);

	return (0);
}

// ALL REGRET COPYPASTA (cp end)

/* alloc action table in the unused area at top of ram */
/* random crashes - don't try this. */
// actionTable_t *actiontable = (actionTable_t *)RAM_ADDR;

int main()
{
  adjust_mmu_mode(0xFF000000, NON_CACHEABLE+AP_FULL_ACCESS);
  Xil_DCacheDisable();
  Xil_L2CacheDisable();
  Xil_L1ICacheDisable();
  Xil_L1ICacheDisable(); // turning off caches makes things worse?

  init_printf(NULL, tinyprintf_putc);
  printf("at_t %u", sizeof(actionTable_t));

  XTime_SetTime(now);
  Xil_Out32(PINS_MODE, 0xFFFFFFFF); // set pins to out (all)
  Xil_Out32(PINS, 0);
  Xil_Out32(LEDS, leds);
  sleep(1);
  ledstatus(1);
  // sleep(1);
  //zero_ocm(); // don't do this, stuff is set up before we are called
  printm("hello, World!\n", 15);
  printf("hello, w from printf\n");
  printf("waiting for %u rows\n", COMM_RX_AT_ROWS);
  // get the action table
  actionTable_t currRow;
  getActionTable(actiontable);
  printm("got action table\n", 17);
  findClockOffset(actiontable);
  printm("translated to nanos\n", 19);
  // init the timers. ns?
  XTime startTime = now;
  XTime diff;
  unsigned long long nsecs;
  XTime_GetTime(&startTime);
  ledstatus(6);
  // while (1){
  //   Xil_Out32(PINS, ~Xil_In32(PINS));
  //
  // }
  printm("running\n", 8);
  int rowN;
  for (rowN = 0; rowN < COMM_RX_AT_ROWS; rowN++){
    currRow = actiontable[rowN];
    while (diff < currRow.clocks) {
      XTime_GetTime(&now);
      diff = now - startTime;
    }
    Xil_Out32(PINS, currRow.pins);
  }
  ledstatus(7);
  printf("finished\n");
  Xil_Out32(PINS, 0);
  return 0;
}
