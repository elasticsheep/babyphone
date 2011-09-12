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

/*****************************************************************************
* Hardware resources used:
*   Timer0 A & B interrupts
*   Timer4 Fast PWM on A & B outputs (PC7 & PB6)
******************************************************************************/

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "dac.h"
#include "interrupts.h"

/*****************************************************************************
* Definitions
******************************************************************************/
typedef struct dac_packet {
  uint8_t *start;
  uint8_t *end;
} t_dac_buffer;

static struct {
  uint16_t rate;
  uint8_t  channels;
  uint8_t  current_buffer;
  uint8_t* read_ptr;
  uint8_t* end_ptr;
} dac;

/*****************************************************************************
* Globals
******************************************************************************/
t_dac_buffer dac_buffer_pool[2];

volatile uint8_t empty_buffer_flag;


/*****************************************************************************
* Local prototypes
******************************************************************************/
void dac_timer_handler(void);

/*****************************************************************************
* Functions
******************************************************************************/

void dac_init(uint16_t rate, uint8_t channels)
{
  dac.rate = rate;
  dac.channels = channels;
  
  /* Init the internal PLL */
  PLLFRQ = _BV(PDIV2);
  PLLCSR = _BV(PLLE);
  while(!(PLLCSR & _BV(PLOCK)));
  PLLFRQ |= _BV(PLLTM0); /* PCK 48MHz */
  
  /* Init a fast PWM on Timer4 */
  TCCR4A = _BV(COM4A0) | _BV(PWM4A); /* Clear OC4A on Compare Match */
  TCCR4B = _BV(CS40); /* No prescaling => f = PCK/256 = 187500Hz */
  OCR4A = 0;
  
  /* Enable the OC4A output */
  DDRC |= _BV(PORTC7);
  
  /* Enable a second PWM channel */
  TCCR4A |= _BV(COM4B1) | _BV(PWM4B);
  OCR4B = 0;
  
  /* Enable the OC4B output */
  DDRB |= _BV(PORTB6);
  
  /* Init the buffer pool */
  memset(dac_buffer_pool, 0x00, sizeof(dac_buffer_pool));
  dac.current_buffer = 0;
  dac.read_ptr = NULL;
}

void dac_start(uint8_t* buffer0, uint8_t* buffer1, uint16_t size)
{
  /* Store the buffer params */
  dac_buffer_pool[0].start = buffer0;
  dac_buffer_pool[0].end = buffer0 + size;
  dac_buffer_pool[1].start = buffer1;
  dac_buffer_pool[1].end = buffer1 + size;
  
  /* Init the read pointer */
  dac.current_buffer = 0;
  dac.read_ptr = dac_buffer_pool[0].start;
  dac.end_ptr = dac_buffer_pool[0].end;
  
  empty_buffer_flag = 0;
  
  /* Setup a periodic interrupt to update the sample value */
  set_sample_timer_handler(&dac_timer_handler);
  
  TCCR0A = _BV(WGM01); /* CTC mode */
#if (F_CPU == 16000000)
  switch(dac.rate)
  {
    case 44100:
      TCCR0B = _BV(CS01); /* Fclk / 8 */
      OCR0A = 45 - 1; /* 44444 Hz */
      break;
    
    case 22050:
      TCCR0B = _BV(CS01); /* Fclk / 8 */
      OCR0A = 91 - 1; /* 21978 Hz */
      break;
    
    case 16000:
      TCCR0B = _BV(CS01); /* Fclk / 8 */
      OCR0A = 125 - 1; /* 16000 Hz */
      break;
      
    case 8000:
    default:
      TCCR0B = _BV(CS01); /* Fclk / 8 */
      OCR0A = 250 - 1; /* 8000 Hz */
      break;
  }
#else
#error F_CPU not supported
#endif
  
  /* Setup the buffer empty interrupt */
  OCR0B = 1;
  
  /* Enable the sample timer interrupt */
  TIMSK0 |= _BV(OCIE0A) | _BV(OCIE0B);
  
  /* Enable interrupts */
  sei();
}

void dac_pause(void)
{
  TIMSK0 &= ~_BV(OCIE0A);
}

void dac_resume(void)
{
  TIMSK0 |= _BV(OCIE0A);
}

void dac_stop(void)
{
  /* Stop the sample timer interrupt */
  TCCR0A = TCCR0B = OCR0A = TIMSK0 = 0;
  
  set_sample_timer_handler(NULL);
}

void dac_timer_handler(void)
{
  uint8_t l_sample = *dac.read_ptr++;
  OCR4A = l_sample;
  if (dac.channels == CHANNELS_STEREO)
  {
    OCR4B = *dac.read_ptr++;
  }
  else
  {
    OCR4B = l_sample;
  }

  /* Check the buffer end */
  if (dac.read_ptr >= dac.end_ptr)
  {
    /* Notify the client */
    empty_buffer_flag = dac.current_buffer + (1 << 4);

    /* Raise a buffer empty interrupt */
    TIMSK0 |= _BV(OCIE0B);
  
    /* Switch the current buffer */
    dac.current_buffer ^= 1;
    if (dac.current_buffer)
    {
      dac.read_ptr = dac_buffer_pool[1].start;
      dac.end_ptr = dac_buffer_pool[1].end;
    }
    else
    {
      dac.read_ptr = dac_buffer_pool[0].start;
      dac.end_ptr = dac_buffer_pool[0].end;
    }
  }
}