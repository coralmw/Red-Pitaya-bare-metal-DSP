from __future__ import print_function

'''
from server import app_cpu1
import time
from mmap import mmap
import struct

app = app_cpu1()

# struct.unpack('i', OCM[16:20]) len
# struct.unpack('QQLLL', OCM[24:52]) row
# struct.unpack('i', OCM[20: 24])[0] dataready flag

devmem = open('/dev/mem', 'r+b')
OCM = mmap(devmem.fileno(), 256, offset=0xFFFF9000)

app.abort()

## make sure len is in OCM before start
OCM[16:20] = struct.pack('L', 2) # len
OCM[24:52] = struct.pack('QQLLL', # data
                              0,
                              0,
                              0xFFFFFFFF,
                              0,
                              0)


app.start()

while struct.unpack('i', OCM[20: 24])[0] != 0: # wait for read
    pass

OCM[24:52] = struct.pack('QQLLL', # data
                              100,
                              0,
                              0,
                              0,
                              0)
OCM[20: 24] = struct.pack('i', 1) # ack
## should be running.

while struct.unpack('i', OCM[20: 24])[0] != 0:
    pass

OCM[24:52] = struct.pack('QQLLL', # data
                              100000000,
                              0,
                              0xFFFFFFFF,
                              0,
                              0)
OCM[20: 24] = struct.pack('i', 1) # ack

from server import app_cpu1
import time
from mmap import mmap
import struct

app = app_cpu1()

#16->24 +8
# struct.unpack('i', OCM[16:20]) len
# struct.unpack('QQLLL', OCM[24:52]) row
# struct.unpack('QQLLL', OCM[24:52])
# struct.unpack('i', OCM[20: 24])[0] dataready flag

devmem = open('/dev/mem', 'r+b')
OCM = mmap(devmem.fileno(), 256, offset=0xFFFF9000)
def dumpcache():
    OCM[:] = OCM[:]

#HACK
def go(rows):
    try:
        app.abort()
    except ValueError:
        pass
    OCM[16:20] = struct.pack('i', rows) # set len
    OCM[20: 24] = struct.pack('i', 1) # data available
    dumpcache()
    print("row 0")
    app.start()
    val = 0xFFFFFFFF
    for row in range(1, rows+1):
        print("row:", row)
        while struct.unpack('i', OCM[20: 24])[0] != 0: # wait for data to be read
            dumpcache()
        #OCM[24:32] = struct.pack('Q', val)
        #
        # should be this
        #
        OCM[0x18:0x18+8] = struct.pack('Q', row) # this is ending up the pin no
        OCM[0x32:0x32+4] = struct.pack('L', val)
        #//OCM[24:52] = struct.pack('QQLLL', row,0,val,0,0)
        #
        #
        OCM[20: 24] = struct.pack('i', 1)
        dumpcache()

while True:
    if struct.unpack('i', OCM[20: 24])[0] != 1:
        print('wtf')
        OCM[20: 24] = struct.pack('i', 1) # there is in fact some buffer crap going wrong.
app.start()


when flag == 1: data is ready to be read by app_cpu1
when flag == 0: data has been consumed by app_cpu1, ready to write

'''

#import Pyro4
import subprocess
import time
import os
import struct
from mmap import mmap

BASE            = 0xFFFF9000
COMM_RX_AT_ROWS = 0x10
COMM_RX_AT_FLAG = 0x14
COMM_RX_AT      = 0x18


def bin(s):
    ''' Returns the set bits in a positive int as a str.'''
    return str(s) if s<=1 else bin(s>>1) + str(s&1)


class app_cpu1(object):

    def __init__(self):
        self._devmem = open('/dev/mem', 'r+b')
        self.OCM = mmap(self._devmem.fileno(), 36, offset=0xFFFF9000)
        self.up = None
        self.remoteProcLoaded = False
        self.running = False

    def load(self, times, digitals, analogA, analogB):
        # unsigned long - no rows
        self.OCM[16:20] = struct.pack('L', len(times)) # write the no of actiontable rows
        for time, DigitalBitMask, analogA, analogB in zip(times, digitals,
                                                          analogA, analogB):
            nanos = time * 1e9
            while struct.unpack('i', self.OCM[10:14]) != 0:
                pass # wait for the value to be consumed
            # nanos, clocks, pins, a1, a2
            self.OCM[24,52] = struct.pack('QQLLL',
                                          self.OCM,
                                          COMM_RX_AT,
                                          time,
                                          DigitalBitMask,
                                          analogA,
                                          analogB)
            self.OCM[COMM_RX_AT_FLAG, COMM_RX_AT_FLAG+4] = struct.pack('i', 1)

    def _load_module(self):
        if not self.remoteProcLoaded:
            subprocess.call('modprobe zynq_remoteproc', shell=True)
            self.remoteProcLoaded = True

    def _unload_module(self):
        subprocess.call('rmmod -f zynq_remoteproc', shell=True)
        self.remoteProcLoaded = False

    def abort(self):
        self.up.write('0')
        self.up.flush()
        self.up.close()
        self.running = True
        self._unload_module()

    def start(self):
        self._load_module()
        self.up = open('/sys/devices/soc0/1e000000.remoteproc/remoteproc0/up', 'w+')
        self.up.write('1')
        self.up.flush()
        self.running = True

    def stop(self):
        self.abort()



class rpServer(object):

    def __init__(self):
        self.pid = None
        self.name = None
        self.times = []
        self.digitals = []
        self.analogA = []
        self.analogB = []

        self.appcpu1 = app_cpu1()



    ## In order to just toggle a pin, we create a short actionTable
    # turning the pin on and then off after 5 secs.
    # if we just turned it on, the end of the DSP program will reset everything
    # too 0 instantly.
    def high(self, name):
        pass

    # raw version does not do pin name lookups.
    def analog_raw(self, pin, value):
        pass

    def high_raw(self, pin):
        pass

    # The dsp has a handler for SIGINT that cleans up
    def Abort(self):
        # kill the server process
        self.appcpu1.abort()

    def MoveAbsoluteADU(self, aline, aduPos):
        # probably just use the python lib
        # volts to ADU's for the DSP card: int(pos * 6553.6))
        pass

    def arcl(self, cameraMask, lightTimePairs):
        # wha?
        pass

    def profileSet(self, profileStr, digitals, *analogs):
        # This is downloading the action table
        # digitals is numpy.zeros((len(times), 2), dtype = numpy.uint32),
        # starting at 0 -> [times for digital signal changes, digital lines]
        # analogs is a list of analog lines and the values to put on them at each time
        self.times, self.digitals = zip(*digitals)
        self.analogA = analogs[0]
        self.analogB = analogs[1]

    def DownloadProfile(self): # This is saving the action table
        self.appcpu1.load(self.times, self.digitals, self.analogA, self.analogB)

    def InitProfile(self, numReps):
        self.times, self.digitals, self.analogA, self.analogB = [], [], [], []

    def trigCollect(self):
        self.appcpu1.start()

    def ReadPosition(self):
        pass

    def WriteDigital(self, level):
        self.red_pitaya.hk.led = level

if __name__ == '__main__':
    dsp = rpServer()

    print("providing dsp.d() as [pyroDSP] on port 7766")
    print("Started program at",time.strftime("%A, %B %d, %I:%M %p"))

    import random
    daemon = Pyro4.Daemon(port = random.randint(2000, 10000), host = '192.168.1.100')
    Pyro4.Daemon.serveSimple({dsp: 'pyroDSP'},
            daemon = daemon, ns = False, verbose = True)
