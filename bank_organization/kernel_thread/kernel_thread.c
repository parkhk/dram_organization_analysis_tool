#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/unistd.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/sched.h>
#include <asm/current.h>

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

volatile unsigned long flags;

static int kthread_example_thr_fun(void *arg)
{
	int val;
	unsigned long i, j;
	unsigned len;
	cpu_set_t mask;
	cpumask_t new_mask;
	val = (int)(long)arg;

	CPU_ZERO(&mask);
	CPU_SET(val, &mask);
	len=sizeof(cpumask_t);
	memcpy((int *)&new_mask, (int *)&mask, len);
	sched_setaffinity(current->pid, &new_mask);	

	printk("<0>kernel_thread %d CPU : %d pid : %d\n", val, smp_processor_id(), current->pid);
	for(i=0,j=0 ; i<100000000 ; i++)
	{
		j+=i;
	}
	printk("<0>%ld before stop kernel_thread %d CPU : %d\n",j, val, smp_processor_id());
	while(!kthread_should_stop())
		ssleep(1);
	printk("<0> finish kernel_thread %d CPU : %d\n", val, smp_processor_id());
	return 0;
} 

static int __init kthread_example_init(void)
{
	printk(KERN_ALERT "@ %s() : called\n", __FUNCTION__);

		g_th_id[0] = (struct task_struct *)kthread_run(kthread_example_thr_fun, (void *)0, "3 test");
		g_th_id[1] = (struct task_struct *)kthread_run(kthread_example_thr_fun, (void *)1, "3 test");
		g_th_id[2] = (struct task_struct *)kthread_run(kthread_example_thr_fun, (void *)2, "3 test");
		g_th_id[3] = (struct task_struct *)kthread_run(kthread_example_thr_fun, (void *)3, "3 test");
	printk(KERN_ALERT "@ %s() : called\n", __FUNCTION__);
	return 0;
} 

static void __exit kthread_example_release(void)
{
		kthread_stop(g_th_id[0]);
		kthread_stop(g_th_id[1]);
		kthread_stop(g_th_id[2]);
		kthread_stop(g_th_id[3]);
	printk(KERN_ALERT "@ %s() : Bye.\n", __FUNCTION__);
} 
 
module_init(kthread_example_init);
module_exit(kthread_example_release);
MODULE_LICENSE("Dual BSD/GPL"); 
