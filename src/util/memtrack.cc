/* Memory debugging macros */
#include "memtrack.h"

const unsigned long _dbg_magic = 0xAABBCCDD;
struct dbg_mem_blk *_dbg_mem_list = NULL;
int _dbg_enabled = 0;
int _dbg_serial_num = 0;
int _dbg_total_mem = 0;

typedef void *(*malloc_hook_t)(size_t, const void *);
typedef void *(*realloc_hook_t)(void *, size_t, const void *);
typedef void (*free_hook_t)(void *, const void *);

malloc_hook_t old_malloc_hook;
realloc_hook_t old_realloc_hook;
free_hook_t old_free_hook;

void *_dbg_alloc(size_t size, const void *return_addr);
void *_dbg_realloc(void *ptr, size_t size, const void *addr);
void _dbg_free(void *ptr, const void *return_addr);

void _dbg_dump(void);

void
dbg_enable_tracking(bool dump_on_exit)
{
	if (_dbg_enabled)
		return;
	_dbg_enabled = 1;

	old_malloc_hook = __malloc_hook;
	old_realloc_hook = __realloc_hook;
	old_free_hook = __free_hook;
	__malloc_hook = _dbg_alloc;
	__realloc_hook = _dbg_realloc;
	__free_hook = _dbg_free;

	if (dump_on_exit)
		atexit(_dbg_dump);
}

void
dbg_disable_tracking(void)
{
	if (!_dbg_enabled)
		return;
	_dbg_enabled = 0;
	__malloc_hook = old_malloc_hook;
	__realloc_hook = old_realloc_hook;
	__free_hook = old_free_hook;
}

size_t
dbg_memory_used(void)
{
	return _dbg_total_mem;
}

void *
_dbg_alloc(size_t size, const void *return_addr)
{
	dbg_mem_blk *new_blk;
	
	if (_dbg_total_mem + size > 300 * 1024 * 1024)
		raise(SIGSEGV);

	__malloc_hook = old_malloc_hook;
	new_blk = (struct dbg_mem_blk *)malloc(size + 4 +
			sizeof(struct dbg_mem_blk));
	__malloc_hook = _dbg_alloc;

	if (!new_blk)
		return NULL;

	_dbg_total_mem += size;

	new_blk->magic = _dbg_magic;
	new_blk->serial_num = ++_dbg_serial_num;
	new_blk->size = size;
	new_blk->status = dbg_allocated;
	new_blk->alloc_addr = return_addr;

	*((unsigned long *)(new_blk->data + size)) = _dbg_magic;

	new_blk->prev = NULL;
	new_blk->next = _dbg_mem_list;
	if (_dbg_mem_list)
		_dbg_mem_list->prev = new_blk;
	_dbg_mem_list = new_blk;

	return new_blk->data;
}

void
_dbg_free(void *ptr, const void *return_addr)
{
	struct dbg_mem_blk *cur_blk;

	if (!ptr) {
		slog("MEMORY: Attempt to deallocate NULL ptr (%p)",
			return_addr);
		return;
	}

	cur_blk = (struct dbg_mem_blk *)((char *)ptr - sizeof(struct dbg_mem_blk));

	if (cur_blk->magic != _dbg_magic) {
		slog("MEMORY: free called on unregistered memory block at %p",
			return_addr);
		return;
	}

	if (*((unsigned long *)(cur_blk->data + cur_blk->size)) != _dbg_magic) {
		slog("MEMORY: Buffer overrun detected at (%p)\n         Block %lld was allocated at %p",
			return_addr, cur_blk->serial_num, cur_blk->alloc_addr);
		return;
	}
	
	// Remove from list of memory blocks
	if (cur_blk->prev)
		cur_blk->prev->next = cur_blk->next;
	else
		_dbg_mem_list = cur_blk->next;
	if (cur_blk->next)
		cur_blk->next->prev = cur_blk->prev;

	_dbg_total_mem -= cur_blk->size;

	__free_hook = old_free_hook;
	free(cur_blk);
	__free_hook = _dbg_free;
}

