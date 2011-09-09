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
#include <avr/power.h>
#include <avr/sleep.h>

#include "delay.h"

#include "LUFA/Drivers/Peripheral/SerialStream.h"

/*****************************************************************************
* Constants
******************************************************************************/
#define NB_ROWS (3)
#define NB_COLS (3)

char keycode[] =
{
  '1', // row 0 col 0
  '2', // row 0 col 1
  '3', // row 0 col 2
  '4', // row 1 col 0
  '5', // row 1 col 1
  '6', // row 1 col 2
  '7', // row 2 col 0
  '8', // row 2 col 1
  '9', // row 2 col 2
};

/*****************************************************************************
* Definitions
******************************************************************************/

/*****************************************************************************
* Globals
******************************************************************************/

/*****************************************************************************
* Function prototypes
******************************************************************************/

/*****************************************************************************
* Functions
******************************************************************************/

void init_cols_rows(void)
{
  /* Setup columns: input with pullup */
  DDRF &= ~_BV(PORTF4); PORTF |= _BV(PORTF4); // Col 0
  DDRF &= ~_BV(PORTF5); PORTF |= _BV(PORTF5); // Col 1
  DDRF &= ~_BV(PORTF6); PORTF |= _BV(PORTF6); // Col 2
  
  /* Setup rows: output high */
  DDRF |= _BV(PORTF7); PORTF |= _BV(PORTF7); // Row 0 
  DDRB |= _BV(PORTB4); PORTB |= _BV(PORTB4); // Row 1
  DDRD |= _BV(PORTD7); PORTD |= _BV(PORTD7); // Row 2
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
  switch(index)
  {
    case 0:
      PORTF &= ~_BV(PORTF7);
      break;
    case 1:
      PORTB &= ~_BV(PORTB4);
      break;
    case 2:
      PORTD &= ~_BV(PORTD7);
      break;
  }
}

void deselect_row(uint8_t index)
{
  switch(index)
  {
    case 0:
      PORTF |= _BV(PORTF7);
      break;
    case 1:
      PORTB |= _BV(PORTB4);
      break;
    case 2:
      PORTD |= _BV(PORTD7);
      break;
  }
}

char scan_keyboard(void)
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
        return keycode[row * NB_COLS + col];
      }
    }
    deselect_row(row);
  }
}

int application_main(void)
{
  set_sleep_mode(SLEEP_MODE_IDLE);

  SerialStream_Init(38400, false);

  init_cols_rows();
  
  while(1)
  {
    char key = scan_keyboard();
    if (key > 0)
      printf("%c", key);
    
    delay_ms(10);
  }

  return 0;
}

