
/*
 * Copyright (c) 2006-2009 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

#include "fat.h"
#include "fat_config.h"
#include "partition.h"
#include "sd_raw.h"
#include "sd_raw_config.h"
#include "uart.h"

#include <stdio.h>

#include "player.h"
#include "recorder.h"

#include "adc.h"
#include "buffer.h"

//#include "LUFA/Drivers/Peripheral/SerialStream.h"

#define DEBUG 1

static uint8_t read_line(char* buffer, uint8_t buffer_length);
static uint32_t strtolong(const char* str);
static uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry);
static struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name); 
static uint8_t print_disk_info(const struct fat_fs_struct* fs);

static uint8_t IsPlaying = 0;
static uint8_t IsRecording = 0;

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

    //SerialStream_Init(38400, false);

    while(1)
    {
        /* setup sd card slot */
        if(!sd_raw_init())
        {
#if DEBUG
            uart_puts_p(PSTR("MMC/SD initialization failed\n"));
#endif
            continue;
        }

        /* open first partition */
        struct partition_struct* partition = NULL;
#if 0
        partition = partition_open(sd_raw_read,
                                                            sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
                                                            sd_raw_write,
                                                            sd_raw_write_interval,
#else
                                                            0,
                                                            0,
#endif
                                                            0
                                                           );

        if(!partition)
        {
            /* If the partition did not open, assume the storage device
             * is a "superfloppy", i.e. has no MBR.
             */
            partition = partition_open(sd_raw_read,
                                       sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
                                       sd_raw_write,
                                       sd_raw_write_interval,
#else
                                       0,
                                       0,
#endif
                                       -1
                                      );
            if(!partition)
            {
#if DEBUG
                uart_puts_p(PSTR("opening partition failed\n"));
#endif
                continue;
            }
        }
#endif

        /* open file system */
        struct fat_fs_struct* fs = NULL;
#if 0
        fs = fat_open(partition);
        if(!fs)
        {
#if DEBUG
            uart_puts_p(PSTR("opening filesystem failed\n"));
#endif
            continue;
        }

        /* open root directory */
        struct fat_dir_entry_struct directory;
        fat_get_dir_entry_of_path(fs, "/", &directory);
#endif

        struct fat_dir_struct* dd = NULL;
#if 0
        dd = fat_open_dir(fs, &directory);
        if(!dd)
        {
#if DEBUG
            uart_puts_p(PSTR("opening root directory failed\n"));
#endif
            continue;
        }
#endif

#if 0
        /* print some card information as a boot message */
        print_disk_info(fs);
#endif

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
            else if(strncmp_P(command, PSTR("cd "), 3) == 0)
            {
                command += 3;
                if(command[0] == '\0')
                    continue;

                /* change directory */
                struct fat_dir_entry_struct subdir_entry;
                if(find_file_in_dir(fs, dd, command, &subdir_entry))
                {
                    struct fat_dir_struct* dd_new = fat_open_dir(fs, &subdir_entry);
                    if(dd_new)
                    {
                        fat_close_dir(dd);
                        dd = dd_new;
                        continue;
                    }
                }

                uart_puts_p(PSTR("directory not found: "));
                uart_puts(command);
                uart_putc('\n');
            }
            else if(strcmp_P(command, PSTR("ls")) == 0)
            {
                /* print directory listing */
                struct fat_dir_entry_struct dir_entry;
                while(fat_read_dir(dd, &dir_entry))
                {
                    uint8_t spaces = sizeof(dir_entry.long_name) - strlen(dir_entry.long_name) + 4;

                    uart_puts(dir_entry.long_name);
                    uart_putc(dir_entry.attributes & FAT_ATTRIB_DIR ? '/' : ' ');
                    while(spaces--)
                        uart_putc(' ');
                    uart_putdw_dec(dir_entry.file_size);
                    uart_putc('\n');
                }
            }
            else if(strncmp_P(command, PSTR("cat "), 4) == 0)
            {
                command += 4;
                if(command[0] == '\0')
                    continue;
                
                /* search file in current directory and open it */
                struct fat_file_struct* fd = open_file_in_dir(fs, dd, command);
                if(!fd)
                {
                    uart_puts_p(PSTR("error opening "));
                    uart_puts(command);
                    uart_putc('\n');
                    continue;
                }

                /* print file contents */
                uint8_t buffer[8];
                uint8_t size;
                uint32_t offset = 0;
                while((size = fat_read_file(fd, buffer, sizeof(buffer))) > 0)
                {
                    uart_putdw_hex(offset);
                    uart_putc(':');
                    for(uint8_t i = 0; i < 8; ++i)
                    {
                        uart_putc(' ');
                        uart_putc_hex(buffer[i]);
                    }
                    
                    uart_putc(' ');
                    
                    /* Display printable characters */
                    for(uint8_t i = 0; i < size; ++i)
                    {
                        if ((buffer[i] >= 32) && (buffer[i] <= 126))
                            uart_putc(buffer[i]);
                        else
                            uart_putc('.');
                    }
                    
                    uart_putc('\n');
                    offset += 8;
                }

                fat_close_file(fd);
            }
            else if(strcmp_P(command, PSTR("disk")) == 0)
            {
                if(!print_disk_info(fs))
                    uart_puts_p(PSTR("error reading disk info\n"));
            }
