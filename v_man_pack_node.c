/*
**
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "v_cmd_gen.h"

#if !defined V_GENERATE_FUNC_MODE

#include "verse.h"
#include "v_cmd_buf.h"
#include "v_network_out_que.h"
#include "v_network.h"
#include "v_connection.h"
#include "v_encryption.h"

unsigned int v_unpack_connect(const char *buf, unsigned int buffer_length)
{
	return -1; /* this command is illegal to send */
}

unsigned int v_unpack_connect_accept(const char *buf, unsigned int buffer_length)
{
	return -1; /* this command is illegal to send */
}

extern void v_callback_connect_terminate(const char *bye);

unsigned int v_unpack_connect_terminate(const char *buf, unsigned int buffer_length)
{
	unsigned int buffer_pos = 0;
	char bye[512];
	buffer_pos = vnp_raw_unpack_string(buf, bye, sizeof bye, buffer_length);
	v_callback_connect_terminate(bye);
	return buffer_pos;
}

typedef struct VTempLine	VTempLine;

struct VTempLine {
	VNodeID		node_id;
	VNMBufferID buffer_id;
	uint32		pos;
	uint32		length;
	uint16		index; 
	char		*text;
	VTempLine	*next;
};

typedef struct{
	VTempLine	*text_temp;
	uint16		text_send_id;
	uint16		text_receive_id;
}VOrderedStorage;

VOrderedStorage *v_create_ordered_storage(void)
{
	VOrderedStorage *s;
	s = malloc(sizeof *s);
	s->text_temp = NULL;
	s->text_send_id = 0;
	s->text_receive_id = 0;
	return s;
}

void v_destroy_ordered_storage(VOrderedStorage *s)
{
	VTempLine *line, *next;
	line = s->text_temp;
	while(line != NULL)
	{
		next = line->next;
		if(line->text != NULL)
			free(line->text);
		free(line);
		line = next;
	}
	free(s);
}


void verse_send_t_text_set(VNodeID node_id, VNMBufferID buffer_id, uint32 pos, uint32 length, const char *text)
{
	uint8 *buf;
	VOrderedStorage *s;
	unsigned int buffer_pos = 0;
	VCMDBufHead *head;
	head = v_cmd_buf_allocate(VCMDBS_1500);/* Allocating the buffer */
	buf = ((VCMDBuffer10 *)head)->buf;

	buffer_pos += vnp_raw_pack_uint8(&buf[buffer_pos], 99);/* Packing the command */
#if defined V_PRINT_SEND_COMMANDS
	printf("send: verse_send_t_line_insert(node_id = %u buffer_id = %u pos = %u length = %u text = %s );\n", node_id, buffer_id, pos, length, text);
#endif
	s = v_con_get_ordered_storage();
	buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], node_id);
	buffer_pos += vnp_raw_pack_uint16(&buf[buffer_pos], buffer_id);
	buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], pos);
	buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], length);	
	buffer_pos += vnp_raw_pack_uint16(&buf[buffer_pos], s->text_send_id++);
	if(text == NULL)
		buffer_pos += vnp_raw_pack_uint8(&buf[buffer_pos], 0);
	else
		buffer_pos += vnp_raw_pack_string(&buf[buffer_pos], text, VN_T_MAX_TEXT_CMD_SIZE);
	v_cmd_buf_set_unique_address_size(head, buffer_pos);
	v_cmd_buf_set_size(head, buffer_pos);
	v_noq_send_buf(v_con_get_network_queue(), head);
}

void v_call_line(VTempLine *line)
{
	char *t;
	void (* func_t_line_insert)(void *user_data, VNodeID node_id, VNMBufferID buffer_id, uint32 pos, uint16 length, char *text);
	func_t_line_insert = v_fs_get_user_func(99);
	#if defined V_PRINT_RECEIVE_COMMANDS
		printf("receive: verse_send_t_line_insert(node_id = %u buffer_id = %u pos = %u length = %u text = %s ); callback = %p\n", line->node_id, line->buffer_id, line->pos, line->length, line->text, v_fs_get_user_func(99));
	#endif
	if(line->text == NULL)
		t = "";
	else
		t = line->text;
	if(func_t_line_insert != NULL)
		func_t_line_insert(v_fs_get_user_data(99), line->node_id, line->buffer_id, line->pos, line->length, t);
}

