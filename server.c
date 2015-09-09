#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <byteswap.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>
#include "regdefs.h"

#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

void transferATrow(volatile uint8_t *mm, unsigned long long nanos, unsigned int pins){
  while ( *(volatile uint32_t *)(mm + 0x14) == 0); //wait for flag
  printf("data ready\n");
  *(volatile uint64_t *)(mm + 0x18) = 0xFFFFFFFFFFFFFFFF;
  *(volatile uint32_t *)(mm + 0x32)  = 0xFFFFFFFF;
  *(volatile uint32_t *)(mm + 0x14) = 1;
  *(volatile uint32_t *)(mm + 0x14) = 0;
}

int main()
{
    int fd;
    int up;
    uint32_t value=0;
    uint32_t flag=0;
    volatile uint8_t *mm;
    uint8_t str[MAX_STR];
    int rcnt;

    printf("opening /dev/mem\n");
    fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "open(/dev/mem) failed (%d)\n", errno);
        return 1;
    }

    printf("opening remoteproc\n");
    up = open("/sys/devices/soc0/1e000000.remoteproc/remoteproc0/up", O_RDWR|O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "open(/sys/devices/soc0/1e000000.remoteproc/remoteproc0/up) failed (%d)\nhave you loaded the remoteproc driver?\n", errno);
        return 1;
    }

    printf("mapping\n");
    mm = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0xFFFF0000);
    if (mm == MAP_FAILED) {
        fprintf(stderr, "mmap64(0x%x@0x%x) failed (%d)\n",
                PAGE_SIZE, (uint32_t)(COMM_BASE), errno);
        return 1;
    }

    // flow.
    // set rows
    // set flag to no data ready to read?
    // boot remote proc
    // it signals ready to read
    // load our data, loop

    // flag should be 'data request'
    // app_cpu1 brings it high to get the server to load data
    // server takes it low to signal data ready
    // server waits for it to go high again


    // set rows
    *(volatile uint32_t *)(mm + 0x10) = 1; // row no
    *(volatile uint32_t *)(mm + 0x14) = 0; // flag - no data ready
    dprintf(up, "1"); // bring the app up
    transferATrow(mm, 0, 0); // set the fist row

    munmap((void *)mm, PAGE_SIZE);
    close(fd);

    return 0;
}
