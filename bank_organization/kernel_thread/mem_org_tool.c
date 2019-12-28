#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
//#include <linux/smp_lock.h>
#include <linux/mmzone.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
//#include <linux/config.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <asm/io.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/page-flags.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/syscalls.h>
#include <linux/sysctl.h>
#include <linux/mm_types.h>
#include <linux/rbtree.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>

#include <asm/atomic.h>
#include <asm/init.h>


#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>


#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/unistd.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/sched.h>
#include <asm/current.h>


/*
	mode = 1;//UC(Uncacheable)
	mode = 2;//Write-combining
	mode = 3;//write-protected
	mode = 4;//writethrough
	mode = 5;//writeback
*/
#define CACHE_MODE 1

#define START_POINT 	0x100000000 
#define SIZE_OF_RESERVED_AREA 0x700000000 
#define MOST_BIT    	0x800000000 
#define CACHE_SIZE  64 
#define ITER 0x1000000

MODULE_LICENSE("GPL");

int __init ioremap_cache_control_init( void );
void __exit ioremap_cache_control_exit( void );
extern void cache_control( unsigned long , unsigned long, unsigned int);
unsigned long tmp;
volatile unsigned long flags;
volatile unsigned long flags_2;


struct task_struct *g_th_id[4];

typedef unsigned long int __cpu_mask;

#define __CPU_SETSIZE 1024
#define __NCPUBITS (8*sizeof(__cpu_mask))

#define __CPUELT(cpu) ((cpu)/__NCPUBITS)
#define __CPUMASK(cpu) ((__cpu_mask)1<<((cpu)%__NCPUBITS))

typedef struct 
{
	__cpu_mask __bits[__CPU_SETSIZE/__NCPUBITS];
}cpu_set_t;
# define __CPU_SET_S(cpu, setsize, cpusetp) \
  (__extension__							      \
   ({ size_t __cpu = (cpu);						      \
      __cpu / 8 < (setsize)						      \
      ? (((__cpu_mask *) ((cpusetp)->__bits))[__CPUELT (__cpu)]		      \
	 |= __CPUMASK (__cpu))						      \
      : 0; }))

#  define __CPU_ZERO_S(setsize, cpusetp) \
  do {									      \
    size_t __i;								      \
    size_t __imax = (setsize) / sizeof (__cpu_mask);			      \
    __cpu_mask *__bits = (cpusetp)->__bits;				      \
    for (__i = 0; __i < __imax; ++__i)					      \
      __bits[__i] = 0;							      \
  } while (0)


# define CPU_SET(cpu, cpusetp)	 __CPU_SET_S (cpu, sizeof (cpu_set_t), cpusetp)
# define CPU_ZERO(cpusetp)	 __CPU_ZERO_S (sizeof (cpu_set_t), cpusetp)

unsigned long bit_shift;

unsigned long mem_test_base(volatile unsigned char *c_addr_1, volatile unsigned char *c_addr_2, unsigned char test_size){
	unsigned long j=0;
	/* parkhk : time check */
	u64 start_tsc, end_tsc;
	rdtscll(start_tsc);

	while( j++ < ITER)
	{
		tmp += *c_addr_1;// += i;
		tmp -= *c_addr_2;// += i;
	}
	rdtscll(end_tsc);
	return (unsigned long)(end_tsc - start_tsc); 
	//printk(KERN_NOTICE"%ld %lld\n", stride, measured_cycles/*/2930000(ms)*/);
}

static int kthread_example_thr_fun(void *arg)
{
	volatile unsigned char *addr[2];
	int val;
	unsigned len;
	cpu_set_t mask;
	cpumask_t new_mask;
	unsigned long conflict;
	void *lower_map;

	unsigned long lower_size;
	unsigned long lower_addr;
	val = (int)(long)arg;

	CPU_ZERO(&mask);
	CPU_SET(val, &mask);
	len=sizeof(cpumask_t);
	memcpy((int *)&new_mask, (int *)&mask, len);
	sched_setaffinity(current->pid, &new_mask);	

	lower_addr = 0x0;
	lower_size = 0xc0000000;
	//ioremap && cache control
	printk(KERN_EMERG"Start IOREMAP\n");
	lower_map = ioremap( (resource_size_t)lower_addr, lower_size);
	if(!lower_map){
		printk(KERN_EMERG"IOREMAP FAIL for lower addr\n");
		return 0;
	}else{
		printk(KERN_EMERG"IOREMAP SUCCESS for lower\n");
	}
	cache_control( (unsigned long)lower_map, lower_size, CACHE_MODE);

	addr[0] = (volatile unsigned char *)lower_map;
	flags++;
	while(!flags_2);	

	addr[1] = addr[0];
	addr[1]+=(0x1LU<<bit_shift);

	local_irq_disable();
	preempt_disable();
	conflict= mem_test_base(addr[0], addr[1], 1);
	preempt_enable();
	local_irq_enable();
	printk(KERN_EMERG"[bank_conflict] elapsed time[bit = %ld] = %ld\n", bit_shift, conflict);
	tmp=0;


	printk("<0> finish kernel_thread %d CPU : %d\n", val, smp_processor_id());
	iounmap(lower_map);
	return 0;
} 

int __init ioremap_cache_control_init()
{
	bit_shift = 6;
	g_th_id[0] = (struct task_struct *)kthread_run(kthread_example_thr_fun, (void *)0, "0_test");
	g_th_id[1] = (struct task_struct *)kthread_run(kthread_example_thr_fun, (void *)1, "1_test");
	g_th_id[2] = (struct task_struct *)kthread_run(kthread_example_thr_fun, (void *)2, "2_test");
	g_th_id[3] = (struct task_struct *)kthread_run(kthread_example_thr_fun, (void *)3, "3_test");
	flags_2 = 1;
	while(flags!=4);
	printk("kernel_thread \n");

	return 0;
}

void __exit ioremap_cache_control_exit()
{
	printk("Module test end\n");
}

module_init(ioremap_cache_control_init);
module_exit(ioremap_cache_control_exit);

