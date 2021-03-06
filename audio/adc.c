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
* Hardware resources used:
*   Timer0 A & B interrupts
*   ATmega32U4: ADC input ADC0 (PF0)
*   ATmega328P: ADC input ADC0 (PC0)
******************************************************************************/

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "adc.h"
#include "interrupts.h"

/*****************************************************************************
* Definitions
******************************************************************************/
typedef struct {
  uint8_t *start;
  uint8_t *end;
} t_adc_buffer;

static struct {
  uint8_t* read_ptr;
  uint8_t* end_ptr;
  uint8_t  current_buffer;
  uint8_t  threshold;
  uint8_t  triggered;
} adc;

/*****************************************************************************
* Globals
******************************************************************************/
t_adc_buffer adc_buffer_pool[2];

volatile uint8_t buffer_full_flag;

/*****************************************************************************
* Functions
******************************************************************************/

void adc_init(uint8_t vad)
{
  /* Configure a led output for the triggered signal */
  DDRB |= _BV(PORTB0);
  PORTB &= ~_BV(PORTB0);

  /* Init the context */
  adc.threshold = 64;
  adc.triggered = 0;
  
  if (!vad)
    adc.triggered = 1; /* Voice activity detection */

  /* Init the buffer pool */
  memset(adc_buffer_pool, 0x00, sizeof(adc_buffer_pool));
  adc.current_buffer = 0;
  adc.read_ptr = NULL;
}

void adc_shutdown(void)
{

}

void adc_enable(void)
{
  /* Initialize the ADC on ADC0 */
  ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); /* Enable the ADC, prescaler 128 */
  ADMUX |= _BV(REFS0) | _BV(ADLAR); /* AVCC ref with cap on AREF, left justify, mux on ADC0 */

#ifdef __AVR_ATmega32U4__
  DDRF  &= ~_BV(PF0); /* Setup ADC0 as an input */
  DIDR0 |=  _BV(ADC0D); /* Disable the digital input buffer on PF0*/
#elif defined(__AVR_ATmega328P__)
  DDRC &= ~_BV(PC0); /* Setup ADC0 as an input */
  DIDR0 |=  _BV(ADC0D); /* Disable the digital input buffer on PC0*/
#endif
}

void adc_disable(void)
{
  /* Disable the ADC */
  ADCSRA &= _BV(ADEN);
}

void adc_start(uint8_t* buffer0, uint8_t* buffer1, uint16_t size)
{
  /* Store the buffer params */
  adc_buffer_pool[0].start = buffer0;
  adc_buffer_pool[0].end = buffer0 + size;
  adc_buffer_pool[1].start = buffer1;
  adc_buffer_pool[1].end = buffer1 + size;
  
  /* Init the read pointer */
  adc.current_buffer = 0;
  adc.read_ptr = adc_buffer_pool[0].start;
  adc.end_ptr = adc_buffer_pool[0].end;
  
  buffer_full_flag = 0;
  
  /* Enable the ADC block */
  adc_enable();
  
  /* Setup a periodic interrupt to update the sample value */
  set_sample_timer_handler(&adc_timer_handler);
    
  TCCR0A = _BV(WGM01); /* CTC mode */
  
#if (F_CPU == 16000000)
  TCCR0B = _BV(CS01); /* Fclk / 8 */
  OCR0A = 250 - 1; /* 8000 Hz */
#else
#error F_CPU not supported
#endif
  
  /* Setup the buffer full interrupt */
  OCR0B = 1;
  
  /* Enable the sample timer interrupt */
  TIMSK0 |= _BV(OCIE0A) | _BV(OCIE0B);
  
  /* Enable interrupts */
  sei();
}

void adc_stop(void)
{
  /* Stop the sample timer interrupt */
  TCCR0A = TCCR0B = OCR0A = TIMSK0 = 0;
  
  /* Disable the ADC block */
  adc_disable();
  
  /* Disable the led */
  PORTB &= ~_BV(PORTB0);
  
  set_sample_timer_handler(NULL);
}

void adc_timer_handler(void)
{
  uint8_t sample = 0;
  
  /* Start a conversion */
  ADCSRA |= _BV(ADSC);

  /* Wait for the end of the conversion */
  while (ADCSRA & _BV(ADSC));

  /* Clear the interrupt flag */
  ADCSRA |= _BV(ADIF); 

  /* Read the sampled value */
  sample = ADCH;

  if (adc.triggered)
  {
    /* Store the sampled value in a buffer */
    *adc.read_ptr = sample;
    adc.read_ptr++;

    /* Check the buffer end */
    if (adc.read_ptr >= adc.end_ptr)
    {
      /* Check for buffer overflow */
      if (buffer_full_flag != 0)
      {
        printf("O");
        adc_stop();

        return;
      }

      /* Store the current buffer */
      buffer_full_flag = 0x80 + adc.current_buffer;

      /* Switch the current buffer */
      adc.current_buffer ^= 1;
      if (adc.current_buffer)
      {
        adc.read_ptr = adc_buffer_pool[1].start;
        adc.end_ptr = adc_buffer_pool[1].end;
      }
      else
      {
        adc.read_ptr = adc_buffer_pool[0].start;
        adc.end_ptr = adc_buffer_pool[0].end;
      }

      /* Enable the buffer event interrupt */
      TIMSK0 |= _BV(OCIE0B);
    }
  }
  else
  {
    if (  (sample < (128 - adc.threshold))
       || (sample > (128 + adc.threshold)) )
    {
      adc.triggered = 1;
      
      /* Enable the led */
      PORTB |= _BV(PORTB0);
    }
  }
}

