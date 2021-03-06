#ifndef TASK_H
#define TASK_H
#include <kernel.h>
#include <memory.h>
#include <fs.h>
#include <sys/stat.h>
#include <sig.h>
#include <mmfile.h>
#include <dev.h>
#define GOD 0 
#define ISGOD(x) (current_task->uid == GOD)
#define KERN_STACK_SIZE 0x16000
#define __EXIT     0
#define __COREDUMP 1
#define __EXITSIG  2
#define __STOPSIG  4
#define TASK_MAGIC 0xDEADBEEF
#define FILP_HASH_LEN 512
#define TF_WMUTEX     0x1
#define TF_EXITING    0x2
#define TF_ALARM      0x4
#define TF_DIDLOCK    0x8
#define TF_SWAP      0x10
#define TF_KTASK     0x20
#define TF_SWAPQUEUE 0x40
#define TF_LOCK      0x80
#define TF_REQMEM   0x100
#define TF_DYING    0x200
#define TF_FORK     0x400
#define TF_INSIG    0x800
#define TF_SCHED   0x1000
#define TF_JUMPIN  0x2000
#define PRIO_PROCESS 1
#define PRIO_PGRP    2
#define PRIO_USER    3

#if CONFIG_SMP
#define current_task (__get_current_task())
#endif

#if SCHED_TTY
static int sched_tty = SCHED_TTY_CYC;
#else
static int sched_tty = 0;
#endif

typedef struct exit_status {
	unsigned pid;
	int ret;
	unsigned sig;
	char coredump;
	int cause;
	struct exit_status *next, *prev;
} ex_stat;

/** I believe its fair to describe the storage of tasks. 
 * There are many linked lists.
 * 
 * First, theres the basic master list. kernel_task is 
 * start, and every task is DLL'd to it.
 * 
 * Second, there is the family list. This is kept track 
 * of by 'parent'. Each task's parent field points to the 
 * task that created it.
 * 
 * Third, there is 'waiting'. This is used when a task is 
 * waiting on another task, and points to it.
 * 
 * Fourth, there is the alarm list. This is a single linked 
 * list that is a list of all tasks that are waiting for alarms.
 * 
 * If a task is dead, it goes to the 'dead queue'. the tokill 
 * list. This is DLL, using the next and prev fields.
 * 
 * alarm_list_start is the first alarmed task.
 * 
 * current_task points to the task currently running.
 */
typedef volatile struct task_struct
{
	volatile unsigned magic;
	volatile addr_t pid, eip, ebp, esp;
	page_dir_t *pd;
	volatile int state, old_state, critical, critical_c;
	volatile char critical_stack[256];
	unsigned int system; 
	addr_t kernel_stack;
	int cur_ts, priority;
	
	volatile addr_t *waitflag, waiting_ret;
	unsigned wait_for;
	char waiting_true;
	volatile long tick;
	volatile unsigned flags;
	int flag;
	
	unsigned stime, utime;
	unsigned t_cutime, t_cstime;
	volatile addr_t stack_end;
	volatile unsigned num_pages;
	unsigned last;
	ex_stat exit_reason, we_res, *exlist;
	registers_t reg_b;
	registers_t *regs, *sysregs;
	unsigned mem_usage_calc;
	volatile unsigned wait_again, path_loc_start;
	unsigned num_swapped;
	
	volatile addr_t heap_start, heap_end, he_red;
	char command[128];
	char **argv, **env;
	int cmask;
	int tty;
	struct inode *root, *pwd;
	struct file_ptr *filp[FILP_HASH_LEN];
	uid_t uid, _uid;
	gid_t gid, _gid;
	mmf_t *mm_files;
	vma_t *mmf_priv_space, *mmf_share_space;
	
	struct sigaction signal_act[128];
	volatile sigset_t sig_mask, global_sig_mask;
	volatile unsigned sigd, cursig;
	sigset_t old_mask;
	unsigned alrm_count;
	unsigned freed, allocated;
	volatile struct task_struct *next, *prev, *parent, *waiting, *alarm_next;
} task_t;

extern volatile task_t *kernel_task, *tokill, *alarm_list_start;

#if CONFIG_SMP
#include <cpu.h>
static inline __attribute__((always_inline))  volatile task_t *__get_current_task()
{
	unsigned t=0;
	if(kernel_task) {
		asm ("mov %%gs, %0" : "=r" (t));
		cpu_t *c = get_cpu(t);
		return c->current;
	}
	return (task_t *)t;
}
#else /* !CONFIG_SMP */
extern volatile task_t *current_task;
#endif

static inline __attribute__((always_inline)) 
void set_current_task_dp(task_t *t, int cpu)
{
#if CONFIG_SMP
	cpu_t *c = get_cpu(cpu);
	c->current = (void *)t;
#else
	current_task = t;
	return;
#endif
}

