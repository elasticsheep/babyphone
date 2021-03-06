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

#ifndef PLAYER_H
#define PLAYER_H

typedef void (*t_notify_eof)(void);

enum {
  PLAYER_OPTION_SAMPLING_RATE,
  PLAYER_OPTION_LOOP_MODE,
};

void player_init(void);
void player_set_option(uint8_t option, uint32_t value);
void player_start(uint32_t start_sector, uint16_t nb_sectors, t_notify_eof notify_eof);
void player_stop(void);

#endif /* PLAYER_H */