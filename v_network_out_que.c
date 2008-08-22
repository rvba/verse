/*
 * $Id$ 
 *
 * ***** BEGIN BSD LICENSE BLOCK *****
 *
 * Copyright (c) 2005-2008, The Uni-Verse Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ***** END BSD LICENSE BLOCK *****
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "verse_header.h"

#include "v_cmd_buf.h"
#include "v_cmd_gen.h"
#include "v_connection.h"
#include "v_network.h"
#include "v_pack.h"
#include "v_network_out_que.h"
#include "v_util.h"

#if !defined(V_GENERATE_FUNC_MODE)

#define STD_QUE_SIZE		64

#define V_NOQ_OPTIMIZATION_SLOTS	2048

#define V_NOQ_WINDOW_SIZE			100000
#define V_NOQ_MAX_SORTED_COMMANDS	5000

typedef struct{
	void	*next;
	char	*data;
	size_t	size;
} NetPacked;

struct VNetOutQueue{
	uint8			packet_buffer[V_NOQ_MAX_PACKET_SIZE];
	size_t			packet_buffer_use;
	NetPacked		*packed;
	NetPacked		*last;
	VCMDBufHead		*unsent[V_NOQ_OPTIMIZATION_SLOTS];
	VCMDBufHead		*history[V_NOQ_OPTIMIZATION_SLOTS];
	VCMDBufHead		*ack_nak;
	VCMDBufHead		*unsorted;
	VCMDBufHead		*unsorted_end;
	uint32			unsorted_count; /* debug only */
	uint32			unsent_comands;
	size_t			unsent_size;
	size_t			sent_size;
	unsigned int	packet_id;
	unsigned int	slot;
	uint32			seconds;
	uint32			fractions;
};

size_t verse_session_get_size(void)
{
	const VNetOutQueue *queue;

	queue = v_con_get_network_queue();
	return queue->unsent_size + queue->sent_size;
}

VNetOutQueue * v_noq_create_network_queue(void)
{
	VNetOutQueue *queue;
	unsigned int i;

	queue = malloc(sizeof *queue);
	for(i = 0; i < V_NOQ_OPTIMIZATION_SLOTS; i++)
	{
		queue->unsent[i] = NULL;
		queue->history[i] = NULL;
	}
	queue->unsent_comands = 0;
	queue->unsent_size = 0;
	queue->sent_size = 0;
	queue->packet_id = 2;
	queue->slot = 0;
	queue->packed = NULL;
	queue->last = NULL;
	queue->ack_nak = NULL;
	queue->unsorted = NULL;
	queue->unsorted_end = NULL;
	queue->unsorted_count = 0;
	queue->packet_buffer_use = 0;
	v_n_get_current_time(&queue->seconds, &queue->fractions);
	return queue;
}

unsigned int v_noq_get_next_out_packet_id(VNetOutQueue *queue)
{
	queue->packet_id++;
	if(queue->packet_id == 0)
		queue->packet_id++;
	return queue->packet_id;
}

void v_noq_destroy_network_queue(VNetOutQueue *queue)
{
	VCMDBufHead *buf, *b;
	unsigned int i;
	for(i = 0; i < V_NOQ_OPTIMIZATION_SLOTS; i++)
	{
		for(buf = queue->history[i]; buf != NULL; buf = b)
		{
			b = buf->next;
			v_cmd_buf_free(buf);
		}
		for(buf = queue->unsent[i]; buf != NULL; buf = b)
		{
			b = buf->next;
			v_cmd_buf_free(buf);
		}
	}
	for(buf = queue->unsorted; buf != NULL; buf = b)
	{
		b = buf->next;
		v_cmd_buf_free(buf);
	}
	free(queue);
}


