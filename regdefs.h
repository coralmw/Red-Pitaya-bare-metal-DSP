// OCM DEFS

#define COMM_BASE       0xFFFF9000
#define OCM_SIZE             0xFFFF
#define MAX_STR 128

#define COMM_VAL        (*(volatile unsigned long *)(0xFFFF9000))

#define COMM_TX_FLAG    (*(volatile unsigned long *)(0xFFFF9000))
#define COMM_TX_DATA    (*(volatile unsigned long *)(0xFFFF9004))
#define COMM_RX_FLAG    (*(volatile unsigned long *)(0xFFFF9008))
#define COMM_RX_DATA    (*(volatile unsigned long *)(0xFFFF900C))

#define COMM_RX_AT_ROWS    (*(volatile unsigned long *)(0xFFFF9010))
#define COMM_RX_AT_FLAG    (*(volatile unsigned long *)(0xFFFF9014))
#define COMM_RX_AT         (*(volatile actionTable_t *)(0xFFFF9018))


// RED PITAYA FPGA DEFS - use Xil_Out32 (or Xil_Out16..)

#define PINS 0x40000000 + 0x18
#define PINS_MODE 0x40000000 + 0x10
#define LEDS 0x40000000 + 0x30

#define ASG  0x40200000
#define ASG_CONFIG ASG + 0x00
#define ASG_A_SCALE  ASG + 0x04
#define ASG_B_SCALE  ASG + 0x24