#if FAT_WRITE_SUPPORT
            else if(strncmp_P(command, PSTR("rm "), 3) == 0)
            {
                command += 3;
                if(command[0] == '\0')
                    continue;
                
                struct fat_dir_entry_struct file_entry;
                if(find_file_in_dir(fs, dd, command, &file_entry))
                {
                    if(fat_delete_file(fs, &file_entry))
                        continue;
                }

                uart_puts_p(PSTR("error deleting file: "));
                uart_puts(command);
                uart_putc('\n');
            }
            else if(strncmp_P(command, PSTR("touch "), 6) == 0)
            {
                command += 6;
                if(command[0] == '\0')
                    continue;

                struct fat_dir_entry_struct file_entry;
                if(!fat_create_file(dd, command, &file_entry))
                {
                    uart_puts_p(PSTR("error creating file: "));
                    uart_puts(command);
                    uart_putc('\n');
                }
            }
            else if(strncmp_P(command, PSTR("write "), 6) == 0)
            {
                command += 6;
                if(command[0] == '\0')
                    continue;

                char* offset_value = command;
                while(*offset_value != ' ' && *offset_value != '\0')
                    ++offset_value;

                if(*offset_value == ' ')
                    *offset_value++ = '\0';
                else
                    continue;

                /* search file in current directory and open it */
                struct fat_file_struct* fd = open_file_in_dir(fs, dd, command);
                if(!fd)
                {
                    uart_puts_p(PSTR("error opening "));
                    uart_puts(command);
                    uart_putc('\n');
                    continue;
                }

                int32_t offset = strtolong(offset_value);
                if(!fat_seek_file(fd, &offset, FAT_SEEK_SET))
                {
                    uart_puts_p(PSTR("error seeking on "));
                    uart_puts(command);
                    uart_putc('\n');

                    fat_close_file(fd);
                    continue;
                }

                /* read text from the shell and write it to the file */
                uint8_t data_len;
                while(1)
                {
                    /* give a different prompt */
                    uart_putc('<');
                    uart_putc(' ');

                    /* read one line of text */
                    data_len = read_line(buffer, sizeof(buffer));
                    if(!data_len)
                        break;

                    /* write text to file */
                    if(fat_write_file(fd, (uint8_t*) buffer, data_len) != data_len)
                    {
                        uart_puts_p(PSTR("error writing to file\n"));
                        break;
                    }
                }

                fat_close_file(fd);
            }
            else if(strncmp_P(command, PSTR("mkdir "), 6) == 0)
            {
                command += 6;
                if(command[0] == '\0')
                    continue;

                struct fat_dir_entry_struct dir_entry;
                if(!fat_create_dir(dd, command, &dir_entry))
                {
                    uart_puts_p(PSTR("error creating directory: "));
                    uart_puts(command);
                    uart_putc('\n');
                }
            }
#endif
#if SD_RAW_WRITE_BUFFERING
            else if(strcmp_P(command, PSTR("sync")) == 0)
            {
                if(!sd_raw_sync())
                    uart_puts_p(PSTR("error syncing disk\n"));
            }
#endif
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
            else if(strncmp_P(command, PSTR("play"), 4) == 0)
            {
                play(0, 64);
            }
            else
            {
                uart_puts_p(PSTR("unknown command: "));
                uart_puts(command);
                uart_putc('\n');
            }
        }

        /* close file system */
        fat_close(fs);

        /* close partition */
        partition_close(partition);
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

uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry)
{
    while(fat_read_dir(dd, dir_entry))
    {
        if(strcmp(dir_entry->long_name, name) == 0)
        {
            fat_reset_dir(dd);
            return 1;
        }
    }

    return 0;
}

struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name)
{
    struct fat_dir_entry_struct file_entry;
    if(!find_file_in_dir(fs, dd, name, &file_entry))
        return 0;

    return fat_open_file(fs, &file_entry);
}

uint8_t print_disk_info(const struct fat_fs_struct* fs)
{
    if(!fs)
        return 0;

    struct sd_raw_info disk_info;
    if(!sd_raw_get_info(&disk_info))
        return 0;

    uart_puts_p(PSTR("manuf:  0x")); uart_putc_hex(disk_info.manufacturer); uart_putc('\n');
    uart_puts_p(PSTR("oem:    ")); uart_puts((char*) disk_info.oem); uart_putc('\n');
    uart_puts_p(PSTR("prod:   ")); uart_puts((char*) disk_info.product); uart_putc('\n');
    uart_puts_p(PSTR("rev:    ")); uart_putc_hex(disk_info.revision); uart_putc('\n');
    uart_puts_p(PSTR("serial: 0x")); uart_putdw_hex(disk_info.serial); uart_putc('\n');
    uart_puts_p(PSTR("date:   ")); uart_putw_dec(disk_info.manufacturing_month); uart_putc('/');
                                   uart_putw_dec(disk_info.manufacturing_year); uart_putc('\n');
    uart_puts_p(PSTR("size:   ")); uart_putdw_dec(disk_info.capacity / 1024 / 1024); uart_puts_p(PSTR("MB\n"));
    uart_puts_p(PSTR("copy:   ")); uart_putw_dec(disk_info.flag_copy); uart_putc('\n');
    uart_puts_p(PSTR("wr.pr.: ")); uart_putw_dec(disk_info.flag_write_protect_temp); uart_putc('/');
                                   uart_putw_dec(disk_info.flag_write_protect); uart_putc('\n');
    uart_puts_p(PSTR("format: ")); uart_putw_dec(disk_info.format); uart_putc('\n');
    uart_puts_p(PSTR("free:   ")); uart_putdw_dec(fat_get_fs_free(fs)); uart_putc('/');
                                   uart_putdw_dec(fat_get_fs_size(fs)); uart_putc('\n');

    return 1;
}

#if FAT_DATETIME_SUPPORT
void get_datetime(uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* min, uint8_t* sec)
{
    *year = 2007;
    *month = 1;
    *day = 1;
    *hour = 0;
    *min = 0;
    *sec = 0;
}
#endif


