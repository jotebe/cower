/*
 *  conf.h
 *
 *  Copyright (c) 2006-2009 Pacman Development Team <pacman-dev@archlinux.org>
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _COWER_CONF_H
#define _COWER_CONF_H

typedef struct __config_t {

  /* operations */
  int op;

  /* options */
  unsigned short color;
  unsigned short force;
  unsigned short verbose;
  unsigned short quiet;
  const char *download_dir;
} config_t;

enum {
  OP_SEARCH = 1,
  OP_INFO = 2,
  OP_DL = 4,
  OP_UPDATE = 8
};

/* global config variable */
extern config_t *config;

config_t *config_new(void);
int config_free(config_t *oldconfig);

#endif /* _COWER_CONF_H */
