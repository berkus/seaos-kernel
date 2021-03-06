/* kmalloc.c: Copyright (c) 2010 Daniel Bittman
 * Defines wrapper functions for kmalloc, kfree and friends
 */
#include <kernel.h>
#include <memory.h>
#include <task.h>
unsigned (*do_kmalloc_wrap)(unsigned, char)=0;
void (*do_kfree_wrap)(void *)=0;
char kmalloc_name[128];
mutex_t km_m;
void install_kmalloc(char *name, unsigned (*init)(unsigned, unsigned), 
	unsigned (*alloc)(unsigned, char), void (*free)(void *))
{
	do_kmalloc_wrap = alloc;
	do_kfree_wrap = free;
	strncpy(kmalloc_name, name, 128);
	if(init)
		init(KMALLOC_ADDR_START, KMALLOC_ADDR_END);
	create_mutex(&km_m);
}

inline unsigned do_kmalloc(unsigned sz, char align)
{
	if(!do_kmalloc_wrap)
		panic(PANIC_MEM | PANIC_NOSYNC, "No kernel-level allocator installed!");
	mutex_on(&km_m);
	unsigned ret = do_kmalloc_wrap(sz, align);
	mutex_off(&km_m);
	if(!ret || ret >= KMALLOC_ADDR_END || ret < KMALLOC_ADDR_START)
		panic(PANIC_MEM | PANIC_NOSYNC, "kmalloc returned impossible address");
	memset((void *)ret, 0, sz);
	return ret;
}

unsigned __kmalloc(unsigned s, char *file, int line)
{
	return do_kmalloc(s, 0);
}

unsigned kmalloc_a(unsigned s)
{
	return do_kmalloc(s, 1);
}

unsigned kmalloc_p(unsigned s, unsigned *p)
{
	unsigned ret = do_kmalloc(s, 0);
	vm_getmap(ret, p);
	*p += ret%PAGE_SIZE;
	return ret;
}

unsigned kmalloc_ap(unsigned s, unsigned *p)
{
	unsigned ret = do_kmalloc(s, 1);
	vm_getmap(ret, p);
	*p += ret%PAGE_SIZE;
	return ret;
}

void kfree(void *pt)
{
	if(!pt) return;
	mutex_on(&km_m);
	if(do_kfree_wrap)
		do_kfree_wrap(pt);
	else
		panic(PANIC_MEM | PANIC_NOSYNC, "No kfree installed!");
	mutex_off(&km_m);
}
