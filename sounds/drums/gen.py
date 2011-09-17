#!/usr/bin/python

import sys
sys.path.append("..")
import bank

FILES = [
    "floortom.wav",
    "kick.wav",
    "closedhat.wav",
]

bank.generate("drums.bank", FILES, sampling_rate = 16000)
