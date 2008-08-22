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

#include "v_network.h"

typedef struct{
	char	name[16];
	char	pass[16];
	uint8	key;
	VNodeID	avatar;
}VSConnectionID;

typedef enum{
	V_CS_IDLE = 0, /* Host connection waiting for connection */
	V_CS_CONTACT = 1, /* client tryes to contact host */
	V_CS_CONTACTED = 2, /* Host replyes whit challange */
	V_CS_PENDING_ACCEPT = 3, /* Client sends login */
	V_CS_PENDING_HOST_CALLBACK = 4, /* Host got login waits for accept connect callback */
	V_CS_PENDING_CLIENT_CALLBACK_ACCEPT = 5, /* Host got login waits for accept connect callback */
	V_CS_PENDING_CLIENT_CALLBACK_TERMINATE = 6, /* Host got login waits for connect terminate callback */
	V_CS_PENDING_DECISION = 7, /* Host got has executed Callback waits for accept command */
	V_CS_CONNECTED = 8 /* Connection establiched */
}VConnectStage;

/* Connection related functions (v_connection.c) */

extern boolean				v_con_network_listen(void);

extern void				v_con_set_name_pass(const char *name, const char *pass);
extern const char *			v_con_get_name(void);
extern const char *			v_con_get_pass(void);

extern void				v_con_set_avatar(uint32 avatar);
extern void				v_con_set_time(uint32 seconds, uint32 fractions);

extern void				v_con_set_connect_stage(VConnectStage stage);
extern VConnectStage	v_con_get_connect_stage(void);

extern void *			v_con_get_network_queue(void);
extern VNetworkAddress *v_con_get_network_address(void);
extern void *			v_con_get_network_address_id(unsigned int id);
extern unsigned int *	v_con_get_network_expected_packet(void);
extern void *			v_con_get_ordered_storage(void);
extern void *			v_con_get_func_storage(void);
extern void *			v_con_connect(VNetworkAddress *address, VConnectStage stage);
extern unsigned int		v_con_get_network_address_count(void);

extern boolean			v_co_switch_connection(VNetworkAddress *address);

extern void			v_con_inqueue_timer_update(void);


/* Func storage related functions (v_func_storage.c)*/
extern void				v_fs_unpack(uint8 *data, unsigned int length);

extern boolean			v_fs_func_accept_connections(void);
extern void				v_fs_add_func(unsigned int cmd_id, unsigned int (*unpack_func)(const char *buf, size_t buffer_length), void *pack_func, void *alias_func);

extern void *			v_fs_get_alias_user_func(unsigned int cmd_id);
extern void *			v_fs_get_alias_user_data(unsigned int cmd_id);
extern void *			v_fs_get_user_func(unsigned int cmd_id);
extern void *			v_fs_get_user_data(unsigned int cmd_id);
