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
#define SIZE_OF_RESERVED_AREA 0x700000000 
#define MOST_BIT    	0x800000000 
#define CACHE_SIZE  64 
#define ITER 128	

MODULE_LICENSE("GPL");

#define IA32_CR_PAT 0x277

int __init ioremap_cache_control_init( void );
void __exit ioremap_cache_control_exit( void );
extern void cache_control( unsigned long , unsigned long, unsigned int);
void mem_test_base(volatile unsigned char *c_addr_1, volatile unsigned char *c_addr_2, unsigned char test_size);
void mem_test(volatile unsigned char *c_addr_1, volatile unsigned char *c_addr_2, volatile unsigned char *r_addr_1, volatile unsigned char *r_addr_2, unsigned char test_size);
//extern int sched_setaffinity(pid_t, const struct cpumask *);

volatile unsigned long cnt;
volatile unsigned long idx;
volatile unsigned long *time;

void mem_test_base(volatile unsigned char *c_addr_1, volatile unsigned char *c_addr_2, unsigned char test_size){
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
	time[cnt++] = (unsigned long)(end_tsc - start_tsc); 
	//printk(KERN_NOTICE"%ld %lld\n", stride, measured_cycles/*/2930000(ms)*/);
}


void mem_test(volatile unsigned char *c_addr_1, volatile unsigned char *c_addr_2, volatile unsigned char *r_addr_1, volatile unsigned char *r_addr_2, unsigned char test_size){
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
	time[cnt++] = (unsigned long)(end_tsc - start_tsc); 
	//printk(KERN_NOTICE"%ld %lld\n", stride, measured_cycles/*/2930000(ms)*/);
}

