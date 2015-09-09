// OCM DEFS

#define COMM_BASE       0xFFFF9000
#define OCM_SIZE             0xFFFF
#define MAX_STR 128

#define COMM_VAL        (*(volatile unsigned long *)(0xFFFF9000))

#define COMM_TX_FLAG    (*(volatile unsigned long *)(0xFFFF9000))
#define COMM_TX_DATA    (*(volatile unsigned long *)(0xFFFF9004))
#define COMM_RX_FLAG    (*(volatile unsigned long *)(0xFFFF9008))
#define COMM_RX_DATA    (*(volatile unsigned long *)(0xFFFF900C))

#define COMM_RX_AT_ROWS    (*(volatile unsigned long *)(0xFFFF0010))
#define COMM_RX_AT_FLAG    (*(volatile unsigned long *)(0xFFFF0014))

#define COMM_RX_AT_NANOS       (*(volatile unsigned long long *)(0xFFFF0018))
#define COMM_RX_AT_PINS        (*(volatile u32 *)(0xffff0032))
#define COMM_RX_AT_A1        (*(volatile u32 *)(0xffff0036))
#define COMM_RX_AT_A2        (*(volatile u32 *)(0xffff003a))

// RED PITAYA FPGA DEFS - use Xil_Out32 (or Xil_Out16..)

#define PINS 0x40000000 + 0x18
#define PINS_MODE 0x40000000 + 0x10
#define LEDS 0x40000000 + 0x30

#define ASG  0x40200000
#define ASG_CONFIG ASG + 0x00
#define ASG_A_SCALE  ASG + 0x04
#define ASG_B_SCALE  ASG + 0x24


// MMU

#define NON_CACHEABLE 0x00 // TEX(1:0) = b00, C = b0, B = b0
#define WRITEBACK_WRITEALLOCATE 0x1004 // TEX(1:0) = b01, C = b0, B = b1
#define WRITETHROUGH_NO_WRITEALLOCATE 0x2008 // TEX(1:0) = b10, C = b1, B = b0
#define WRITEBACK_NO_WRITEALLOCATE 0x300C // TEX(1:0) = b11, C = b1, B = b1
#define NON_GLOBAL 0x20000 // nG = b1
#define EXECUTE_NEVER 0x10 // XN = b1
#define SHAREABLE 0x10000 // S = b1
#define AP_PERMISSIONFAULT 0x00 // AP(2) = b0, AP(1:0) = b00
#define AP_PRIVIEGED_ACCESS_ONLY 0x400 // AP(2) = b0, AP(1:0) = b01
#define AP_NO_USERMODE_WRITE 0x800 // AP(2) = b0, AP(1:0) = b10
#define AP_FULL_ACCESS 0xC00 // AP(2) = b0, AP(1:0) = b11
#define AP_PRIVILEGED_READ_ONLY 0x8800 // AP(2) = b1, AP(1:0) = b10
