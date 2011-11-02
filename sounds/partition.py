#!/usr/bin/python

import os
import struct
import wave

def build_partition(name, files, sampling_rate = 0):
    
    # Compute the size of each file
    entries = []
    offset = 1 # Start at sector 1
    for i, f in enumerate(files):
        w = wave.open(f, "r")
        nb_blocks = w.getnframes() / 512
        print "%s => offset %i, %i blocks" % (f, offset, nb_blocks)
        w.close()
    
        entries.append((offset, nb_blocks, nb_blocks))
        offset += nb_blocks

    # Write the partition content
    with open(name, "w") as output:
        print "Writing %s..." % name

        # Write the partition header
        output.write(struct.pack("BB", 0, 1)) # Version 0, Read-only
        output.write(struct.pack("<H", sampling_rate)) # Sampling rate
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

    # Print the number of blocks in the partition
    bytes = os.path.getsize(name)
    print "Partition size: %i blocks" % (bytes / 512)


def build_empty_rw_partition(name, nb_slots, nb_blocks_by_slot, sampling_rate = 0):
    
    # Write the partition content
    with open(name, "w") as output:
        print "Writing %s..." % name

        # Write the header
        output.write(struct.pack("BB", 0, 0)) # Version 0, Read-Write
        output.write(struct.pack("<H", 0)) # Default sampling rate
        output.write(struct.pack("<LLL", 0, 0, 0)) # Padding

        # Write the slot entries
        offset = 1
        for i in range(nb_slots):
            output.write(struct.pack("<LHH", offset, nb_blocks_by_slot, sampling_rate)) # Little endian
            offset += nb_blocks_by_slot
    
        # Sector padding
        pos = output.tell()
        if pos > 512:
            raise "Too many slots"
        else:
            padding = [0] * (512 - pos)
            output.write(struct.pack("B" * len(padding), *padding))

        # Slots padding
        empty_block = struct.pack("B", 0) * 512
        for i in range(nb_slots):
            for i in range(nb_blocks_by_slot):
                output.write(empty_block)

    # Print the number of blocks in the partition
    bytes = os.path.getsize(name)
    print "Partition size: %i blocks" % (bytes / 512)

def build_fs(name, partitions):
    pass