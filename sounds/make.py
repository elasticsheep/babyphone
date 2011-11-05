#!/usr/bin/python

PARTITIONS = [
    "dtmf/dtmf.slotfs",
    "animals/animals.slotfs",
    "record/12slots10sec.slotfs",
]

if __name__ == '__main__':
    import sys
    sys.path.append("..")
    import slotfs
    
    slotfs.build_image("babyphone.image", PARTITIONS)
    
    print "To load the image on a SD card: dd if=<image> of=/dev/diskx bs=512"