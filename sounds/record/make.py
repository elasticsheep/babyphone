#!/usr/bin/python

if __name__ == '__main__':
    import sys
    sys.path.append("..")
    import slotfs

    slotfs.build_empty_rw_fs("12slots10sec.slotfs", 12, 156)
