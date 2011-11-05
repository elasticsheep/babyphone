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
    import partition
    
    partition.build_partition("animals.part", SLOTS)
