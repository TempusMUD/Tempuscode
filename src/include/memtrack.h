#ifndef _MEMTRACK_H_
#define _MEMTRACK_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <sys/resource.h>
#include "utils.h"

#define _DBG_MEM_BLKS	64000

enum dbg_mem_blk_status { dbg_allocated, dbg_alloc_failed, dbg_deallocated,
	dbg_bad_dealloc, dbg_bad_realloc, dbg_corrupted
};

struct dbg_mem_blk {
	unsigned long magic;
	struct dbg_mem_blk *next, *prev;
	unsigned long long serial_num;
	size_t size;
	const void *alloc_addr;
	enum dbg_mem_blk_status status;
	unsigned char data[0];	// four extra bytes on end of block
};

void dbg_enable_tracking(bool dump_on_exit);
void dbg_disable_tracking(void);
void dbg_dump(void);
void dbg_check_now(char *str, bool abort_now);
dbg_mem_blk *dbg_get_block(void *ptr);

#endif
