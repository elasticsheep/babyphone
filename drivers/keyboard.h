/*
  Copyright 2011  Mathieu SONET (contact [at] elasticsheep [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#define KEYCODE_MASK        (0x1F)

enum {
  KEYCODE_NONE  = 0,
  KEYCODE_1     = 1,
  KEYCODE_2     = 2,
  KEYCODE_3     = 3,
  KEYCODE_4     = 4,
  KEYCODE_5     = 5,
  KEYCODE_6     = 6,
  KEYCODE_7     = 7,
  KEYCODE_8     = 8,
  KEYCODE_9     = 9,
  KEYCODE_0     = 10,
  KEYCODE_STAR  = 11,
  KEYCODE_SHARP = 12,
  KEYCODE_ENR   = 13,
  KEYCODE_MEM   = 14,
  KEYCODE_R     = 15,
  KEYCODE_BIS   = 16,
  KEYCODE_M1    = 17,
  KEYCODE_M2    = 18,
  KEYCODE_M3    = 19,
};

#define EVENT_MASK          (0xC0)
#define EVENT_NO_KEY        (0 << 6)
#define EVENT_KEY_PRESSED   (1 << 6)
#define EVENT_KEY_RELEASED  (2 << 6)

void keyboard_init(uint8_t debouncing_threshold);
void keyboard_update(uint8_t* event);