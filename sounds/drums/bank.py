#!/usr/bin/python

import struct
import wave

FILE_TO_SLOT = [
    "floortom",
    "kick",
    "closedhat",
]

bank_name = "drums.bank"
sampling_rate = 16000

# Compute the directory
directory = []
offset = 1 # Start at sector 1 in the bank
for i, keycode in enumerate(FILE_TO_SLOT):
    filename = "%s.wav" % keycode
    w = wave.open(filename, "r")
    nb_blocks = w.getnframes() / 512
    print "%s => offset %i, %i blocks" % (filename, offset, nb_blocks)
    w.close()
    
    directory.append(offset)
    directory.append(nb_blocks)
    offset += nb_blocks

# Write the bank content
with open(bank_name, "w") as bank:
    print "Writing %s..." % bank_name

    # Write the header
    bank.write(struct.pack("BB", 0, 1)) # Version 0, Read-only
    bank.write(struct.pack("<H", sampling_rate)) # Padding
    bank.write(struct.pack("<LLL", 0, 0, 0)) # Padding

    # Write the directory
    bank.write(struct.pack("<" + "L" * len(directory), *directory)) # Little endian
    
    # Sector padding
    pos = bank.tell()
    padding = [0] * (512 - pos)
    bank.write(struct.pack("B" * len(padding), *padding))

    # Write the files
    for keycode in FILE_TO_SLOT:
        filename = "%s.wav" % keycode
        w = wave.open(filename, "r")
        nb_blocks = w.getnframes() / 512
    
        for i in range(nb_blocks):
            data = w.readframes(512)
            bank.write(data)
        
        w.close()