void v_noq_sort_and_collapse_buf(VNetOutQueue *queue, VCMDBufHead *buf)
{
	VCMDBufHead *b, *last = NULL;
	unsigned int slot;

	slot = buf->address_sum % V_NOQ_OPTIMIZATION_SLOTS;
	queue->unsent_size += buf->size;
	queue->unsent_comands++;
	if(queue->unsent[slot] != NULL)
	{
		for(b = queue->unsent[slot]; !v_cmd_buf_compare(buf, b) && b->next != NULL; b = b->next)
			last = b;
		if(v_cmd_buf_compare(buf, b)) /* found a command to replace */
		{
			queue->unsent_size -= b->size;
			queue->unsent_comands--;
			if(last != NULL) /* if its not the first */
				last->next = buf;
			else
				queue->unsent[slot] = buf;
			buf->next = b->next;
			v_cmd_buf_free(b);
		}else /* inserting the command last in queue */
		{
			buf->next = NULL;
			b->next = buf;
		}
	}else /* inserting the first command */
	{
		queue->unsent[slot] = buf;
		buf->next = NULL;
	}
	if(queue->history[slot] != NULL) /* if there is a history clear it from any commnds with same address */
	{
		last = NULL;
		for(b = queue->history[slot]; b != NULL && !v_cmd_buf_compare(buf, b); b = b->next)
			last = b;
		if(b != NULL) /* found a command to replace */
		{
			if(last == NULL)
				queue->history[slot] = b->next;
			else
				last->next = b->next;
			queue->sent_size -= b->size;
			v_cmd_buf_free(b);
		}
	}
}

void v_noq_send_buf(VNetOutQueue *queue, VCMDBufHead *buf)
{
	static int count = 0;

	if(queue->unsorted == NULL)
	{
		queue->unsorted_end = buf;
		queue->unsorted = buf;
	} else
	{
		queue->unsorted_end->next = buf;
		queue->unsorted_end = buf;
	}
	
	queue->unsorted_count++;
	count = (count + 1) % 30; /* Magix number again!!! */

	if(count == 0)
	{
		v_con_network_listen();
		v_noq_send_queue(queue, v_con_get_network_address());
	}
}

void v_noq_sort_unsorted(VNetOutQueue *queue)
{
	VCMDBufHead *buf;

	while(queue->unsent_comands < V_NOQ_MAX_SORTED_COMMANDS && queue->unsorted != NULL)
	{
		buf = queue->unsorted;
		if(queue->unsorted == queue->unsorted_end)
		{
			queue->unsorted_end = NULL;
			queue->unsorted = NULL;
		}else
		{
			queue->unsorted = buf->next;
			buf->next = NULL;
		}
		queue->unsorted_count--;
		v_noq_sort_and_collapse_buf(queue, buf);
	}
}

/* Pack some data from sending queue to packet and send it. */
boolean v_noq_send_queue(VNetOutQueue *queue, void *address)
{
	VCMDBufHead *buf;
	unsigned int size;
	uint8 *data;
	uint32 seconds, fractions;
	double delta;
	int i;
	
	data = queue->packet_buffer;
	v_n_get_current_time(&seconds, &fractions);

	/* Delta time of previous sending from this queue */
	delta = seconds - queue->seconds + (fractions - queue->fractions) / (double) 0xffffffff;

	/* Sort commands in sending queue (how?). */
	if(queue->unsorted != NULL)	
		v_noq_sort_unsorted(queue);

	/* No data to send? */
	if(queue->unsent_size == 0 && delta < 1.0 && (queue->ack_nak == NULL || queue->ack_nak->next == NULL))
		return FALSE;

	/* Send data from packet buffer. */
	if(delta > 3.0 && queue->unsent_size == 0 && queue->ack_nak == NULL && queue->packet_buffer_use != 0)
	{
		v_n_send_data(address, data, queue->packet_buffer_use);
		queue->seconds = seconds;
		queue->fractions = fractions;
		return TRUE;
	}

	/* Packet ID is at beginning of packet */
	vnp_raw_pack_uint32(data, queue->packet_id);
	size = 4;	/* 4 is size of unit32 (packet_id) */
	buf = queue->ack_nak;
	while(buf != NULL && size + buf->size < V_NOQ_MAX_PACKET_SIZE)
	{
		queue->ack_nak = buf->next;
		buf->next = queue->history[queue->slot];
		queue->history[queue->slot] = buf;
		buf->packet = queue->packet_id;
		for(i=0; i < buf->size; i++) data[size+i] = ((VCMDBuffer1500 *)buf)->buf[i];
		size += buf->size;
		queue->sent_size += buf->size;
		buf = queue->ack_nak;
	}

	/* Send data, when we can't send more. */
	if(queue->unsent_size == 0 || queue->sent_size >= V_NOQ_WINDOW_SIZE)
	{
		if(size > 5)
		{
			v_n_send_data(address, data, size);
			queue->packet_buffer_use = size;
			queue->seconds = seconds;
			queue->fractions = fractions;
			queue->packet_id++;
			return TRUE;
		}
		return FALSE;
	}
	
	/* Try to add something more to this packet. */
	while(queue->unsent_size != 0)
	{
		queue->slot = ((1 + queue->slot) % V_NOQ_OPTIMIZATION_SLOTS);
		buf = queue->unsent[queue->slot];
		if(buf != NULL)
		{
			if(buf->size + size > V_NOQ_MAX_PACKET_SIZE)
				break;
			queue->unsent[queue->slot] = buf->next;
			buf->next = queue->history[queue->slot];
			queue->history[queue->slot] = buf;
			buf->packet = queue->packet_id;
			for(i=0; i < buf->size; i++) data[size+i] = ((VCMDBuffer1500 *)buf)->buf[i];
			size += buf->size;
			queue->unsent_comands--;
			queue->unsent_size -= buf->size;
			queue->sent_size += buf->size;
		}
	}

	v_n_send_data(address, data, size);
	queue->packet_buffer_use = size;
	queue->packet_id++;
	queue->seconds = seconds;
	queue->fractions = fractions;

	return TRUE;
}

