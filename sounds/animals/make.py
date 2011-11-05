#!/usr/bin/python

SLOTS = [
    "chimp.wav",
    "cow.wav",
    "dog.wav",
    "dolphin.wav",
    "ducks.wav",
    "elephant.wav",
    "frog.wav",
    "horse.wav",
    "lamb.wav",
    "pig.wav",
    "rooster.wav",
    "wolf.wav"
]

if __name__ == '__main__':
    import sys
    sys.path.append("..")
    import slotfs
    
    slotfs.build_fs("animals.slotfs", SLOTS, sampling_rate=16000)
