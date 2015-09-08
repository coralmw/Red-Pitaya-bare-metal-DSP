from __future__ import print_function

import struct
from mmap import mmap

UART_BASE = 0xFFFF9000
UART_SIZE = 0xFF

TX_FLAG_OFFSET  =  0x00
TX_DATA_OFFSET  =  0x04
RX_FLAG_OFFSET  =  0x08
RX_DATA_OFFSET  =  0x0C


with open('/dev/mem', 'r+b') as f:
    mem = mmap(f.fileno(), UART_SIZE, offset=UART_BASE)

while True:
    if struct.unpack('i', mem[TX_FLAG_OFFSET:TX_FLAG_OFFSET+4])[0] == 1:
        print(mem[TX_DATA_OFFSET:TX_DATA_OFFSET+1], end='')
        mem[TX_FLAG_OFFSET:TX_FLAG_OFFSET+4] = struct.pack('i', 0)