void *
_dbg_realloc(void *ptr, size_t size, const void *addr)
{
	struct dbg_mem_blk *cur_blk;
	void *new_ptr;

	if (!ptr) {
		new_ptr = _dbg_alloc(size, addr);
		return new_ptr;
	}

	if (!size) {
		_dbg_free(ptr, addr);
		return NULL;
	}

	new_ptr = _dbg_alloc(size, addr);
	cur_blk = (struct dbg_mem_blk *)((char *)ptr - sizeof(struct dbg_mem_blk));
	memcpy(new_ptr, ptr, cur_blk->size);
	_dbg_free(ptr, addr);

	return new_ptr;
}

void
_dbg_dump(void)
{
	FILE *ouf;
	struct dbg_mem_blk *cur_blk;
	unsigned long total_bytes = 0;
	unsigned int block_count = 0;

	if (!_dbg_mem_list)
		return;

	ouf = fopen("memusage.dat", "w");
	if (!ouf)
		return;

	for (cur_blk = _dbg_mem_list; cur_blk; cur_blk = cur_blk->next) {
		if (cur_blk->status == dbg_deallocated)
			continue;

		total_bytes += cur_blk->size;
		block_count++;
		if (cur_blk->status == dbg_allocated) {
			if (cur_blk->magic &&
					*((unsigned long *)(cur_blk->data + cur_blk->size)) !=
					0xAABBCCDD)
				cur_blk->status = dbg_corrupted;
		}

		fprintf(ouf, "%10llu %4i %p %p %s\n",
			cur_blk->serial_num, cur_blk->size, cur_blk->data,
			cur_blk->alloc_addr,
			(cur_blk->status == dbg_corrupted) ? "OVERFLOWED":"");
		
		/*
		if (cur_blk->status == dbg_allocated) {
			unsigned int i;

			printf("  [ ");
			for (i = 0; i < cur_blk->size; i++) {
				printf("%02x ", cur_blk->data[i]);
				if (i != cur_blk->size - 1 && !((i + 1) % 16))
					printf("]\n  [ ");
			}
			printf("]\n");
		}
		*/
	}
	if (total_bytes || block_count) {
		fprintf(ouf, "Total bytes: %lu\n", total_bytes);
		fprintf(ouf, "Block count: %u\n", block_count);
	} else
		fprintf(ouf, "No memory leaks detected.\n");
	fclose(ouf);
}

#ifdef __cplusplus
void *
operator new(size_t size)
{
		dbg_enable_tracking(false);
	if (_dbg_enabled)
		return _dbg_alloc(size, __builtin_return_address(0));
	else
		return malloc(size);
}

void *
operator new[] (size_t size) {
		dbg_enable_tracking(false);
	if (_dbg_enabled)
		return _dbg_alloc(size, __builtin_return_address(0));
	else
		return malloc(size);
}

void
operator delete(void *ptr)
{
	if (ptr) {
		if (_dbg_enabled)
			_dbg_free(ptr, __builtin_return_address(0));
		else
			free(ptr);
	}
}

void
operator delete[] (void *ptr) {
	if (ptr) {
		if (_dbg_enabled)
			_dbg_free(ptr, __builtin_return_address(0));
		else
			free(ptr);
	}
}

#endif

void
dbg_check_now(char *str, bool abort_now)
{
	struct dbg_mem_blk *cur_blk;
	unsigned long total_bytes = 0;
	unsigned int block_count = 0;

	if (!_dbg_mem_list)
		return;
	for (cur_blk = _dbg_mem_list; cur_blk; cur_blk = cur_blk->next) {
		if (cur_blk->status != dbg_allocated)
			continue;

		total_bytes += cur_blk->size;
		block_count++;
		if (cur_blk->status == dbg_allocated) {
			if (cur_blk->magic != _dbg_magic) {
				slog("MEMORY: Header corruption detected: %s (%p)",
					str, cur_blk->alloc_addr);
			}
			
			if (*((unsigned long *)
					(cur_blk->data + cur_blk->size)) != 0xAABBCCDD) {
				cur_blk->status = dbg_corrupted;
				slog("MEMORY: Footer corruption detected: %s (%p)",
					str, cur_blk->alloc_addr);
				if (abort_now)
					abort();
			}
		}
	}
}

struct dbg_mem_blk *
dbg_get_block(void *ptr)
{
	struct dbg_mem_blk *blk;
	
	blk = (struct dbg_mem_blk *)((char *)ptr - sizeof(struct dbg_mem_blk));
	if (blk->status != dbg_allocated)
		return NULL;
	
	return blk;
}