#define raise_flag(f) current_task->flags |= f
#define lower_flag(f) current_task->flags &= ~f
void delay_sleep(int t);
void force_schedule();
void take_issue_with_current_task();
void clear_resources(task_t *);
int times(struct tms *buf);
extern void set_kernel_stack(u32int stack);
void run_scheduler();
extern volatile long ticks;
int set_gid(int new);
int set_uid(int new);
void release_mutexes(task_t *t);
int get_gid();
int get_uid();
int fork();
int read_eip();
int get_exit_status(int pid, int *status, int *retval, int *signum, int *);
int sys_waitpid(int pid, int *st, int opt);
int sys_wait3(int *, int, int *);
int task_stat(unsigned int pid, struct task_stat *s);
int task_pstat(unsigned int, struct task_stat *s);
void schedule();
int get_pid();
void init_multitasking();
void exit(int);
task_t *get_next_task();
void kill_task(unsigned int);
int sys_getppid();
void __wait_flag(unsigned *f, int, char *file, int line);
void release_task(task_t *p);
int wait_task(unsigned pid, int state);
void delay(int);
int send_signal(int, int);
void handle_signal(task_t *t);
void wait_flag_except(unsigned *f, int fo);
struct file *get_file_pointer(task_t *t, int n);
void remove_file_pointer(task_t *t, int n);
int add_file_pointer(task_t *t, struct file *f);
int add_file_pointer_after(task_t *, struct file *f, int after);
void copy_file_handles(task_t *p, task_t *n);
void freeze_all_tasks();
void unfreeze_all_tasks();
int sys_alarm(int a);
int get_mem_usage();
void take_issue_with_current_task();
extern volatile unsigned next_pid;
task_t *get_task_pid(int pid);
int do_send_signal(int, int, int);
extern unsigned glob_sched_eip;
void kill_all_tasks();
void task_unlock_mutexes(task_t *t);
void do_force_nolock(task_t *t);
void clear_mmfiles(task_t *t, int);
void copy_mmf(task_t *old, task_t *new);
void check_mmf_and_flush(task_t *t, int fd);
int load_so_library(char *name);
int sys_ret_sig();
void close_all_files(task_t *);
int sys_gsetpriority(int set, int which, int id, int val);
int sys_waitagain();
void force_nolock(task_t *);
int get_task_mem_usage(task_t *t);
int sys_nice(int which, int who, int val, int flags);
int sys_setsid();
int sys_setpgid(int a, int b);
void task_suicide();
extern unsigned ret_values_size;
extern unsigned *ret_values;
void set_signal(int sig, unsigned hand);
extern mutex_t scheding;
int sys_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, 
	struct timeval *timeout);
int swap_in_page(task_t *, unsigned);
#define wait_flag(a, b) __wait_flag(a, b, __FILE__, __LINE__)
#define lock_scheduler() _lock_scheduler(__FILE__, __LINE__);
#define unlock_scheduler() _unlock_scheduler(__FILE__, __LINE__);

static inline  __attribute__((always_inline))  
void _lock_scheduler(char *f, int l)
{
	current_task->flags |= TF_LOCK;
	__super_cli();
}

static inline  __attribute__((always_inline))  
void _unlock_scheduler(char *f, int l)
{
	current_task->flags &= ~TF_LOCK;
	__super_sti();
}

static inline int got_signal(task_t *t)
{
	return (t->sigd);
}

static __attribute__((always_inline)) inline int count_tasks()
{
	task_t *t = (task_t *)kernel_task;
	int i=0;
	while(t)
	{
		i++;
		t=(task_t *)t->next;
	}
	return i;
}

static __attribute__((always_inline)) inline void task_critical()
{
	//if(!current_task) return;
	cli();
	current_task->critical+=1;
}

static __attribute__((always_inline))  inline void task_uncritical()
{
	//if(!current_task) return;
	cli();
	if(current_task->critical > 0)
		current_task->critical--;
}

static __attribute__((always_inline)) inline void task_full_uncritical()
{
	//if(!current_task) return;
	current_task->critical=0;
	current_task->critical_c=0;
}

static __attribute__((always_inline)) inline void enter_system(int sys)
{
	current_task->system=(!sys ? -1 : sys);
	lower_flag(TF_DIDLOCK);
	current_task->cur_ts/=2;
}

static inline int __is_valid_user_ptr(void *p, char flags)
{
	unsigned addr = (unsigned)p;
	if(!addr && !flags) return 0;
	if(addr < TOP_LOWER_KERNEL && addr) {
#if DEBUG
		printk(5, "[kernel]: warning - task %d passed ptr %x to syscall (invalid)\n", 
			current_task->pid, addr);
#endif
		return 0;
	}
	if(addr >= KMALLOC_ADDR_START) {
#if DEBUG
		printk(5, "[kernel]: warning - task %d passed ptr %x to syscall (invalid)\n", 
			current_task->pid, addr);
#endif
		return 0;
	}
	return 1;
}

static __attribute__((always_inline)) inline void exit_system()
{
	current_task->last = current_task->system;
	current_task->system=0;
}

static inline int GET_MAX_TS(task_t *t)
{
	if(t->flags & TF_EXITING || t->flags & TF_WMUTEX)
		return 1;
	int x = t->priority;
	if(t->tty == curcons->tty)
		x += sched_tty;
	return x;
}

struct inode *set_as_kernel_task(char *name);
__attribute__((always_inline)) inline static int task_is_runable(task_t *task)
{
	assert(task);
	if(task->flags & TF_REQMEM) {
		task->old_state = task->state;
		task->state = TASK_RUNNING;
		return 1;
	}
	if(task->state == TASK_FROZEN || task->state == TASK_DEAD)
		return 0;
	return (int)(task->state == TASK_RUNNING 
		|| task->state == TASK_SUICIDAL 
		|| task->state == TASK_SIGNALED 
		|| (task->state == TASK_ISLEEP && (task->sigd)));
}

#endif
