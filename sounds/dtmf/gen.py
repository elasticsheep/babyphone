#!/usr/bin/python

import sys
sys.path.append("..")

import bank

KEYCODE_TO_SLOT = [
    "1.wav",
    "2.wav",
    "3.wav",
    "4.wav",
    "5.wav",
    "6.wav",
    "7.wav",
    "8.wav",
    "9.wav",
    "star.wav",
    "0.wav",
    "sharp.wav"
]

bank.generate("dtmf.bank", KEYCODE_TO_SLOT)
