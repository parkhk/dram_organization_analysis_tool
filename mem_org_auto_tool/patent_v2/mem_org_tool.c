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
#define TOTAL_SIZE	0x200000000 
#define CHUNK_SIZE    	  0x1000000 
#define CACHE_LINE_SIZE 	4 
#define ITER		16 

MODULE_LICENSE("GPL");

#define IA32_CR_PAT 0x277

int __init ioremap_cache_control_init( void );
void __exit ioremap_cache_control_exit( void );
extern void cache_control( unsigned long , unsigned long, unsigned int);
static int ioremap_core(unsigned long start_addr, unsigned long size, int fd);
unsigned long mem_test(volatile unsigned int *vaddr_1, volatile unsigned int *vaddr_2, unsigned int test_size);
//extern int sched_setaffinity(pid_t, const struct cpumask *);

volatile unsigned long cnt;
volatile unsigned long idx;
volatile unsigned long *time;
void *criterion_addr;
unsigned long criterion_time;

unsigned long interleave_unit;

static int ioremap_core(volatile unsigned int *addr, unsigned long size, int fd)
{
	volatile unsigned long i;
	unsigned long test_size;
	char str[100];

	size = size/interleave_unit;
	test_size = interleave_unit/sizeof(unsigned int);
	cnt=0;

	local_irq_disable();
	preempt_disable();
		
	for(i=0 ; i<size ;i++, addr+=test_size)
		mem_test( (volatile unsigned int *)criterion_addr, addr, test_size);

	preempt_enable();
	local_irq_enable();
	
	for(i=0;i<cnt;i++){
		/*if(time[i] < criterion_time)
			sprintf(str, "%lu 0\n", idx++);
		else
			sprintf(str, "%lu 1\n", idx++);
		*/sprintf(str, "%lu %lu\n", idx++, time[i]);
    		sys_write(fd, str, strlen(str));
	}
	return 0;
}

unsigned long mem_test(volatile unsigned int *vaddr_1, volatile unsigned int *vaddr_2, unsigned int test_size){

	unsigned int i=0, j=0;
	volatile unsigned int *tvaddr_1, *tvaddr_2;
	/* parkhk : time check */
	u64 start_tsc, end_tsc;
	rdtscll(start_tsc);

	while( j++ < ITER)
	{
		tvaddr_1 = vaddr_1;
		tvaddr_2 = vaddr_2;
		
		for(i=0; i < test_size; tvaddr_1++, tvaddr_2++, i++)
		{
			 *tvaddr_1 += i;//*tvaddr_2;// -j + stride;
			 *tvaddr_2 += i;//*tvaddr_1;// + i + stride;
		}
	}
	rdtscll(end_tsc);
	/*if((end_tsc - start_tsc) < criterion_time)
		return 0;
	else
		return 1;*/
	return (unsigned long)(end_tsc - start_tsc);
	//time[cnt++] = (unsigned long)(end_tsc - start_tsc); 
	//printk(KERN_NOTICE"%ld %lld\n", stride, measured_cycles/*/2930000(ms)*/);
}

int __init ioremap_cache_control_init()
{
	void *time_measure;
	struct cpumask cpus;

	void *vaddr;
	unsigned long start_addr;
	int fd;
	char file_name[40];
	mm_segment_t old_fs = get_fs();

/*
 *  Create Log File
 */
	set_fs(KERNEL_DS);
	sprintf(file_name, "./results/bank_conflict");
	fd = sys_open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
/*
 * Initializing the data for measuring time
 */
	time_measure = vmalloc((CHUNK_SIZE/CACHE_LINE_SIZE) * sizeof(volatile unsigned long));
	time = (volatile unsigned long *) time_measure;
	for(start_addr = 0 ; start_addr < (CHUNK_SIZE/CACHE_LINE_SIZE); start_addr++)
		time[start_addr] = 0;
/*
 * Creating phys. to virt. mapping for test area
 */
	vaddr = ioremap( (resource_size_t)START_POINT, TOTAL_SIZE );
	cache_control( (unsigned long)vaddr, TOTAL_SIZE, CACHE_MODE);
	addr = (unsigned int *)vaddr;
/*
 * Making criterion address(First address line)
 */
	criterion_addr = ioremap( (resource_size_t)0x0, 0x1000 );
	cache_control( (unsigned long)criterion_addr, 0x1000, CACHE_MODE);
/*
 * Set CPU affinity
 */ 
	cpumask_clear(&cpus);
	cpumask_set_cpu(0, &cpus);
	sched_setaffinity(current->pid, &cpus);
/*
 * Define criterion time
 */ 

	cnt = 0;
	idx = 0;

	start_addr = (unsigned long)criterion_addr;
	start_addr+=CACHE_LINE_SIZE;
	local_irq_disable();
	preempt_disable();
		
	mem_test( (volatile unsigned int *)criterion_addr, (volatile unsigned int *)start_addr, CACHE_LINE_SIZE/sizeof(unsigned int));

	preempt_enable();
	local_irq_enable();
	printk(KERN_EMERG "Criterion time : %lu\n", time[0]);
	criterion_time = time[0]+(time[0]/10);

/*
 * Getting Interleaving Unit 
 */ 
	for( ;  ; ){
		mem_test( (volatile unsigned int *)criterion_addr, (volatile unsigned int *)start_addr, CACHE_LINE_SIZE/sizeof(unsigned int));
	}
		
	
	for(start_addr = START_POINT ; start_addr < END_POINT ; start_addr+=CHUNK_SIZE){
		ioremap_core(start_addr, CHUNK_SIZE, fd);
	}

	iounmap(vaddr);
	sys_close(fd);
	set_fs(old_fs);
	iounmap(criterion_addr);
	vfree(time_measure);
	return 0;
}

void __exit ioremap_cache_control_exit()
{
	printk("Module test end\n");
}

module_init(ioremap_cache_control_init);
module_exit(ioremap_cache_control_exit);

