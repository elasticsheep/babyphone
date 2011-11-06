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

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <avr/io.h>

#include "leds.h"

/*****************************************************************************
* Functions
******************************************************************************/

void leds_init(void)
{
  DDRB |= _BV(PORTB0) | _BV(PORTB1);
  PORTB |= _BV(PORTB0) | _BV(PORTB1);
}

void leds_set(enum led led, uint8_t state)
{
  if (state == 0)
  {
    switch(led)
    {
      case LED_GREEN: PORTB |= _BV(PORTB0); break;
      case LED_RED:   PORTB |= _BV(PORTB1); break;
      default: break;
    }
  }
  else
  {
    switch(led)
    {
      case LED_GREEN: PORTB &= ~_BV(PORTB0); break;
      case LED_RED:   PORTB &= ~_BV(PORTB1); break;
      default: break;
    }
  }
}