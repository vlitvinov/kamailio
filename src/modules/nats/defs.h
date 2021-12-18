/*
 * NATS module interface
 *
 * Copyright (C) 2021 Voxcom Inc
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */

#ifndef __NATS_DEFS_H_
#define __NATS_DEFS_H_

#include <nats/nats.h>

#define NATS_DEFAULT_URL "nats://localhost:4222"
#define NATS_MAX_SERVERS 10
#define NATS_URL_MAX_SIZE 256

typedef struct _nats_connection
{
	natsOptions *opts;
	char *servers[NATS_MAX_SERVERS];
} nats_connection, *nats_connection_ptr;

#endif
