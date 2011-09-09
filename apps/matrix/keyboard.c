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
#define NB_ROWS (3)
#define NB_COLS (3)

uint8_t keycode[] =
{
  KEYCODE_1, // row 0 col 0
  KEYCODE_2, // row 0 col 1
  KEYCODE_3, // row 0 col 2
  KEYCODE_4, // row 1 col 0
  KEYCODE_5, // row 1 col 1
  KEYCODE_6, // row 1 col 2
  KEYCODE_7, // row 2 col 0
  KEYCODE_8, // row 2 col 1
  KEYCODE_9, // row 2 col 2
};

#define DEBOUNCING_THRESHOLD (5)

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
} kbd;

/*****************************************************************************
* Function prototypes
******************************************************************************/

/*****************************************************************************
* Functions
******************************************************************************/

void keyboard_init(void)
{
  kbd.rawcode = 0xFF;
  kbd.counter = 0;
  kbd.state = STATE_IDLE;
  
  /* Setup columns: input with pullup */
  DDRF &= ~_BV(PORTF4); PORTF |= _BV(PORTF4); // Col 0
  DDRF &= ~_BV(PORTF5); PORTF |= _BV(PORTF5); // Col 1
  DDRF &= ~_BV(PORTF6); PORTF |= _BV(PORTF6); // Col 2
  
  /* Setup rows: input highZ */
  DDRF &= ~_BV(PORTF7); PORTF &= ~_BV(PORTF7); // Row 0 
  DDRB &= ~_BV(PORTB4); PORTB &= ~_BV(PORTB4); // Row 1
  DDRD &= ~_BV(PORTD7); PORTD &= ~_BV(PORTD7); // Row 2
}

uint8_t read_col(uint8_t index)
{
  switch(index)
  {
    case 0:
      return (PINF & _BV(PORTF4)) > 0;
    case 1:
      return (PINF & _BV(PORTF5)) > 0;
    case 2:
      return (PINF & _BV(PORTF6)) > 0;
  }
  
  return 1;
}

void select_row(uint8_t index)
{
  /* Switch row to output low */
  
  switch(index)
  {
    case 0:
      DDRF |= _BV(PORTF7);
      break;
    case 1:
      DDRB |= _BV(PORTB4);
      break;
    case 2:
      DDRD |= _BV(PORTD7);
      break;
  }
}

void deselect_row(uint8_t index)
{
  /* Go back to input highZ */
  
  switch(index)
  {
    case 0:
      DDRF &= ~_BV(PORTF7);
      break;
    case 1:
      DDRB &= ~_BV(PORTB4);
      break;
    case 2:
      DDRD &= ~_BV(PORTD7);
      break;
  }
}

uint8_t matrix_scan(void)
{
  uint8_t row, col;
  
  for(row = 0; row < NB_ROWS; row++)
  {
    select_row(row);
    for(col = 0; col < NB_COLS; col++)
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
  
  return keycode[row * NB_COLS + col];
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

        if (kbd.counter >= DEBOUNCING_THRESHOLD)
        {
          /* Key pressed event */
          //printf("%iP %02x\r\n", kbd.state, kbd.rawcode);

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
        //printf("%i+R %02x\r\n", kbd.state, kbd.rawcode);

        *event = EVENT_KEY_RELEASED | raw2keycode(kbd.rawcode);
    
        kbd.state = STATE_IDLE;
        kbd.rawcode = 0;
        kbd.counter = 0;
      }
      break;

  }
  
  //printf("\r\n");
}