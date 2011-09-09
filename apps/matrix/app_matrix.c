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

#include "keyboard.h"

/*****************************************************************************
* Constants
******************************************************************************/

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

int application_main(void)
{
  set_sleep_mode(SLEEP_MODE_IDLE);

  SerialStream_Init(38400, false);

  keyboard_init();
  
  while(1)
  {
    uint8_t event;
    
    delay_ms(10);
    keyboard_update(&event);
    
    if (event & EVENT_KEY_PRESSED)
      printf("P %i\r\n", event & KEYCODE_MASK);
      
    if (event & EVENT_KEY_RELEASED)
      printf(" R %i\r\n", event & KEYCODE_MASK);
      
    event = 0;
  }

  return 0;
}