unsigned int v_unpack_t_text_set(const char *buf, size_t buffer_length)
{
	unsigned int i, buffer_pos = 0;
	VOrderedStorage *s;
	VTempLine l, *line, *past = NULL;
	char text[512];

	if(buffer_length < 12)
		return -1;
	buffer_pos += vnp_raw_unpack_uint32(&buf[buffer_pos], &l.node_id);
	buffer_pos += vnp_raw_unpack_uint16(&buf[buffer_pos], &l.buffer_id);
	buffer_pos += vnp_raw_unpack_uint32(&buf[buffer_pos], &l.pos);
	buffer_pos += vnp_raw_unpack_uint32(&buf[buffer_pos], &l.length);	
	buffer_pos += vnp_raw_unpack_uint16(&buf[buffer_pos], &l.index);
	buffer_pos += vnp_raw_unpack_string(&buf[buffer_pos], text, 512, buffer_length - buffer_pos);
	if(text[0] == 0)
		l.text = NULL;
	else
		l.text = text;
	s = v_con_get_ordered_storage();
	if(s->text_receive_id == l.index)
	{
		v_call_line(&l);
		s->text_receive_id++;
		line = s->text_temp;
		while(line != NULL)
		{
			if(line->index == s->text_receive_id)
			{
				if(past == NULL)
					s->text_temp = line->next;
				else
					past->next = line->next;
				if(line->text != NULL)
					free(line->text);
				past = NULL;
				line = s->text_temp;
				s->text_receive_id++;
			}else
			{
				past = line;
				line = line->next;
			}
		}
	}
	else
	{
		line = malloc(sizeof *line);
		*line = l;
		line->next = s->text_temp;
		s->text_temp = line;
		i = strlen(text);
		if(i > 0)
		{
			line->text = malloc(i + 1);
			strcpy(line->text, text);
		}
		else
			line->text = NULL;
	}
	return buffer_pos;
}

void verse_send_c_curve_key_set(VNodeID node_id, VLayerID curve_id, uint32 key_id, uint8 dimensions, real64 *pre_value, uint32 *pre_pos, real64 *value, real64 pos, real64 *post_value, uint32 *post_pos)
{
	uint8 *buf;
	unsigned int i, buffer_pos = 0;
	VCMDBufHead *head;
	head = v_cmd_buf_allocate(VCMDBS_1500);/* Allocating the buffer */
	buf = ((VCMDBuffer10 *)head)->buf;

	if(dimensions == 0 || dimensions > 4)
		return;
	buffer_pos += vnp_raw_pack_uint8(&buf[buffer_pos], 130);/* Packing the command */
#if defined V_PRINT_SEND_COMMANDS
	switch(dimensions)
	{
	case 1:
		printf("receive: verse_send_c_curve_key_set(node_id = %u curve_id = %u key_id = %u dimensions = %u pre_value = %f pre_pos = %u value = %f pos = %f, pre_value = %f pre_pos = %u ); callback = %p\n", node_id, curve_id, key_id, dimensions, pre_value[0], pre_pos[0], value[0], pos, pre_value[0], pre_pos[0], v_fs_get_user_func(130));
	break;
	case 2:
		printf("receive: verse_send_c_curve_key_set(node_id = %u curve_id = %u key_id = %u dimensions = %u pre_value = {%f, %f} pre_pos = {%u, %u} value = {%f, %f} pos = {%f, %f}, pre_value = {%f, %f} pre_pos = {%u, %u}); callback = %p\n",
			node_id, curve_id, key_id, dimensions, 
			pre_value[0], pre_value[1], 
			pre_pos[0], pre_pos[1], 
			value[0], value[1], pos, 
			pre_value[0], pre_value[1],  
			pre_pos[0], pre_pos[1], v_fs_get_user_func(130));
	break;
	case 3:
		printf("receive: verse_send_c_curve_key_set(node_id = %u curve_id = %u key_id = %u dimensions = %u pre_value = {%f, %f, %f} pre_pos = {%u, %u, %u} value = {%f, %f, %f} pos = {%f, %f, %f}, pre_value = {%f, %f, %f} pre_pos = {%u, %u, %u}); callback = %p\n",
			node_id, curve_id, key_id, dimensions, 
			pre_value[0], pre_value[1], pre_value[2],  
			pre_pos[0], pre_pos[1], pre_pos[2],  
			value[0], value[1], value[2], pos, 
			pre_value[0], pre_value[1], pre_value[2],  
			pre_pos[0], pre_pos[1], pre_pos[2], v_fs_get_user_func(130));
		
	break;
	case 4:
		printf("receive: verse_send_c_curve_key_set(node_id = %u curve_id = %u key_id = %u dimensions = %u pre_value = {%f, %f, %f, %f} pre_pos = {%u, %u, %u, %u} value = {%f, %f, %f, %f} pos = {%f, %f, %f, %f}, pre_value = {%f, %f, %f, %f} pre_pos = {%u, %u, %u, %u}); callback = %p\n",
			node_id, curve_id, key_id, dimensions, 
			pre_value[0], pre_value[1], pre_value[2], pre_value[3], 
			pre_pos[0], pre_pos[1], pre_pos[2], pre_pos[3], 
			value[0], value[1], value[2], value[3], pos, 
			pre_value[0], pre_value[1], pre_value[2], pre_value[3], 
			pre_pos[0], pre_pos[1], pre_pos[2], pre_pos[3], v_fs_get_user_func(130));		
	break;
	}
#endif
	buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], node_id);
	buffer_pos += vnp_raw_pack_uint16(&buf[buffer_pos], curve_id);
	buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], key_id);
	buffer_pos += vnp_raw_pack_uint8(&buf[buffer_pos], dimensions);

	for(i = 0; i < dimensions; i++)
		buffer_pos += vnp_raw_pack_real64(&buf[buffer_pos], pre_value[i]);	
	for(i = 0; i < dimensions; i++)
		buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], pre_pos[i]);	
	for(i = 0; i < dimensions; i++)
		buffer_pos += vnp_raw_pack_real64(&buf[buffer_pos], value[i]);	
	buffer_pos += vnp_raw_pack_real64(&buf[buffer_pos], pos);	
	for(i = 0; i < dimensions; i++)
		buffer_pos += vnp_raw_pack_real64(&buf[buffer_pos], post_value[i]);	
	for(i = 0; i < dimensions; i++)
		buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], post_pos[i]);	

	if(key_id == (uint32)(-1))
		v_cmd_buf_set_unique_address_size(head, 11);
	else
		v_cmd_buf_set_address_size(head, 11);
	v_cmd_buf_set_size(head, buffer_pos);
	v_noq_send_buf(v_con_get_network_queue(), head);
}

