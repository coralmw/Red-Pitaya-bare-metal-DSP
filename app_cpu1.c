
//#include "xparameters.h"
//#include <stdio.h>
//#include "xil_mmu.h"
//#include "xil_printf.h"
//#include "xil_exception.h"
//#include "xscugic.h"
//#include "sleep.h"
#include <xpseudo_asm.h>
#include <stdlib.h>
#include <xtime_l.h>
#include <xil_io.h>
#include "regdefs.h"
#undef COMM_RX_AT_ROWS
#define COMM_RX_AT_ROWS 500


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
	unsigned long long nanos; // ulong max 4.9secs, ull max = 584 years
	uint32_t pins;
  uint32_t a1;
  uint32_t a2;
} actionTable_t;

void led(int led, int state){
  // read modify write.
  Xil_Out32(LEDS, Xil_In32(LEDS) || state << led);
}

void pin(int pin, int state){
  Xil_Out32(PINS, Xil_In32(PINS) || state << pin);
}


// // From XAPP1078
// void myPutchar(char c) {
// 	while(Xil_In32(COMM_BASE+COMM_TX_FLAG_OFFSET) != 0);	//wait for cpu0 to consume previous value
// 	Xil_Out32(COMM_BASE+COMM_TX_DATA_OFFSET, c);
//   Xil_Out32(COMM_BASE+COMM_TX_FLAG_OFFSET, 0x00000001);
//   // int* value = (int *)(COMM_BASE+COMM_TX_DATA_OFFSET);
//   // int* flag = (int *)(COMM_BASE+COMM_TX_FLAG_OFFSET);
//   // *value = (uint32_t)c;
//   // *flag = 1;
// 	//dmb();
// }

// void myPutcharUART(char c){
//   int i;
//   Xil_Out32(PINS, 0);
//   sleepMicros(104);
//
//   for ( i = 8 ; i != 0 ; --i ) {
//     if (c & 0x01) Xil_Out32(PINS, ~0);
//     else          Xil_Out32(PINS, 0);
//     c >>= 1;
//     sleepMicros(104);//9600BPS
//   }
//   Xil_Out32(PINS, ~0);
//   sleepMicros(104);
// }

// void printm(char* str, int len){
//   leds = leds + 4;
//   Xil_Out32(LEDS, leds);
//   while( Xil_In32(COMM_BASE+COMM_TX_FLAG_OFFSET) ); //wait for flag to be cleared
//   leds = leds + 8;
//   Xil_Out32(LEDS, leds);
//   int i;
//   for (i = 0; i < len; i++){
//     *(uint32_t *)(0xFFFF0000+i) = str[i];
//   }
//   leds = leds + 16;
//   Xil_Out32(LEDS, leds);
// }

// global to keep track of state.
int leds = 0;
actionTable_t actiontable[COMM_RX_AT_ROWS];
XTime now = 0;

//HACK. OMG
void dumpcache(){
  int i;
  for (i = 0; i<0xFFFF; i += 4){
    (*(volatile unsigned long *)(0xFFFF0000+i)) = (*(volatile unsigned long *)(0xFFFF0000+i));
  }
}

void myPutchar(char c) {
	//while(COMM_TX_FLAG) dumpcache();	//wait for cpu0 to consume previous value
  leds |= 1 << 3;
  Xil_Out32(LEDS, leds);
	COMM_TX_DATA = (volatile unsigned long)c;
  leds |= 1 << 4;
  Xil_Out32(LEDS, leds);
	dmb();
  leds |= 1 << 5;
  Xil_Out32(LEDS, leds);
	COMM_TX_FLAG = 1;
  //dmb();
  dumpcache();
  sleep(1);
}

void outbyte(char c) {
  leds ^= 1 << 2;
  Xil_Out32(LEDS, leds);
  myPutchar(c);
}

void getActionTable(actionTable_t* actiontable){
  int i;
  for (i = 0; i < COMM_RX_AT_ROWS; i++){
    while (COMM_RX_AT_FLAG == 0); //wait for row to be ready
    actiontable[i] = COMM_RX_AT;
    COMM_RX_AT_FLAG = 0; // signal for more data
  }
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
    actiontable[i].nanos = i*10000000;
  }
}

void printm(char string[], int len){
  int i;
  for (i = 0; i < len; i++){
    outbyte(string[i]);
  }
}


int fill(){
  int i;
  for (i = 0; i < OCM_SIZE; i = i+4){
    *(volatile uint32_t *)(COMM_BASE+i) = i;
    //*(volatile uint32_t *)(COMM_BASE+i+4) = 0x4F0A0000;
  }
}

int zero_ocm(){
  int i;
  for (i = 0; i < OCM_SIZE; i = i+4){
    *(uint32_t *)(COMM_BASE+i) = 0x00000000U;
  }
}

// int main(){
//   Xil_Out32(LEDS, 0xFFFFFFFF);
//   while (1){
//     Xil_Out32(LEDS, 0xFFFFFFFF);
//     Xil_Out32(PINS, 0xFFFFFFFF);
//
//   }
// }

// void printh(){
//   char h[] = "hello, World!\n";
//   int i;
//   for (i = 0; i < 16; i++){
//     *(volatile char *)(COMM_BASE+i) = h[i];
//   }
//   for (i = 0; i < 16; i++){
//     *(volatile char *)(COMM_BASE+i+16) = h[i];
//   }
//   for (i = 0; i < 16; i++){
//     *(volatile char *)(COMM_BASE+i+32) = h[i];
//   }
// }

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


void ledstatus(int led){
  leds |= 1 << led;
  Xil_Out32(LEDS, leds);
}

#/* alloc action table in the unused area at top of ram */
/* random crashes - don't try this. */
// actionTable_t *actiontable = (actionTable_t *)RAM_ADDR;

int main()
{
  XTime_SetTime(now);
  Xil_Out32(PINS_MODE, 0xFFFFFFFF); // set pins to out (all)
  //Xil_Out32(PINS, 0xFFFFFFFF);
  Xil_Out32(LEDS, leds);
  sleep(1);
  ledstatus(1);
  // sleep(1);
  //zero_ocm();
  //printm("hello, World!\n", 15);
  // get the action table
  actionTable_t currRow;
  getFActionTable(actiontable);
  ledstatus(2);
  // init the timers. ns?
  XTime startTime = now;
  XTime diff;
  unsigned long long nsecs;
  ledstatus(3);
  XTime_GetTime(&startTime);
  ledstatus(4);
  int nsPerCount = 1000000000 / COUNTS_PER_SECOND;
  ledstatus(5);
  // while (1){
  //   Xil_Out32(PINS, ~Xil_In32(PINS));
  //
  // }
  int rowN;
  for (rowN = 0; rowN < COMM_RX_AT_ROWS; rowN++){
    currRow = actiontable[rowN];
    //diff = now - startTime;
    //nsecs = (diff * 10000000) / COUNTS_PER_SECOND;
    //while (nsecs < currRow.nanos) XTime_GetTime(&now);
    usleep(currRow.nanos / 1000);
    Xil_Out32(PINS, currRow.pins);
  }
  ledstatus(7);
  return 0;
}

  //
  // int freq = 2;
  // volatile int i = 0;
  // while(1)
  // {
  //   XTime_GetTime(&t);
  //   if (t - startTime >= waitTime){
  //     Xil_Out32(LEDS, leds);
  //     leds ^= 1 << 7;
  //     startTime = t;
  //   }
  // }
//   return 0;
// }
