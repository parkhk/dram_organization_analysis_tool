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



#define TEST_SIZE 0x8 //32byte

MODULE_LICENSE("GPL");


#define IA32_CR_PAT 0x277

int __init ioremap_cache_control_init( void );
void __exit ioremap_cache_control_exit( void );
extern void cache_control( unsigned long , unsigned long, unsigned int);
static int ioremap_core(unsigned long start_addr, unsigned long size, int mode, int fd);
void mem_test(unsigned int *vaddr_1, unsigned int *vaddr_2, unsigned long stride/*page unit*/);
//extern int sched_setaffinity(pid_t, const struct cpumask *);

volatile unsigned long long idx=0;
volatile unsigned long long index=0;
unsigned int *e_time;

static int ioremap_core(unsigned long start_addr, unsigned long size, int mode, int fd)
{
	void *vaddr, *eaddr, *fix_addr;
	volatile unsigned long long i;
	char str[100];
	struct cpumask cpus;
	unsigned int *addr;

	printk("start!!\n");
	cpumask_clear(&cpus);
	cpumask_set_cpu(0, &cpus);

	if (sched_setaffinity(current->pid, &cpus));

	fix_addr = ioremap( (resource_size_t)0x0, 0x1000 );
	cache_control( (unsigned long)fix_addr, 0x1000, mode);

	vaddr = ioremap( (resource_size_t)start_addr, size );
	cache_control( (unsigned long)vaddr, size, mode);
	addr = (unsigned int *)vaddr;

	size = size/32;
	eaddr = ioremap( (resource_size_t)0x300000000, 0x100000000);
	e_time = (unsigned int *)eaddr;
	idx=0;

	local_irq_disable();
	preempt_disable();
		
	for(i=0 ; i<size ;i++, addr+=TEST_SIZE)
		mem_test( (unsigned int *)fix_addr, addr, i);

	preempt_enable();
	local_irq_enable();
	printk("done!!\n");
	for(i=0;i<idx;i++){
		sprintf(str, "%llu %u\n", index++, e_time[i]);
    		sys_write(fd, str, strlen(str));
	}
	iounmap(fix_addr);
	iounmap(vaddr);
	iounmap(eaddr);
	return 0;
}


void mem_test(unsigned int *vaddr_1, unsigned int *vaddr_2, unsigned long stride/*page unit*/){
	unsigned int i=0, j=0;
	unsigned int *tvaddr_1, *tvaddr_2;
	/* parkhk : time check */
	u64 start_tsc, end_tsc;
	rdtscll(start_tsc);

	while( j++ < 32)
	{
		tvaddr_1 = vaddr_1;
		tvaddr_2 = vaddr_2;
		
		//*tvaddr_1 = i;
		//*tvaddr_2 = j;
		for(i=0; i < TEST_SIZE; tvaddr_1++, tvaddr_2++, i++)
		{
			 *tvaddr_1 += i;//*tvaddr_2;// -j + stride;
			 *tvaddr_2 += i;//*tvaddr_1;// + i + stride;
		}
	}
	rdtscll(end_tsc);
	e_time[idx++] = (unsigned long)(end_tsc - start_tsc); 
	//printk(KERN_NOTICE"%ld %lld\n", stride, measured_cycles/*/2930000(ms)*/);


}

int __init ioremap_cache_control_init()
{

/*
	mode = 1;//UC(Uncacheable)
	mode = 2;//Write-combining
	mode = 3;//write-protected
	mode = 4;//writethrough
	mode = 5;//writeback
*/

	unsigned long start_addr;
	unsigned long size;
	int mode;
	int fd;
	char file_name[40];
//	unsigned int i=0;

	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	sprintf(file_name, "./results/bank_conflict_64M");
	fd = sys_open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	start_addr = 0x100000000;
	size = 0x1000000;
	mode = 1;

	for(start_addr = 0x100000000; start_addr < 0x104000000 ; start_addr+=size){
		ioremap_core(start_addr, size, mode, fd);
	}

		sys_close(fd);
	set_fs(old_fs);
	return 0;
}

void __exit ioremap_cache_control_exit()
{
	printk("Module test end\n");
}

module_init(ioremap_cache_control_init);
module_exit(ioremap_cache_control_exit);

