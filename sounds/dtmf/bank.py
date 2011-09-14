#!/usr/bin/python

import struct
import wave

KEYCODE_TO_SLOT = [
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "star",
    "0",
    "sharp"
]

# Compute the directory
directory = []
offset = 1 # Start at sector 1 in the bank
for i, keycode in enumerate(KEYCODE_TO_SLOT):
    filename = "%s.wav" % keycode
    w = wave.open(filename, "r")
    nb_blocks = w.getnframes() / 512
    print "%s => offset %i, %i blocks" % (filename, offset, nb_blocks)
    w.close()
    
    directory.append(offset)
    directory.append(nb_blocks)
    offset += nb_blocks

# Write the bank content
with open("dtmf.bank", "w") as bank:
    print "Writing dtmf.bank..."

    # Write the header
    bank.write(struct.pack("BB", 0, 1)) # Version 0, Read-only
    bank.write(struct.pack("<BBLLL", 0, 0, 0, 0, 0)) # Padding

    # Write the directory
    bank.write(struct.pack("<" + "L" * len(directory), *directory)) # Little endian
    
    # Sector padding
    pos = bank.tell()
    padding = [0] * (512 - pos)
    bank.write(struct.pack("B" * len(padding), *padding))

    # Write the files
    for keycode in KEYCODE_TO_SLOT:
        filename = "%s.wav" % keycode
        w = wave.open(filename, "r")
        nb_blocks = w.getnframes() / 512
    
        for i in range(nb_blocks):
            data = w.readframes(512)
            bank.write(data)
        
        w.close()