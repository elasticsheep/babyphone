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
#include <stddef.h>
#include <avr/interrupt.h>

#include "interrupts.h"

/*****************************************************************************
* Globals
******************************************************************************/
handler_t sample_timer_handler = NULL;
handler_t buffer_event_handler = NULL;

/*****************************************************************************
* Functions
******************************************************************************/

void set_sample_timer_handler(handler_t handler)
{
  sample_timer_handler = handler;
}

/* Sample timer interrupt */
ISR(TIMER0_COMPA_vect)
{
  /* Call the handler */
  if (sample_timer_handler)
    sample_timer_handler();
}

void set_buffer_event_handler(handler_t handler)
{
  buffer_event_handler = handler;
}

extern void buffer_refill_handler(void);

/* Buffer event interrupt */
ISR(TIMER0_COMPB_vect)
{
  /* Enable the buffer event interrupt */
  TIMSK0 &= ~_BV(OCIE0B);
  
  /* Re-enable the interrupts to allow the execution of TIMER0_COMPA_vect */
  //sei(); => cause glitch in the recording on buffer boundaries... (why ???)
  
  /* Call the handler */
  if (buffer_event_handler)
    buffer_event_handler();
}