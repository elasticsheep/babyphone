#!/usr/bin/python

import os
import struct
import wave

def generate(name, files, sampling_rate = 0):
    
    # Compute the size of each file
    entries = []
    offset = 1 # Start at sector 1 in the bank
    for i, f in enumerate(files):
        w = wave.open(f, "r")
        nb_blocks = w.getnframes() / 512
        print "%s => offset %i, %i blocks" % (f, offset, nb_blocks)
        w.close()
    
        entries.append((offset, nb_blocks, nb_blocks))
        offset += nb_blocks

    # Write the bank content
    with open(name, "w") as output:
        print "Writing %s..." % name

        # Write the header
        output.write(struct.pack("BB", 0, 1)) # Version 0, Read-only
        output.write(struct.pack("<H", sampling_rate)) # Padding
        output.write(struct.pack("<LLL", 0, 0, 0)) # Padding

        # Write the slot entries
        for entry in entries:
            output.write(struct.pack("<LHH", *entry)) # Little endian
    
        # Sector padding
        pos = output.tell()
        if pos > 512:
            raise "Too many slots"
        else:
            padding = [0] * (512 - pos)
            output.write(struct.pack("B" * len(padding), *padding))

        # Write the files
        for f in files:
            w = wave.open(f, "r")
            nb_blocks = w.getnframes() / 512
    
            for i in range(nb_blocks):
                data = w.readframes(512)
                output.write(data)
        
            w.close()

    # Print the number of blocks in the bank
    bytes = os.path.getsize(name)
    print "Bank size: %i blocks" % (bytes / 512)

