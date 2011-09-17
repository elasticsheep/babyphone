
/*
 * Copyright (c) 2006-2009 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

#include "sd_raw.h"
#include "sd_raw_config.h"
#include "uart.h"

#include <stdio.h>

#include "banks.h"
#include "player.h"
#include "recorder.h"

#include "adc.h"
#include "buffer.h"

#include "delay.h"

#include "LUFA/Drivers/Peripheral/SerialStream.h"

/*****************************************************************************
* Globals
******************************************************************************/
uint8_t IsPlaying = 0;
uint8_t IsRecording = 0;

/*****************************************************************************
* Local prototypes
******************************************************************************/
uint8_t read_line(char* buffer, uint8_t buffer_length);
uint32_t strtolong(const char* str);

/*****************************************************************************
* Functions
******************************************************************************/

void buffer_event(void)
{
  printf("B");
}

void end_of_playback(void)
{
  printf_P(PSTR("End of playback\r\n"));
  
  player_stop();
  IsPlaying = 0;
}

void play(uint32_t start_sector, uint16_t nb_sectors)
{
  printf_P(PSTR("Start playing...\r\n"));

  if (IsPlaying)
    player_stop();
  
  IsPlaying = 1;

  /* Start the playback */
  player_start(start_sector, nb_sectors, &end_of_playback);
}

void play_slot(uint8_t slot)
{
  uint32_t start_block, content_blocks;
  uint16_t sampling_rate = 0;

  if (IsPlaying)
    player_stop();
  
  IsPlaying = 1;

  /* Read content position and size */
  bank_read(0, &sampling_rate);
  bank_read_slot(0, slot, &start_block, &content_blocks);

  printf("Bank sampling rate = %u\r\n", sampling_rate);
  printf("Start block = %lu\r\n", start_block);
  printf("Content blocks = %lu\r\n", content_blocks);

  if (content_blocks > 0)
  {
    /* Start the playback */
    printf_P(PSTR("Start playing...\r\n"));
    
    player_set_option(PLAYER_OPTION_SAMPLING_RATE, sampling_rate);
    player_set_option(PLAYER_OPTION_LOOP_MODE, 0);
    
    player_start(start_block, content_blocks, &end_of_playback);
  }
  else
  {
    printf_P(PSTR("Empty slot\r\n"));
  }
}

void end_of_record(void)
{
  printf_P(PSTR("End of record\r\n"));
  
  recorder_stop();
  IsRecording = 1;
}

void record(uint32_t start_sector, uint16_t nb_sectors)
{
  printf_P(PSTR("Start recording...\r\n"));
  
  if (IsRecording)
    recorder_stop();
  
  IsRecording = 1;
  
  /* Start the recording */
  recorder_start(start_sector, nb_sectors, &end_of_record);
}

void fill(uint32_t start_sector, uint16_t nb_sectors)
{
  uint32_t i = 0;
  
  printf_P(PSTR("Start filling...\r\n"));
  
  for(i = 0; i < 512; ++i)
  {
    pcm_buffer[i] = 0xCA;
  }
  
  for(i = start_sector; i < (start_sector + nb_sectors); i++)
  {
    sd_raw_write(i << 9, pcm_buffer, 512);
  }
  
  printf_P(PSTR("End of filling\r\n"));
}

void erase(uint32_t start_sector, uint16_t nb_sectors)
{
  uint8_t res = 0;
  
  printf_P(PSTR("Start erasing...\r\n"));
  
  /* Start the recording */
  res = sd_raw_erase_blocks(start_sector, nb_sectors);
  
  printf_P(PSTR("End of erasing (%i)\r\n"), res);
}

int application_main()
{
    /* we will just use ordinary idle mode */
    set_sleep_mode(SLEEP_MODE_IDLE);

    SerialStream_Init(38400, false);

#if 0
    /* Blinking led test */
    while(1)
    {
      DDRB |= _BV(0);
      PORTB ^= _BV(0);
      delay_ms(500);
    }
#endif

    player_init();

    while(1)
    {
        /* setup sd card slot */
        if(!sd_raw_init())
        {
            uart_puts_p(PSTR("MMC/SD initialization failed\n"));
            continue;
        }

        /* provide a simple shell */
        char buffer[24];
        while(1)
        {
            /* print prompt */
            uart_putc('>');
            uart_putc(' ');

            /* read command */
            char* command = buffer;
            if(read_line(command, sizeof(buffer)) < 1)
                continue;

            /* execute command */
            if(strcmp_P(command, PSTR("init")) == 0)
            {
                break;
            }
            else if(strncmp_P(command, PSTR("rawadc"), 3) == 0)
            {
#ifdef __AVR_ATmega32U4__
                /* Initialize the ADC on ADC0 */
                ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); /* Enable the ADC, prescaler 128 */
                ADMUX |= _BV(REFS0) | _BV(ADLAR); /* AVCC ref with cap on AREF, left justify, mux on ADC0 */

                DDRF  &= ~_BV(PF0); /* Setup ADC0 as an input */
                DIDR0 |=  _BV(ADC0D); /* Disable the digital input buffer on PF0*/
                
                while(1)
                {
                    /* Start a conversion */
                    ADCSRA |= _BV(ADSC);

                    /* Wait for the end of the conversion */
                    while (ADCSRA & _BV(ADSC));

                    /* Clear the interrupt flag */
                    ADCSRA |= _BV(ADIF); 

                    /* Print the sampled value */
                    printf_P(PSTR("%i\r\n"), ADCH);
                }
#endif
            }
            else if(strncmp_P(command, PSTR("adc"), 3) == 0)
            {
                set_buffer_event_handler(&buffer_event);
              
                adc_init();
                adc_start(&pcm_buffer[0], &pcm_buffer[PCM_BUFFER_SIZE], PCM_BUFFER_SIZE);
            }
            else if(strncmp_P(command, PSTR("rec"), 3) == 0)
            {
                record(0, 64);
            }
            else if(strncmp_P(command, PSTR("erase"), 5) == 0)
            {
                erase(0, 64);
            }
            else if(strncmp_P(command, PSTR("fill"), 4) == 0)
            {
                fill(0, 128);
            }
            else if(strncmp_P(command, PSTR("play\0"), 5) == 0)
            {
                play(0, 64);
            }
            else if(strncmp_P(command, PSTR("playslot "), 9) == 0)
            {
                command += 9;
                if(command[0] == '\0')
                    continue;

                uint32_t slot = strtolong(command);
                play_slot((uint8_t)slot);
            }
            else
            {
                uart_puts_p(PSTR("unknown command: "));
                uart_puts(command);
                uart_putc('\n');
            }
        }
    }
    
    return 0;
}

uint8_t read_line(char* buffer, uint8_t buffer_length)
{
    memset(buffer, 0, buffer_length);

    uint8_t read_length = 0;
    while(read_length < buffer_length - 1)
    {
        uint8_t c = uart_getc();

        if(c == 0x08 || c == 0x7f)
        {
            if(read_length < 1)
                continue;

            --read_length;
            buffer[read_length] = '\0';

            uart_putc(0x08);
            uart_putc(' ');
            uart_putc(0x08);

            continue;
        }

        uart_putc(c);

        if(c == '\n')
        {
            buffer[read_length] = '\0';
            break;
        }
        else
        {
            buffer[read_length] = c;
            ++read_length;
        }
    }

    return read_length;
}

uint32_t strtolong(const char* str)
{
    uint32_t l = 0;
    while(*str >= '0' && *str <= '9')
        l = l * 10 + (*str++ - '0');

    return l;
}

