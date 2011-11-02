#!/usr/bin/python

if __name__ == '__main__':
    import sys
    sys.path.append("..")
    import partition

    partition.build_empty_rw_partition("12slots10sec.part", 12, 156)
