#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <linux/unistd.h>
//#include <linux/smp_lock.h>
#include <linux/smp.h>
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
#include <linux/cpumask.h>
#include <linux/mm_types.h>
#include <linux/rbtree.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>

#include <asm/atomic.h>
#include <asm/init.h>
#include <linux/sched.h>


#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>


/*
	mode = 1;//UC(Uncacheable)
	mode = 2;//Write-combining
	mode = 3;//write-protected
	mode = 4;//writethrough
	mode = 5;//writeback
*/
#define CACHE_MODE 1

#define START_POINT 	0x100000000 
#define CHUNK_SIZE    	 0x80000000 
#define CACHE_SIZE  64 
#define ITER	8

MODULE_LICENSE("GPL");

#define IA32_CR_PAT 0x277

int __init ioremap_cache_control_init( void );
void __exit ioremap_cache_control_exit( void );
extern void cache_control( unsigned long , unsigned long, unsigned int);
void mem_test(volatile unsigned int *vaddr_1, volatile unsigned int *vaddr_2, unsigned int test_size);
//extern int sched_setaffinity(pid_t, const struct cpumask *);

volatile unsigned long cnt;
volatile unsigned long idx;
volatile unsigned long *time;

void mem_test(volatile unsigned int *vaddr_1, volatile unsigned int *vaddr_2, unsigned int test_size){
	unsigned int i=0, j=0;
	volatile unsigned int *tvaddr_1, *tvaddr_2;
	/* parkhk : time check */
	u64 start_tsc, end_tsc;
	rdtscll(start_tsc);

	while( j++ < ITER)
	{
		tvaddr_1 = vaddr_1;
		tvaddr_2 = vaddr_2;
		
		//*tvaddr_1 = i;
		//*tvaddr_2 = j;
		for(i=0; i < test_size; tvaddr_1++, tvaddr_2++, i++)
		{
			 *tvaddr_1 += i;//*tvaddr_2;// -j + stride;
			 *tvaddr_2 += i;//*tvaddr_1;// + i + stride;
		}
	}
	rdtscll(end_tsc);
	time[cnt++] = (unsigned long)(end_tsc - start_tsc); 
	//printk(KERN_NOTICE"%ld %lld\n", stride, measured_cycles/*/2930000(ms)*/);
}

int __init ioremap_cache_control_init()
{

	
	void *criterion_addr;
	void *relative_addr;
	volatile unsigned int *c_addr;
	volatile unsigned int *r_addr;
	unsigned long criterion_time;
	unsigned long i;
	char str[100];
	
	void *time_measure;
	struct cpumask cpus;

	unsigned long bit_mask = 0x1;
	unsigned long start_addr;
	int fd;
	mm_segment_t old_fs = get_fs();

	time_measure = vmalloc(0x1000 * sizeof(volatile unsigned long));
	time = (volatile unsigned long *) time_measure;
	for(start_addr = 0 ; start_addr < 0x1000; start_addr++)
		time[start_addr] = 0;

	set_fs(KERNEL_DS);
	sprintf(str, "./results/identify_bank_rank");
	fd = sys_open(str, O_WRONLY|O_CREAT|O_TRUNC, 0644);

	criterion_addr = ioremap( (resource_size_t)0x0, 0x1000 );
	relative_addr = ioremap( (resource_size_t) 0x100000000, 0xf0000000 );
	printk(KERN_EMERG "criterion_addr : %p\n", criterion_addr );
	printk(KERN_EMERG "relative_addr : %p\n",relative_addr);

	cache_control( (unsigned long)criterion_addr, 0x1000, CACHE_MODE);
	cache_control( (unsigned long)relative_addr, 0xf0000000, CACHE_MODE);

	/*sys_close(fd);
	set_fs(old_fs);
	iounmap(criterion_addr);
	iounmap(relative_addr);
	vfree(time_measure);
	return 0;*/
	cpumask_clear(&cpus);
	cpumask_set_cpu(0, &cpus);

	if (sched_setaffinity(current->pid, &cpus));

//////// criterion time

	cnt=0;
	idx=0;

	c_addr = (volatile unsigned int *)criterion_addr;
	r_addr = (volatile unsigned int *)criterion_addr;
	r_addr += (64/sizeof(volatile unsigned int));
	local_irq_disable();
	preempt_disable();
		
	mem_test(c_addr, r_addr, 64/sizeof(unsigned int));

	preempt_enable();
	local_irq_enable();
	printk(KERN_EMERG "Criterion time : %lu\n", time[0]);
	criterion_time = time[0]+(time[0]/10);

	cnt=0;
////////
	c_addr = (volatile unsigned int *)criterion_addr;
	//c_addr = (volatile unsigned int *)relative_addr;//criterion_addr;
	r_addr = (volatile unsigned int *)relative_addr;//criterion_addr;

/*	local_irq_disable();
	preempt_disable();
	mem_test(c_addr, r_addr, 64/sizeof(unsigned int));
	preempt_enable();
	local_irq_enable();
	printk(KERN_EMERG "Criterion(hit) : %lu\n", time[0]);
*/
	bit_mask <<=2;
	for(i=0;i<2;i++){
		sprintf(str, "%lu\n", idx++);
		sys_write(fd, str, strlen(str));
	}
	cnt=0;
	local_irq_disable();
	preempt_disable();
	for(; bit_mask < 0xf0000000; bit_mask <<=1 ){
		//r_addr = (volatile unsigned int *)relative_addr;//criterion_addr;
		start_addr = (unsigned long)relative_addr;//criterion_addr;
		start_addr += bit_mask; 
		//start_addr += (1<<16); 
		//start_addr += (1<<6); 
		//r_addr += (bit_mask/sizeof(volatile unsigned int));
		r_addr = (volatile unsigned int *)start_addr;
		mem_test(c_addr, r_addr, 64/sizeof(unsigned int));
	}
	preempt_enable();
	local_irq_enable();

	for(i=0;i<cnt;i++){
		sprintf(str, "%lu %lu\n", idx++, time[i]);
		sys_write(fd, str, strlen(str));
	}

	sys_close(fd);
	set_fs(old_fs);
	iounmap(criterion_addr);
	iounmap(relative_addr);
	vfree(time_measure);
	return 0;
}

void __exit ioremap_cache_control_exit()
{
	printk("Module test end\n");
}

module_init(ioremap_cache_control_init);
module_exit(ioremap_cache_control_exit);