int __init ioremap_cache_control_init()
{

	
	void *zero_addr;
	void *reserved_addr;
	volatile unsigned char *addr[4];
	unsigned long i;
	char str[100];
	
	void *time_measure;

	unsigned long bit_mask = 0x1;
	unsigned long start_addr;
	int fd;
	mm_segment_t old_fs = get_fs();

	time_measure = vmalloc(0x1000 * sizeof(volatile unsigned long));
	time = (volatile unsigned long *) time_measure;
	for(start_addr = 0 ; start_addr < 0x1000; start_addr++)
		time[start_addr] = 0;

	set_fs(KERNEL_DS);
	sprintf(str, "./results/identify_high_bits");
	fd = sys_open(str, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	
	zero_addr = ioremap( (resource_size_t)0x0, 0x1000);
	reserved_addr = ioremap( (resource_size_t)START_POINT, SIZE_OF_RESERVED_AREA);
	cache_control( (unsigned long)zero_addr, 0x1000, CACHE_MODE);
	cache_control( (unsigned long)reserved_addr, START_POINT+SIZE_OF_RESERVED_AREA, CACHE_MODE);

	cnt=0;
	idx=0;
	bit_mask <<=32;
	printk(KERN_EMERG "bit_mask = %lx\n", bit_mask);
	addr[0] = (volatile unsigned char *)zero_addr;
	local_irq_disable();
	preempt_disable();

	mem_test_base(addr[0], addr[0], 64);

	preempt_enable();
	local_irq_enable();
	sprintf(str, "%lu %lu\n", idx++, time[0]);
	sys_write(fd, str, strlen(str));
	cnt=0;
	idx=0;
	for(i=0;i<32;i++){
		sprintf(str, "%lu\n", idx++);
		sys_write(fd, str, strlen(str));
	}

	for(; bit_mask < MOST_BIT; bit_mask <<=1 ){
		addr[1] = (volatile unsigned char *)reserved_addr;
	//	i=0;
	//	printk(KERN_EMERG "addr[%ld] : %p\n", i, addr[1]);
		addr[1]+=(bit_mask-START_POINT);
	//	printk(KERN_EMERG "addr[%ld] : %p\n", i, addr[1]);

		local_irq_disable();
		preempt_disable();
		mem_test_base(addr[0], addr[1], 64);
		preempt_enable();
		local_irq_enable();

	}
	for(i=0;i<cnt;i++){
		sprintf(str, "%lu %lu\n", idx++, time[i]);
		sys_write(fd, str, strlen(str));
	}

	sys_close(fd);

	sprintf(str, "./results/identify_low_bits");
	fd = sys_open(str, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	cnt=0;
	idx=0;
	bit_mask = 0x1;
	bit_mask <<=6;
	for(i=0;i<6;i++){
		sprintf(str, "%lu\n", idx++);
		sys_write(fd, str, strlen(str));
	}

	addr[0] = (volatile unsigned char *)reserved_addr;
	for(; bit_mask < START_POINT; bit_mask <<=1 ){
		addr[1] = (volatile unsigned char *)reserved_addr;
	//	i=0;
	//	printk(KERN_EMERG "addr[%ld] : %p\n", i, addr[1]);
		addr[1]+=bit_mask;
	//	printk(KERN_EMERG "addr[%ld] : %p\n", i, addr[1]);

		local_irq_disable();
		preempt_disable();
		mem_test_base(addr[0], addr[1], 64);
		preempt_enable();
		local_irq_enable();

	}
	addr[0] = (volatile unsigned char *)zero_addr;
	for(; bit_mask < MOST_BIT; bit_mask <<=1 ){
		addr[1] = (volatile unsigned char *)reserved_addr;
	//	i=0;
	//	printk(KERN_EMERG "addr[%ld] : %p\n", i, addr[1]);
		addr[1]+=(bit_mask-START_POINT);
	//	printk(KERN_EMERG "addr[%ld] : %p\n", i, addr[1]);

		local_irq_disable();
		preempt_disable();
		mem_test_base(addr[0], addr[1], 64);
		preempt_enable();
		local_irq_enable();

	}
	for(i=0;i<cnt;i++){
		sprintf(str, "%lu %lu\n", idx++, time[i]);
		sys_write(fd, str, strlen(str));
	}


	sys_close(fd);

////////
	addr[0] = (volatile unsigned char *)reserved_addr;
	addr[1] = (volatile unsigned char *)reserved_addr;
	bit_mask = 0x1;
	bit_mask <<=33;
	printk("(bit_mask)%lx : %lx(start)\n", bit_mask, START_POINT);
	addr[0]+=(bit_mask-START_POINT);
	addr[1]+=(bit_mask-START_POINT);
	bit_mask >>=3;
	addr[1]+=bit_mask;

	sprintf(str, "./results/identify_col_bits");
	fd = sys_open(str, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	cnt=0;
	idx=0;
	bit_mask = 0x1;
	bit_mask <<=6;
	for(i=0;i<6;i++){
		sprintf(str, "%lu\n", idx++);
		sys_write(fd, str, strlen(str));
	}
	for(; bit_mask < (START_POINT>>3); bit_mask <<=1 ){
		addr[2] = addr[0]+bit_mask;
		addr[3] = addr[1]+bit_mask;
		local_irq_disable();
		preempt_disable();
		//mem_test(addr[0], addr[1], addr[2], addr[3], 64);
		mem_test_base(addr[0], addr[3], 64);
		preempt_enable();
		local_irq_enable();
		//mem_test_base(addr[0], addr[2], 64);
		//mem_test_base(c_addr[1], r_addr[1], 64);
	}

	for(i=0;i<cnt;i++){
		sprintf(str, "%lu %lu\n", idx++, time[i]);
		sys_write(fd, str, strlen(str));
	}

	sys_close(fd);
	set_fs(old_fs);
	iounmap(zero_addr);
	iounmap(reserved_addr);
	vfree(time_measure);
	return 0;
}

void __exit ioremap_cache_control_exit()
{
	printk("Module test end\n");
}

module_init(ioremap_cache_control_init);
module_exit(ioremap_cache_control_exit);