void verse_send_c_curve_key_destroy(VNodeID node_id, VLayerID curve_id, uint32 key_id)
{
	uint8 *buf;
	unsigned int buffer_pos = 0;
	VCMDBufHead *head;
	head = v_cmd_buf_allocate(VCMDBS_1500);/* Allocating the buffer */
	buf = ((VCMDBuffer10 *)head)->buf;

	buffer_pos += vnp_raw_pack_uint8(&buf[buffer_pos], 130);/* Packing the command */
#if defined V_PRINT_SEND_COMMANDS
	printf("send: verse_send_c_curve_key_destroy(node_id = %u curve_id = %u key_id = %u );\n", node_id, curve_id, key_id);
#endif
	buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], node_id);
	buffer_pos += vnp_raw_pack_uint16(&buf[buffer_pos], curve_id);
	buffer_pos += vnp_raw_pack_uint32(&buf[buffer_pos], key_id);
	buffer_pos += vnp_raw_pack_uint8(&buf[buffer_pos], 0);
	v_cmd_buf_set_address_size(head, 11);
	v_cmd_buf_set_size(head, buffer_pos);
	v_noq_send_buf(v_con_get_network_queue(), head);
}

unsigned int v_unpack_c_curve_key_set(const char *buf, size_t buffer_length)
{
	unsigned int i, buffer_pos = 0;
	VNodeID node_id;
	VLayerID curve_id;
	uint32 key_id;
	uint8 dimensions;
	real64 pre_value[4], value[4], pos, post_value[4];
	uint32 post_pos[4], pre_pos[4];
	

	if(buffer_length < 10)
		return -1;
	buffer_pos += vnp_raw_unpack_uint32(&buf[buffer_pos], &node_id);
	buffer_pos += vnp_raw_unpack_uint16(&buf[buffer_pos], &curve_id);
	buffer_pos += vnp_raw_unpack_uint32(&buf[buffer_pos], &key_id);
	buffer_pos += vnp_raw_unpack_uint8(&buf[buffer_pos], &dimensions);
	if(dimensions == 0 || dimensions > 4)
	{
		void (* func_c_curve_key_set)(void *user_data, VNodeID node_id, VLayerID curve_id, uint32 key_id, uint8 dimensions, real64 *pre_value, uint32 *pre_pos, real64 *value, real64 pos, real64 *post_value, uint32 *post_pos);
		for(i = 0; i < dimensions; i++)
			buffer_pos += vnp_raw_unpack_real64(&buf[buffer_pos], &pre_value[i]);	
		for(i = 0; i < dimensions; i++)
			buffer_pos += vnp_raw_unpack_uint32(&buf[buffer_pos], &pre_pos[i]);	
		for(i = 0; i < dimensions; i++)
			buffer_pos += vnp_raw_unpack_real64(&buf[buffer_pos], &value[i]);	
		buffer_pos += vnp_raw_unpack_real64(&buf[buffer_pos], &pos);	
		for(i = 0; i < dimensions; i++)
			buffer_pos += vnp_raw_unpack_real64(&buf[buffer_pos], &post_value[i]);	
		for(i = 0; i < dimensions; i++)
			buffer_pos += vnp_raw_unpack_uint32(&buf[buffer_pos], &post_pos[i]);
	#if defined V_PRINT_RECEIVE_COMMANDS
		switch(dimensions)
		{
		case 1:
			printf("receive: verse_send_c_curve_key_set(node_id = %u curve_id = %u key_id = %u dimensions = %u pre_value = %f pre_pos = %u value = %f pos = %f, pre_value = %f pre_pos = %u ); callback = %p\n", node_id, curve_id, key_id, dimensions, pre_value[0], pre_pos[0], value[0], pos, pre_value[0], pre_pos[0], v_fs_get_user_func(130));
		break;
		case 2:
			printf("receive: verse_send_c_curve_key_set(node_id = %u curve_id = %u key_id = %u dimensions = %u pre_value = {%f, %f} pre_pos = {%u, %u} value = {%f, %f} pos = {%f, %f}, pre_value = {%f, %f} pre_pos = {%u, %u}); callback = %p\n",
				node_id, curve_id, key_id, dimensions, 
				pre_value[0], pre_value[1], 
				pre_pos[0], pre_pos[1], 
				value[0], value[1], pos, 
				pre_value[0], pre_value[1],  
				pre_pos[0], pre_pos[1], v_fs_get_user_func(130));
		break;
		case 3:
			printf("receive: verse_send_c_curve_key_set(node_id = %u curve_id = %u key_id = %u dimensions = %u pre_value = {%f, %f, %f} pre_pos = {%u, %u, %u} value = {%f, %f, %f} pos = {%f, %f, %f}, pre_value = {%f, %f, %f} pre_pos = {%u, %u, %u}); callback = %p\n",
				node_id, curve_id, key_id, dimensions, 
				pre_value[0], pre_value[1], pre_value[2],  
				pre_pos[0], pre_pos[1], pre_pos[2],  
				value[0], value[1], value[2], pos, 
				pre_value[0], pre_value[1], pre_value[2],  
				pre_pos[0], pre_pos[1], pre_pos[2], v_fs_get_user_func(130));
			
		break;
		case 4:
			printf("receive: verse_send_c_curve_key_set(node_id = %u curve_id = %u key_id = %u dimensions = %u pre_value = {%f, %f, %f, %f} pre_pos = {%u, %u, %u, %u} value = {%f, %f, %f, %f} pos = {%f, %f, %f, %f}, pre_value = {%f, %f, %f, %f} pre_pos = {%u, %u, %u, %u}); callback = %p\n",
				node_id, curve_id, key_id, dimensions, 
				pre_value[0], pre_value[1], pre_value[2], pre_value[3], 
				pre_pos[0], pre_pos[1], pre_pos[2], pre_pos[3], 
				value[0], value[1], value[2], value[3], pos, 
				pre_value[0], pre_value[1], pre_value[2], pre_value[3], 
				pre_pos[0], pre_pos[1], pre_pos[2], pre_pos[3], v_fs_get_user_func(130));		
		break;
		}
	#endif
		func_c_curve_key_set = v_fs_get_user_func(130);
		if(func_c_curve_key_set != NULL)
			func_c_curve_key_set(v_fs_get_user_data(130), node_id, curve_id, key_id, dimensions, pre_value, pre_pos, value, pos, post_value, post_pos);
		return buffer_pos;
	}else
	{
		void (* alias_c_curve_key_destroy)(void *user_data, VNodeID node_id, VLayerID curve_id, uint32 key_id);
		alias_c_curve_key_destroy = v_fs_get_alias_user_func(130);
		if(alias_c_curve_key_destroy != NULL)
			alias_c_curve_key_destroy(v_fs_get_alias_user_data(130), node_id, curve_id, key_id);
		return buffer_pos;
	}
}

#endif
