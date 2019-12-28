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
#define CACHE_SIZE  64 
#define ITER 128	

MODULE_LICENSE("GPL");

int __init ioremap_cache_control_init( void );
void __exit ioremap_cache_control_exit( void );
extern void cache_control( unsigned long , unsigned long, unsigned int);

unsigned long mem_test_base(volatile unsigned char *c_addr_1, volatile unsigned char *c_addr_2, unsigned char test_size){
	unsigned char i=0, j=0;
	volatile unsigned char *tc_addr_1, *tc_addr_2;
	/* parkhk : time check */
	u64 start_tsc, end_tsc;
	rdtscll(start_tsc);

	while( j++ < ITER)
	{
		tc_addr_1 = c_addr_1;
		tc_addr_2 = c_addr_2;
		
		//*tvaddr_1 = i;
		//*tvaddr_2 = j;
		for(i=0; i < test_size; tc_addr_1++, tc_addr_2++, i++)
		{
			 *tc_addr_1 += i;
			 *tc_addr_2 += i;
		}
	}
	rdtscll(end_tsc);
	return (unsigned long)(end_tsc - start_tsc); 
	//printk(KERN_NOTICE"%ld %lld\n", stride, measured_cycles/*/2930000(ms)*/);
}


unsigned long mem_test(volatile unsigned char *c_addr_1, volatile unsigned char *c_addr_2, volatile unsigned char *r_addr_1, volatile unsigned char *r_addr_2, unsigned char test_size){
	unsigned char i=0, j=0;
	volatile unsigned char *tc_addr_1, *tc_addr_2;
	volatile unsigned char *tr_addr_1, *tr_addr_2;
	/* parkhk : time check */
	u64 start_tsc, end_tsc;
	rdtscll(start_tsc);

	while( j++ < ITER)
	{
		tc_addr_1 = c_addr_1;
		tc_addr_2 = c_addr_2;
		tr_addr_1 = r_addr_1;
		tr_addr_2 = r_addr_2;
		
		//*tvaddr_1 = i;
		//*tvaddr_2 = j;
		for(i=0; i < test_size; tc_addr_1++, tc_addr_2++, tr_addr_1++, tr_addr_2++, i++)
		{
		//	 *tc_addr_1 += *tr_addr_1 += *tc_addr_2 += *tr_addr_2;
			 *tc_addr_1 += *tr_addr_1;
			 *tr_addr_1 += *tc_addr_1;
			 *tc_addr_2 += *tr_addr_2;
			 *tr_addr_2 += *tc_addr_2;
		}
	}
	rdtscll(end_tsc);
	return (unsigned long)(end_tsc - start_tsc); 
	//printk(KERN_NOTICE"%ld %lld\n", stride, measured_cycles/*/2930000(ms)*/);
}

unsigned long find_most_bit(const unsigned long addr){
	unsigned long bit = 0x1;
	unsigned long i;
	unsigned long most_bit = 0x0;
	unsigned long max_bit= sizeof(unsigned long)*8;

	for(bit = 0x1, i = 0; i < max_bit ; bit<<=1, i++){
		//printk(KERN_EMERG"addr = %lx, bit = %lx\n", addr, bit);
		if(addr & bit)
			most_bit = i;
	}
	return most_bit;
}

void array_init(unsigned long *arr, unsigned long size){
	unsigned long i;
	for(i = 0 ; i < size ; i++)
		arr[i] = 0;

}
int __init ioremap_cache_control_init()
{

	
	void *zero_addr;
	void *reserved_addr;
	volatile unsigned char *addr[4];
	unsigned long i;
	char str[100];
	extern unsigned long max_pfn; 
	unsigned long *conflict;
	void *conf;

	unsigned long reserved_size;
	
	int fd;
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	conf= vmalloc(1100000 * sizeof(unsigned long));
	conflict= (unsigned long *) conf;
	for(i= 0 ; i < 1100000; i++)
		conflict[i] = 0;

	array_init(conflict, 1100000);
	reserved_size = (max_pfn<<PAGE_SHIFT)-START_POINT;
	//ioremap
	zero_addr = ioremap( (resource_size_t)0x0, PAGE_SIZE);
	reserved_addr = ioremap( (resource_size_t)START_POINT, reserved_size);
	//cache control
	cache_control( (unsigned long)zero_addr, PAGE_SIZE, CACHE_MODE);
	cache_control( (unsigned long)reserved_addr, reserved_size, CACHE_MODE);

	sprintf(str, "./results/identify_page");
	fd = sys_open(str, O_WRONLY|O_CREAT|O_TRUNC, 0644);

	addr[0] = (volatile unsigned char *)reserved_addr;
	addr[1] = addr[0];
	for(i = 0; i < START_POINT/0x1000; i++ ){
		addr[1]+=0x1000;

		local_irq_disable();
		preempt_disable();
		conflict[i]= mem_test_base(addr[0], addr[1], 64);
		preempt_enable();
		local_irq_enable();

	}
	for(i = 0; i < START_POINT/4096; i++ ){
		sprintf(str, "%lu %lu\n", i, conflict[i]);
		sys_write(fd, str, strlen(str));
	}
	sys_close(fd);
	set_fs(old_fs);
	iounmap(zero_addr);
	iounmap(reserved_addr);
	vfree(conf);
	return 0;
}

void __exit ioremap_cache_control_exit()
{
	printk("Module test end\n");
}

module_init(ioremap_cache_control_init);
module_exit(ioremap_cache_control_exit);

