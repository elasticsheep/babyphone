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
* Includes
******************************************************************************/
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

/*****************************************************************************
* Function prototype
******************************************************************************/
extern void application_main(void);

/*****************************************************************************
* Functions
******************************************************************************/

void basic_setup(void)
{
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

#if (BOARD == BOARD_USER)
#if (F_CPU == 16000000UL)
  /* Set the clock prescaler */
  clock_prescale_set(clock_div_1);
#elif (F_CPU == 8000000UL)
  /* Set the clock prescaler */
  clock_prescale_set(clock_div_2);
#else
#error Unsupported F_CPU
#endif
#endif
}

int main()
{
  basic_setup();
  application_main();
  return 0;
}
