#!/usr/bin/python

SLOTS = [
    "1.wav",
    "2.wav",
    "3.wav",
    "4.wav",
    "5.wav",
    "6.wav",
    "7.wav",
    "8.wav",
    "9.wav",
    "star.wav",
    "0.wav",
    "sharp.wav"
]

if __name__ == '__main__':
    import sys
    sys.path.append("..")
    import slotfs
    
    slotfs.build_fs("dtmf.slotfs", SLOTS)
