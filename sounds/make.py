#!/usr/bin/python

PARTITIONS = [
    "dtmf/dtmf.part",
    "animals/animals.part",
    "record/12slots10sec.part",
]

if __name__ == '__main__':
    import sys
    sys.path.append("..")
    import partition
    
    partition.build_image("partitions.bin", PARTITIONS)
    
    print "To load the image on a SD card: dd if=<image> of=/dev/diskx bs=512"