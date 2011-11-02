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
#include <avr/pgmspace.h>

#include "delay.h"

#include "LUFA/Drivers/Peripheral/SerialStream.h"

#include "keyboard.h"

/*****************************************************************************
* Constants
******************************************************************************/
#if 0
#define DEBUG
#endif

#define NB_MATRIX_ROWS (4)
#define NB_MATRIX_COLS (5)

uint8_t keycode[] =
{
  /* Row 0 */
  KEYCODE_1,    // Col 0
  KEYCODE_2,    // Col 1
  KEYCODE_3,    // Col 2
  KEYCODE_NONE, // Col 3
  KEYCODE_M1,   // Col 4
  
  /* Row 1 */
  KEYCODE_4,   // Col 0
  KEYCODE_5,   // Col 1
  KEYCODE_6,   // Col 2
  KEYCODE_R,   // Col 3
  KEYCODE_M2,  // Col 4
  
  /* Row 2 */
  KEYCODE_7,   // Col 0
  KEYCODE_8,   // Col 1
  KEYCODE_9,   // Col 2
  KEYCODE_MEM, // Col 3
  KEYCODE_M3,  // Col 4

  /* Row 3 */
  KEYCODE_STAR,  // Col 0
  KEYCODE_0,     // Col 1
  KEYCODE_SHARP, // Col 2
  KEYCODE_BIS,   // Col 3
  KEYCODE_ENR,  // Col 4
};

/*****************************************************************************
* Definitions
******************************************************************************/
enum {
  STATE_IDLE,
  STATE_DEBOUNCING,
  STATE_PRESSED,
};

/*****************************************************************************
* Globals
******************************************************************************/
struct {
  uint8_t state;
  uint8_t rawcode;
  uint8_t counter;
  uint8_t threshold;
} kbd;

/*****************************************************************************
* Function prototypes
******************************************************************************/

/*****************************************************************************
* Functions
******************************************************************************/

void keyboard_init(uint8_t debouncing_threshold)
{
  kbd.rawcode = 0xFF;
  kbd.counter = 0;
  kbd.state = STATE_IDLE;
  kbd.threshold = debouncing_threshold;
  
  /* Setup columns: input with pullup */
  DDRD &= ~(_BV(PORTD2) | _BV(PORTD4) | _BV(PORTD5) | _BV(PORTD6));
  PORTD |= _BV(PORTD2) | _BV(PORTD4) | _BV(PORTD5) | _BV(PORTD6);
  DDRC &= ~_BV(PORTC1); PORTC |= _BV(PORTC1);
  
  /* Setup rows: input highZ */
  DDRC &= ~(_BV(PORTC2) | _BV(PORTC3) | _BV(PORTC4) | _BV(PORTC5));
  PORTC &= ~(_BV(PORTC2) | _BV(PORTC3) | _BV(PORTC4) | _BV(PORTC5));
}

uint8_t read_col(uint8_t index)
{
  switch(index)
  {
    case 0: return (PIND & _BV(PORTD6)) > 0;
    case 1: return (PIND & _BV(PORTD5)) > 0;
    case 2: return (PIND & _BV(PORTD2)) > 0;
    case 3: return (PIND & _BV(PORTD4)) > 0;
    case 4: return (PINC & _BV(PORTC1)) > 0;
  }
  
  return 1;
}

void select_row(uint8_t index)
{
  /* Switch row to output low */
  switch(index)
  {
    case 0: DDRC |= _BV(PORTC2); break;
    case 1: DDRC |= _BV(PORTC3); break;
    case 2: DDRC |= _BV(PORTC4); break;
    case 3: DDRC |= _BV(PORTC5); break;
  }
}

void deselect_row(uint8_t index)
{
  /* Go back to input highZ */
  switch(index)
  {
    case 0: DDRC &= ~_BV(PORTC2); break;
    case 1: DDRC &= ~_BV(PORTC3); break;
    case 2: DDRC &= ~_BV(PORTC4); break;
    case 3: DDRC &= ~_BV(PORTC5); break;
  }
}

uint8_t matrix_scan(void)
{
  uint8_t row, col;
  
  for(row = 0; row < NB_MATRIX_ROWS; row++)
  {
    select_row(row);
    for(col = 0; col < NB_MATRIX_COLS; col++)
    {
      if (read_col(col) == 0)
      {
        deselect_row(row);
        return (row << 4 | col);
      }
    }
    deselect_row(row);
  }
  
  return 0;
}

uint8_t matrix_scan_key(uint8_t rawcode)
{
  uint8_t row = (rawcode >> 4);
  uint8_t col = (rawcode & 0x0F);
  
  select_row(row);
  if (read_col(col) == 0)
  {
    deselect_row(row);
    return 1;
  }
  deselect_row(row);
  
  return 0;
}

uint8_t raw2keycode(uint8_t rawcode)
{
  uint8_t row = (rawcode >> 4);
  uint8_t col = (rawcode & 0x0F);
  
  return keycode[row * NB_MATRIX_COLS + col];
}

void keyboard_update(uint8_t* event)
{
  uint8_t rawcode;
  uint8_t pressed;
  
  //printf("U");
  
  *event = 0;
  
  switch(kbd.state)
  {
    case STATE_IDLE:
      rawcode = matrix_scan();
      
      if (keycode > 0)
      {
        kbd.state = STATE_DEBOUNCING;
        kbd.rawcode = rawcode;
        kbd.counter = 0;
      }
      break;
      
    case STATE_DEBOUNCING:
      pressed = matrix_scan_key(kbd.rawcode);
    
      if (pressed == 0)
      {
        /* => Key released before end of debouncing */
      
        kbd.state = STATE_IDLE;
        kbd.rawcode = 0;
        kbd.counter = 0;
      }
      else
      {
        kbd.counter++;
        //printf("%i %02x\r\n", kbd.counter, kbd.rawcode);

        if (kbd.counter >= kbd.threshold)
        {
          /* Key pressed event */
#ifdef DEBUG
          printf("%iP %02x\r\n", kbd.state, kbd.rawcode);
#endif

          kbd.state = STATE_PRESSED;
          *event = EVENT_KEY_PRESSED | raw2keycode(kbd.rawcode);
        }
      }
      break;
      
    case STATE_PRESSED:
      pressed = matrix_scan_key(kbd.rawcode);
      //printf("%02x %i\r\n", kbd.rawcode, pressed);
  
      if (pressed == 0)
      {
        /* => Key released */
#ifdef DEBUG
        printf("%iR %02x\r\n", kbd.state, kbd.rawcode);
#endif

        *event = EVENT_KEY_RELEASED | raw2keycode(kbd.rawcode);
    
        kbd.state = STATE_IDLE;
        kbd.rawcode = 0;
        kbd.counter = 0;
      }
      break;

  }
  
  //printf("\r\n");
}