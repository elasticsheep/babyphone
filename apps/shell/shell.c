
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

/* Drivers */
#include "keyboard.h"
#include "leds.h"
#include "slotfs.h"

/* Audio */
#include "player.h"
#include "recorder.h"

#include "dac.h"
#include "adc.h"
#include "buffer.h"

#include "delay.h"

#include "LUFA/Drivers/Peripheral/SerialStream.h"

/*****************************************************************************
* Globals
******************************************************************************/
uint8_t IsPlaying = 0;
uint8_t IsRecording = 0;

char keycode2char[] =
{
  0,
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  '0',
  '*',
  '#',
  'E',
  'M',
  'R',
  'B',
  'X', // M1
  'Y', // M2
  'Z', // M3
};

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
  
  /* Acknowledge the event */
  empty_buffer_flag = 0;
  buffer_full_flag = 0;
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

void play_slot(uint8_t partition, uint8_t slot)
{
  uint32_t start_block;
  uint16_t content_blocks;
  uint16_t sampling_rate = 0;

  if (IsPlaying)
    player_stop();
  
  IsPlaying = 1;

  /* Read content info */
  slotfs_get_partition_info(partition, &sampling_rate, NULL);
  slotfs_get_slot_info(partition, slot, &start_block, NULL, &content_blocks);

  printf("Sampling rate = %u\r\n", sampling_rate);
  printf("Start block = %lu\r\n", start_block);
  printf("Content blocks = %u\r\n", content_blocks);

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

void end_of_record(void* opaque)
{
  uint16_t nb_written_blocks;
  
  printf_P(PSTR("End of record slot %i\r\n"), (uint8_t)opaque);
  
  recorder_stop(&nb_written_blocks);
  IsRecording = 0;
  
  printf("Written blocks = %u\r\n", nb_written_blocks);
  
  /* Update the slot header */
  slotfs_update_slot_content_size(2, (uint8_t)opaque, nb_written_blocks);
}

void record(uint32_t start_sector, uint16_t nb_sectors)
{
  printf_P(PSTR("Start recording...\r\n"));
  
  if (IsRecording)
    recorder_stop(NULL);
  
  IsRecording = 1;
  
  /* Start the recording */
  recorder_start(start_sector, nb_sectors, &end_of_record, NULL);
}

void record_slot(uint8_t slot)
{
  uint32_t start_block;
  uint16_t max_content_blocks;
  uint16_t sampling_rate = 0;
  
  uint8_t partition = 2;

  if (IsRecording)
    recorder_stop(NULL);
  
  IsRecording = 1;

  /* Read content position and max size */
  slotfs_get_partition_info(partition, &sampling_rate, NULL);
  slotfs_get_slot_info(partition, slot, &start_block, &max_content_blocks, NULL);

  printf("Sampling rate = %u\r\n", sampling_rate);
  printf("Start block = %lu\r\n", start_block);
  printf("Max content blocks = %u\r\n", max_content_blocks);

  if (max_content_blocks > 0)
  {
    /* Start the recording */
    printf_P(PSTR("Start recording...\r\n"));

    recorder_start(start_block, max_content_blocks, &end_of_record, (void*)slot);
  }
  else
  {
    printf_P(PSTR("No blocks in slot\r\n"));
  }
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

    while(1)
    {
        /* setup sd card slot */
        if(!sd_raw_init())
        {
            uart_puts_p(PSTR("MMC/SD initialization failed\n"));
            continue;
        }

        if(!slotfs_init())
        {
            uart_puts_p(PSTR("SlotFS initialization failed\n"));
            continue;
        }
        
        player_init();

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
            }
            else if(strncmp_P(command, PSTR("adc"), 3) == 0)
            {
              set_buffer_event_handler(&buffer_event);
            
              adc_init(0);
              adc_start(&pcm_buffer[0], &pcm_buffer[PCM_BUFFER_SIZE], PCM_BUFFER_SIZE);
            }
            else if(strncmp_P(command, PSTR("rec\0"), 4) == 0)
            {
              record(0, 64);
            }
            else if(strncmp_P(command, PSTR("recslot "), 8) == 0)
            {
              command += 8;
              if(command[0] == '\0')
                continue;

              uint32_t slot = strtolong(command);
              record_slot((uint8_t)slot);
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
              play_slot(1, (uint8_t)slot);
            }
            else if(strncmp_P(command, PSTR("ls\0"), 3) == 0)
            {
              /* Display the partitions content */
              uint8_t i;
              uint8_t nb_partitions;
            
              nb_partitions = slotfs_get_nb_partitions();
              printf_P(PSTR("%i partitions\r\n"), nb_partitions);
            
              for(i = 0; i < nb_partitions; i++)
              {
                uint8_t j;
                uint16_t sampling_rate = 0;
                uint8_t nb_slots;
                uint32_t start_block;
                uint16_t max_content_blocks;
                uint16_t nb_content_blocks;
                
                printf_P(PSTR("Partition %u\r\n"), i);
                  
                slotfs_get_partition_info(i, &sampling_rate, &nb_slots);
                printf_P(PSTR("%u slots\r\n"), nb_slots);
                
                if (sampling_rate == 0)
                  printf_P(PSTR("8000 Hz\r\n"));
                else
                  printf_P(PSTR("%u Hz\r\n"), sampling_rate);
                  
                for(j = 0; j< nb_slots; j++)
                {
                  slotfs_get_slot_info(i, j, &start_block, &max_content_blocks, &nb_content_blocks);

                  printf("Slot %02i: %4lu ", j, start_block);
                  printf("%u/%u\r\n", nb_content_blocks, max_content_blocks);
                }
              }
            }
            else if(strncmp_P(command, PSTR("kbd\0"), 4) == 0)
            {
              /* Matrix keyboad test loop */
              
              keyboard_init(2);
              
              while(1)
              {
                uint8_t event;

                delay_ms(10);
                keyboard_update(&event);

                if (event & EVENT_KEY_PRESSED)
                  printf("P %i %c\r\n", event & KEYCODE_MASK, keycode2char[event & KEYCODE_MASK]);

                if (event & EVENT_KEY_RELEASED)
                  printf(" R %i\r\n", event & KEYCODE_MASK);

                event = 0;
              }
            }
            else if(strncmp_P(command, PSTR("leds\0"), 5) == 0)
            {
              /* Blinking led test */
              
              leds_init();
              while(1)
              {
                leds_set(LED_RED, 1);
                delay_ms(250);
                leds_set(LED_RED, 0);
                delay_ms(250);
                leds_set(LED_GREEN, 1);
                delay_ms(500);
                leds_set(LED_GREEN, 0);
                delay_ms(500);
              }
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

