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
#define NB_MATRIX_ROWS (4)
#define NB_MATRIX_COLS (5)

uint8_t keycode[] =
{
  /* Row 0 */
  KEYCODE_1,   // Col 0
  KEYCODE_2,   // Col 1
  KEYCODE_3,   // Col 2
  KEYCODE_ENR, // Col 3
  KEYCODE_M1,  // Col 4
  
  /* Row 1 */
  KEYCODE_4,   // Col 0
  KEYCODE_5,   // Col 1
  KEYCODE_6,   // Col 2
  KEYCODE_MEM, // Col 3
  KEYCODE_M2,  // Col 4
  
  /* Row 2 */
  KEYCODE_7,   // Col 0
  KEYCODE_8,   // Col 1
  KEYCODE_9,   // Col 2
  KEYCODE_R,   // Col 3
  KEYCODE_M3,  // Col 4

  /* Row 3 */
  KEYCODE_STAR,  // Col 0
  KEYCODE_0,     // Col 1
  KEYCODE_SHARP, // Col 2
  KEYCODE_BIS,   // Col 3
  KEYCODE_NONE,  // Col 4
  
  /* Row 4 => Non matrix keys */
  KEYCODE_HF,    // Col 0
  KEYCODE_SW0,   // Col 1
  KEYCODE_SW1,   // Col 2
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
  DDRF &= ~_BV(PORTF1); PORTF |= _BV(PORTF1); // Col 0
  DDRF &= ~_BV(PORTF4); PORTF |= _BV(PORTF4); // Col 1
  DDRF &= ~_BV(PORTF5); PORTF |= _BV(PORTF5); // Col 2
  
  /* Setup rows: input highZ */
  DDRF &= ~_BV(PORTF6); PORTF &= ~_BV(PORTF6); // Row 0 
  DDRF &= ~_BV(PORTF7); PORTF &= ~_BV(PORTF7); // Row 1
  DDRD &= ~_BV(PORTD7); PORTD &= ~_BV(PORTD7); // Row 2
  
  /* Setup non-matrix keys */
  DDRB &= ~_BV(PORTB4); PORTB |= _BV(PORTB4); // SW 0 with pullup
  DDRB &= ~_BV(PORTB5); PORTB |= _BV(PORTB5); // SW 1 with pullup
}

uint8_t read_col(uint8_t index)
{
  switch(index)
  {
    case 0:
      return (PINF & _BV(PORTF1)) > 0;
    case 1:
      return (PINF & _BV(PORTF4)) > 0;
    case 2:
      return (PINF & _BV(PORTF5)) > 0;
  }
  
  return 1;
}

void select_row(uint8_t index)
{
  /* Switch row to output low */
  
  switch(index)
  {
    case 0:
      DDRF |= _BV(PORTF6);
      break;
    case 1:
      DDRF |= _BV(PORTF7);
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
      DDRF &= ~_BV(PORTF6);
      break;
    case 1:
      DDRF &= ~_BV(PORTF7);
      break;
    case 2:
      DDRD &= ~_BV(PORTD7);
      break;
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

uint8_t keyboard_scan(void)
{
  /* Non matrix keys */
  if ((PINB & _BV(PORTB4)) == 0)
    return (4 << 4) | 1;

  if ((PINB & _BV(PORTB5)) == 0)
    return (4 << 4) | 2;

  /* Matrix keyboard */
  return matrix_scan();
}

uint8_t keyboard_scan_key(uint8_t rawcode)
{
  /* Non matrix keys */
  if ((rawcode >> 4) >= 4)
  {
    switch(rawcode & 0x0F)
    {
      case 1:
        return ((PINB & _BV(PORTB4)) == 0);

      case 2:
        return ((PINB & _BV(PORTB5)) == 0);
    }
    
    return 0;
  }

  /* Matrix keyboard */
  return matrix_scan_key(rawcode);
}

uint8_t raw2keycode(uint8_t rawcode)
{
  uint8_t row = (rawcode >> 4);
  uint8_t col = (rawcode & 0x0F);
  
  printf("%u\r\n", row * NB_MATRIX_COLS + col);
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
      rawcode = keyboard_scan();
      
      if (keycode > 0)
      {
        kbd.state = STATE_DEBOUNCING;
        kbd.rawcode = rawcode;
        kbd.counter = 0;
      }
      break;
      
    case STATE_DEBOUNCING:
      pressed = keyboard_scan_key(kbd.rawcode);
    
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
          printf("%iP %02x\r\n", kbd.state, kbd.rawcode);

          kbd.state = STATE_PRESSED;
          *event = EVENT_KEY_PRESSED | raw2keycode(kbd.rawcode);
        }
      }
      break;
      
    case STATE_PRESSED:
      pressed = keyboard_scan_key(kbd.rawcode);
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