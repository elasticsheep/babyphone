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

#define KEYCODE_MASK        (0x0F)

enum {
  KEYCODE_1 = 1,
  KEYCODE_2,
  KEYCODE_3,
  KEYCODE_4,
  KEYCODE_5,
  KEYCODE_6,
  KEYCODE_7,
  KEYCODE_8,
  KEYCODE_9,
  KEYCODE_ENR,
  KEYCODE_MEM,
  KEYCODE_R,
  KEYCODE_BIS,
  KEYCODE_M1,
  KEYCODE_M2,
  KEYCODE_M3,
  KEYCODE_HF,
};

#define EVENT_MASK          (0xC0)
#define EVENT_NO_KEY        (0 << 6)
#define EVENT_KEY_PRESSED   (1 << 6)
#define EVENT_KEY_RELEASED  (2 << 6)

void keyboard_init(void);
void keyboard_update(uint8_t* event);