void v_noq_send_ack_nak_buf(VNetOutQueue *queue, VCMDBufHead *buf)
{
	buf->next = queue->ack_nak;
	queue->ack_nak = buf;
}

void callback_send_packet_ack(void *user, uint32 packet_id)
{
	VNetOutQueue *queue;
	VCMDBufHead *buf, *last;
	unsigned int slot;

	queue = v_con_get_network_queue();
	for(slot = 0; slot < V_NOQ_OPTIMIZATION_SLOTS; slot++)
	{
		last = NULL;
		for(buf = queue->history[slot]; buf != NULL && buf->packet != packet_id; buf = buf->next)
			last = buf;

		if(buf != NULL)
		{
			if(last == NULL)
			{
				while(queue->history[slot] != NULL && queue->history[slot]->packet == packet_id)
				{
					queue->sent_size -= queue->history[slot]->size;
					buf = queue->history[slot]->next;
					v_cmd_buf_free(queue->history[slot]);
					queue->history[slot] = buf;
				}
			}else
			{
				for(; buf != NULL && buf->packet == packet_id; buf = last->next)
				{
					queue->sent_size -= buf->size;
					last->next = buf->next;
					v_cmd_buf_free(buf);
				}
			}
		}
	}
}

void callback_send_packet_nak(void *user, uint32 packet_id)
{
	VNetOutQueue *queue;
	VCMDBufHead *buf, *last;
	unsigned int slot;

	queue = v_con_get_network_queue();
	for(slot = 0; slot < V_NOQ_OPTIMIZATION_SLOTS; slot++)
	{
		last = NULL;
		for(buf = queue->history[slot]; buf != NULL && buf->packet != packet_id; buf = buf->next)
			last = buf;
		if(buf != NULL)
		{
			if(last == NULL)
			{
				for(; queue->history[slot] != NULL && queue->history[slot]->packet == packet_id; queue->history[slot] = buf)
				{
					queue->unsent_comands++;
					queue->unsent_size += queue->history[slot]->size;
					queue->sent_size -= queue->history[slot]->size;
					buf = queue->history[slot]->next;
					queue->history[slot]->next = queue->unsent[slot];
					queue->unsent[slot] = queue->history[slot];
				}
			}else
			{
				for(; last->next != NULL && ((VCMDBufHead *)last->next)->packet == packet_id;)
				{
					queue->unsent_comands++;
					queue->unsent_size += ((VCMDBufHead *)last->next)->size;
					queue->sent_size -= ((VCMDBufHead *)last->next)->size;
					buf = last->next;
					last->next = buf->next;
					buf->next = queue->unsent[slot];
					queue->unsent[slot] = buf;
				}
			}
		}
	}
}

#endif
