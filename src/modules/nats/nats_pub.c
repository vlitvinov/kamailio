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

#include "defs.h"
#include "nats_pub.h"

extern int *nats_pub_worker_pipes;

int set_non_blocking(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if(flags < 0)
		return flags;
	flags |= O_NONBLOCK;
	if(fcntl(fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}

int fixup_publish_get_value(void **param, int param_no)
{
	if(param_no == 1 || param_no == 2) {
		return fixup_spve_null(param, 1);
	}
	LM_ERR("invalid parameter number <%d>\n", param_no);
	return -1;
}

int fixup_publish_get_value_free(void **param, int param_no)
{
	if(param_no == 1 || param_no == 2) {
		fixup_free_spve_null(param, 1);
		return 0;
	}
	LM_ERR("invalid parameter number <%d>\n", param_no);
	return -1;
}

nats_pub_delivery_ptr _nats_pub_delivery_new(str subject, str payload)
{
	nats_pub_delivery_ptr p =
			(nats_pub_delivery_ptr)shm_malloc(sizeof(nats_pub_delivery));
	memset(p, 0, sizeof(nats_pub_delivery));

	p->subject = shm_malloc(subject.len + 1);
	strcpy(p->subject, subject.s);
	p->subject[subject.len] = '\0';

	p->payload = shm_malloc(payload.len + 1);
	strcpy(p->payload, payload.s);
	p->payload[payload.len] = '\0';

	return p;
}

static int _w_nats_publish_f(str subj, str payload)
{
	nats_pub_delivery_ptr ptr = _nats_pub_delivery_new(subj, payload);

	if(write(nats_pub_worker_pipes[0], &ptr, sizeof(ptr)) != sizeof(ptr)) {
		LM_ERR("failed to publish message %d, write to "
			   "command pipe: %s\n",
				getpid(), strerror(errno));
	}

	return 1;
}

int w_nats_publish_f(sip_msg_t *msg, char *subj, char *payload)
{
	str subj_s = STR_NULL;
	str payload_s = STR_NULL;
	if(fixup_get_svalue(msg, (gparam_t *)subj, &subj_s) < 0) {
		LM_ERR("failed to get subj value\n");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_t *)payload, &payload_s) < 0) {
		LM_ERR("failed to get subj value\n");
		return -1;
	}
	return _w_nats_publish_f(subj_s, payload_s);
}

void _nats_pub_worker_cb(int fd, short event, void *arg)
{
	natsStatus s = NATS_OK;
	nats_pub_delivery_ptr ptr;

	if(read(fd, &ptr, sizeof(ptr)) != sizeof(ptr)) {
		LM_ERR("failed to read from command pipe: %s\n", strerror(errno));
		return;
	}
	natsConnection *conn = (natsConnection *)arg;

	if((s = natsConnection_PublishString(conn, ptr->subject, ptr->payload))
			!= NATS_OK) {
		LM_ERR("could not publish to subject %s [%s]\n", ptr->subject,
				natsStatus_GetText(s));
	}

	nats_pub_free_delivery_ptr(ptr);
}

int _nats_pub_worker_proc(int cmd_pipe, nats_connection_ptr c)
{
	natsStatus s = NATS_OK;
	natsConnection *conn;

	if((s = natsConnection_Connect(&conn, c->opts)) != NATS_OK) {
		LM_ERR("could not connect to nats servers [%s]\n",
				natsStatus_GetText(s));
	}

	event_init();
	struct event pipe_ev;
	set_non_blocking(cmd_pipe);
	event_set(&pipe_ev, cmd_pipe, EV_READ | EV_PERSIST, _nats_pub_worker_cb,
			conn);

	event_add(&pipe_ev, NULL);
	return event_dispatch();
}

void nats_pub_free_delivery_ptr(nats_pub_delivery_ptr ptr)
{
	if(ptr == NULL)
		return;
	if(ptr->subject)
		shm_free(ptr->subject);
	if(ptr->payload)
		shm_free(ptr->payload);
	shm_free(ptr);
}
