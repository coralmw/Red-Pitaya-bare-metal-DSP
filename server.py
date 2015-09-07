from __future__ import print_function

import Pyro4
import subprocess
import time
import os
import uuid
from PyRedPitaya.board import RedPitaya

COMM_RX_AT_ROWS = 0xFFFF9010
COMM_RX_AT_FLAG = 0xFFFF9014
COMM_RX_AT      = 0xFFFF9018


def bin(s):
    ''' Returns the set bits in a positive int as a str.'''
    return str(s) if s<=1 else bin(s>>1) + str(s&1)

class rpServer(object):

    def __init__(self):
        self.timeLineValues = []
        self.profileFile = None
        self.red_pitaya = RedPitaya()
        self.pid = None
        self.name = None
        self.times = []
        self.digitals = []
        self.analogA = []
        self.analogB = []

        self.OCM = open('/dev/mem', 'r+')

    ## In order to just toggle a pin, we create a short actionTable
    # turning the pin on and then off after 5 secs.
    # if we just turned it on, the end of the DSP program will reset everything
    # too 0 instantly.
    def high(self, name):
        with open('tmp', 'w') as f:
            f.write('0 0 {} 1\n'.format(self.RPPINS[name]))
            f.write('5 0 {} 0'.format(self.RPPINS[name]))
        cmd = ['./dsp', os.path.join(os.getcwd(), 'tmp')]
        subprocess.call(cmd)

    # raw version does not do pin name lookups.
    def analog_raw(self, pin, value):
        with open('tmp', 'w') as f:
            f.write('0 0 {} {}\n'.format(-1*pin, value))
            f.write('5 0 {} 0'.format(-1*pin))
        cmd = ['./dsp', os.path.join(os.getcwd(), 'tmp')]
        subprocess.call(cmd)

    def high_raw(self, pin):
        with open('tmp', 'w') as f:
            f.write('0 0 {} 1\n'.format(pin))
            f.write('5 0 {} 0'.format(pin))
        cmd = ['./dsp', os.path.join(os.getcwd(), 'tmp')]
        subprocess.call(cmd)

    # The dsp has a handler for SIGINT that cleans up
    def Abort(self):
        # kill the server process
        subprocess.call(['kill', '-9', str(self.pid)])

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
        # unsigned long - no rows
        struct.pack_into('L', self.OCM, COMM_RX_AT_ROWS, len(times)) # write the no of actiontable rows
        self.OCM.flush()
        for time, DigitalBitMask, analogA, analogB in zip(self.times, self.digitals,
                                                          self.analogA, self.analogB):
            nanos = time * 1e9
            while struct.unpack('i', self.OCM, COMM_RX_AT_FLAG) != 0:
                pass # wait for the value to be consumed
            # nanos, clocks, pins, a1, a2
            struct.pack_into('QQLLL', self.OCM, COMM_RX_AT, time,
                                                            DigitalBitMask,
                                                            analogA,
                                                            analogB)
            struct.pack_into('i', self.OCM, COMM_RX_AT_FLAG, 1)


    def InitProfile(self, numReps):
        self.name = 'profile'
        self.timeLineValues = []
        self.profileFile = open(self.name, 'w')

    def trigCollect(self):
        cmd = ['./dsp', os.path.join(os.getcwd(), self.name)]
        print('calling', cmd)
        self.pid = subprocess.Popen(cmd).pid

    def ReadPosition(self):
        pass

    def WriteDigital(self, level):
        self.red_pitaya.hk.led = level


dsp = rpServer()

print("providing dsp.d() as [pyroDSP] on port 7766")
print("Started program at",time.strftime("%A, %B %d, %I:%M %p"))

import random
daemon = Pyro4.Daemon(port = random.randint(2000, 10000), host = '192.168.1.100')
Pyro4.Daemon.serveSimple({dsp: 'pyroDSP'},
        daemon = daemon, ns = False, verbose = True)
