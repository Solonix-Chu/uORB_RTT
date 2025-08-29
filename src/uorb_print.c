/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include "uORB.h"
#include <rtthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

const char *orb_get_c_type(unsigned char short_type)
{
	RT_UNUSED(short_type);
	return RT_NULL;
}

void orb_print_message_internal(const struct orb_metadata *meta, const void *data, bool print_topic_name)
{
	if (!meta || !data)
	{
		return;
	}

	const uint8_t *bytes = (const uint8_t *)data;
	int size = (int)meta->o_size;

	if (print_topic_name)
	{
		rt_kprintf("%s: size=%d\n", meta->o_name, size);
	}
	else
	{
		rt_kprintf("size=%d\n", size);
	}

	/* 十六进制转储（每行16字节） */
	for (int i = 0; i < size; i += 16)
	{
		rt_kprintf("  %03x: ", i);
		int line_end = (i + 16 < size) ? (i + 16) : size;
		for (int j = i; j < line_end; j++)
		{
			rt_kprintf("%02x ", bytes[j]);
		}
		rt_kprintf("\n");
	}
} 