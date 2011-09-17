/*
  Copyright 2010  Mathieu SONET (contact [at] elasticsheep [dot] com)

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

#ifndef DAC_H
#define DAC_H

enum {
  FORMAT_8_BITS,
  FORMAT_16_BITS_LE,
};

enum {
  CHANNELS_MONO,
  CHANNELS_STEREO,
};

void dac_init(uint16_t rate);
void dac_start(uint8_t* buffer0, uint8_t* buffer1, uint16_t size);
void dac_stop(void);
void dac_pause(void);
void dac_resume(void);

extern volatile uint8_t empty_buffer_flag;
void dac_timer_handler(void);

#endif /* DAC_H */