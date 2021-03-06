/* every file should include this. This defines the basic things needed for
 * kernel operation. asm is redefined here, which is vital. */

#ifndef KERNEL_H
#define KERNEL_H
#define asm __sync_synchronize(); __asm__ __volatile__
#include <types.h>
#include <string.h>
#include <config.h>
#include <vsprintf.h>
#include <console.h>
#include <asm/system.h>
#include <memory.h>
#include <syscall.h>
#include <time.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <mutex.h>
extern char shutting_down;
extern volatile int panicing;
extern volatile unsigned int __allow_idle;
#define super_cli __super_cli
#define super_sti __super_sti
#define PANIC_NOSYNC 1
#define PANIC_MEM    2
#define __UTSNAMELEN 65
struct utsname {
    char sysname[__UTSNAMELEN];
    char nodename[__UTSNAMELEN];
    char release[__UTSNAMELEN];
    char version[__UTSNAMELEN];
    char machine[__UTSNAMELEN];
    char domainname[__UTSNAMELEN];
};

#define assert(c) if(!(c)) panic_assert(__FILE__, __LINE__, #c)

#define assert_act(c, q) if(!(c)) { \
	kprintf("-----Assertion Failure Information-----\n"); \
		void (*w)()= (void (*)())q; \
		w();\
		panic("Assertion failed: %s", #c); \
		}

#define assert_act2(c, q) if(!(c)) { \
	kprintf("-----Assertion Failure Information-----\n"); \
		void (*w)()= (void (*)())q; \
		w();\
		printk(5, "Assertion failed: %s", #c); \
		}

static inline void __super_cli()
{
	__sync_synchronize();
	__asm__ volatile("cli");
}

static inline void __super_sti()
{
	__sync_synchronize();
	__asm__ volatile("sti");
}

static inline void outb(short port, char value)
{
	__asm__ volatile ("outb %1, %0" : : "dN" (port), "a" ((unsigned char)value));
}

static inline char inb(short port)
{
	char ret;
	__asm__ volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

static inline short inw(short port)
{
	short ret;
	__asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

static inline void outw(short port, short val)
{
	__asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (val));
}

static inline void outl(short port, int val)
{
	__asm__ volatile ("outl %1, %0" : : "dN" (port), "a" (val));
}

static inline int inl(short port)
{
	int ret;
	__asm__ volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

static inline void __engage_idle()
{
	__allow_idle=1;
}

static inline void __disengage_idle()
{
	__allow_idle=0;
}

static inline void get_kernel_version(char *b)
{
	char t = 'a';
	if(PRE_VER >= 4 && PRE_VER < 8)
		t = 'b';
	if(PRE_VER >= 8 && PRE_VER < 10)
		t = 'c';
	if(PRE_VER == 10)
		t=0;
	int p=0;
	if(PRE_VER < 8)
		p = (PRE_VER % 4)+1;
	else
		p = (PRE_VER - 7);
	sprintf(b, "%d.%d%c%c%d", MAJ_VER, MIN_VER, t ? '-' : 0, t, p);
}

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

void print_trace(unsigned int MaxFrames);
void panic(int flags, char *fmt, ...);
void serial_puts(int, char *);
void kernel_reset();
void panic_assert(const char *file, u32int line, const char *desc);
void kernel_poweroff();
int sys_uname(struct utsname *name);
int get_timer_th(int *t);
extern int april_fools;
void unregister_interrupt_handler(unsigned char n, isr_t);
int sys_isstate(int pid, int state);
void do_reset();
int sys_gethostname(char *buf, size_t len);
void restart_int();
int proc_append_buffer(char *buffer, char *data, int off, int len, 
	int req_off, int req_len);
#endif
