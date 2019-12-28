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
#define ITER 0x10000

MODULE_LICENSE("GPL");

int __init ioremap_cache_control_init( void );
void __exit ioremap_cache_control_exit( void );
extern void cache_control( unsigned long , unsigned long, unsigned int);
unsigned long tmp;

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
	void *lower_map, *upper_map;
	volatile unsigned char *addr[4];
	unsigned long i;//, j;
	char str[100];
	unsigned long lower_most_bit, upper_most_bit, set_bit, most_comp_bit, least_comp_bit;
	extern unsigned long max_pfn; 
	unsigned long conflict[64];
	unsigned long bank_bit[64];
	unsigned long bit_idx;

	unsigned long lower_size, upper_size;
	unsigned long lower_addr, upper_addr;
	unsigned long avr;
	
	int fd;
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	lower_addr = 0x0;
	upper_addr = START_POINT;
	lower_size = 0xc0000000;
	upper_size = ((max_pfn<<PAGE_SHIFT)-upper_addr);
	printk(KERN_EMERG"[bank_conflict] lower area = 0x%lx(size:%lx), upper area = 0x%lx(size:%lx)\n", lower_addr, lower_size, upper_addr, upper_size);
	
	lower_most_bit = find_most_bit(lower_addr+lower_size);
	upper_most_bit = find_most_bit(upper_addr+upper_size);
	printk(KERN_EMERG"[bank_conflict] most bit(lower:upper = %ld:%ld)\n", lower_most_bit, upper_most_bit);

	array_init(conflict, 64);
	//ioremap && cache control
	printk(KERN_EMERG"Start IOREMAP\n");
	lower_map = ioremap( (resource_size_t)lower_addr, lower_size);
	if(!lower_map){
		printk(KERN_EMERG"IOREMAP FAIL for lower addr\n");
		return 0;
	}
	upper_map = ioremap( (resource_size_t)upper_addr, upper_size);
	if(!upper_map){
		printk(KERN_EMERG"IOREMAP SUCCESS for upper addr\n");
		printk(KERN_EMERG"IOREMAP FAIL for upper addr\n");
		iounmap(lower_map);
		return 0;
	}else{
		printk(KERN_EMERG"IOREMAP SUCCESS for lower & upper addr\n");
	}
	cache_control( (unsigned long)lower_map, lower_size, CACHE_MODE);
	cache_control( (unsigned long)upper_map, upper_size, CACHE_MODE);


	sprintf(str, "./results/identify_bits_same_addr");
	fd = sys_open(str, O_WRONLY|O_CREAT|O_TRUNC, 0644);

	addr[0] = (volatile unsigned char *)lower_map;
	//printk(KERN_EMERG"[bank_conflict] LOWER addr conflict start\n");
	for(set_bit = 0; set_bit <= lower_most_bit; set_bit++ ){
		addr[1] = addr[0];
		addr[1]+=(0x1LU<<set_bit);

		local_irq_disable();
		preempt_disable();
		conflict[set_bit]= mem_test_base(addr[0], addr[1], 1);
		preempt_enable();
		local_irq_enable();
		printk(KERN_EMERG"[bank_conflict] elapsed time[bit = %ld] = %ld\n", set_bit, conflict[set_bit]);
		tmp=0;

	}
	//printk(KERN_EMERG"[bank_conflict] UPPER addr conflict start\n");
	for(; set_bit <= upper_most_bit; set_bit++ ){
		addr[1] = (volatile unsigned char *)upper_map;
		addr[1]+=((0x1LU<<set_bit)-upper_addr);

		local_irq_disable();
		preempt_disable();
		conflict[set_bit]= mem_test_base(addr[0], addr[1], 1);
		preempt_enable();
		local_irq_enable();
		printk(KERN_EMERG"[bank_conflict] elapsed time[bit = %ld] = %ld\n", set_bit, conflict[set_bit]);
		tmp=0;

	}

	for(i=0, bit_idx=0;i<set_bit;i++){

		sprintf(str, "%lu %lu\n", i, conflict[i]);
		sys_write(fd, str, strlen(str));
	}
	sys_close(fd);

	most_comp_bit = 0;
	least_comp_bit = -1;
	for(i=13;i<set_bit;i++){
		if(conflict[i]>most_comp_bit){
			most_comp_bit = conflict[i];
		}else if(conflict[i]<least_comp_bit){
			least_comp_bit = conflict[i];
		}
	}
	avr=((most_comp_bit + least_comp_bit) / 2);
	most_comp_bit = 0;
	least_comp_bit = 0;
	printk(KERN_EMERG"[bank_conflict] Median value = %ld\n", avr);
	for(i=upper_most_bit;i>0;i--){
		if(conflict[i]>avr){
			most_comp_bit = i;
			printk(KERN_EMERG"bank_conflict most compare bit = %ld\n", i);
			break;
		}
	}
	for(i=20/*1MB*/;i<lower_most_bit;i++){
		if(conflict[i]>avr){
			least_comp_bit = i;
			printk(KERN_EMERG"bank_conflict least compare bit = %ld\n", i);
			break;
		}
	}

///////////////////

	sprintf(str, "./results/identify_bits_diff_addr");
	fd = sys_open(str, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	array_init(conflict, 64);
	addr[0] = (volatile unsigned char *)lower_map;

	for(set_bit = 0; set_bit < least_comp_bit ; set_bit++ ){

		addr[1]=addr[0];
		addr[1]+=(0x1LU<<least_comp_bit);
		addr[1]+=(0x1LU<<set_bit);

		local_irq_disable();
		preempt_disable();
		conflict[set_bit]= mem_test_base(addr[0], addr[1], 1);
		preempt_enable();
		local_irq_enable();
		tmp=0;
	}

	for(; set_bit <= lower_most_bit ; set_bit++ ){

		addr[1]=addr[0];
		addr[1]+=(0x1LU<<set_bit);

		local_irq_disable();
		preempt_disable();
		conflict[set_bit]= mem_test_base(addr[0], addr[1], 1);
		preempt_enable();
		local_irq_enable();
		tmp=0;

	}

	for(; set_bit <= upper_most_bit; set_bit++ ){
		addr[1] = (volatile unsigned char *)upper_map;
		if(set_bit == 34)
		addr[1]+=((0x1LU<<33)-upper_addr);
		addr[1]+=((0x1LU<<set_bit)-upper_addr);

		local_irq_disable();
		preempt_disable();
		conflict[set_bit]= mem_test_base(addr[0], addr[1], 1);
		preempt_enable();
		local_irq_enable();
		tmp=0;

	}
	for(i=0, bit_idx=0;i<set_bit;i++){

		if(conflict[i]<avr && conflict[i]){
			bank_bit[bit_idx++] = i;
		}
		sprintf(str, "%lu %lu\n", i, conflict[i]);
		sys_write(fd, str, strlen(str));
	}
	for(i=0;i<bit_idx;i++){
		printk(KERN_EMERG"bank_conflict conflict bit = %ld\n", bank_bit[i]);
	}
	
	///// Find xor bit precheck
	/*sys_close(fd);
	array_init(conflict, 64);
	addr[0] = (volatile unsigned char *)zero_addr;
	for(i=0;i<bit_idx && bank_bit[i] != most_bit;i++){
		if(bank_bit[i] == start_bit)
			continue;
		addr[1] = (volatile unsigned char *)ioremap_addr;
		addr[1]+=((0x1UL<<bank_bit[i]));

		local_irq_disable();
		preempt_disable();
		conflict[bank_bit[i]]= mem_test_base(addr[0], addr[1], 1);
		preempt_enable();
		local_irq_enable();
		tmp=0;
		if(conflict[bank_bit[i]] > avr){
			printk("bank_conflict xor bit = %ld & %ld\n", start_bit, bank_bit[i]);
		}else
			printk("bank_conflict It isn't xor bit(%ld & %ld) lat : %ld \n", start_bit, bank_bit[i], conflict[bank_bit[i]]);

	}

	///// Find xor bit
	//sprintf(str, "./results/identify_xor_bits");
	//fd = sys_open(str, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	addr[0] = (volatile unsigned char *)ioremap_addr;
	for(i=0;i<bit_idx && bank_bit[i] != most_bit;i++){
		if(bank_bit[i] == start_bit)
			continue;
		for(j=i+1;j<bit_idx && bank_bit[j] != most_bit;j++){
			unsigned long conf;
			if(bank_bit[j] == start_bit)
				continue;
			addr[1]=addr[0];
			addr[1]+=((0x1UL<<bank_bit[i]));
			addr[1]+=((0x1UL<<bank_bit[j]));

			local_irq_disable();
			preempt_disable();
			conf= mem_test_base(addr[0], addr[1], 1);
			preempt_enable();
			local_irq_enable();
		tmp=0;
			if(conf > avr){
				printk("bank_conflict xor bit = %ld & %ld\n", bank_bit[i], bank_bit[j]);
			}else
				printk("bank_conflict It isn't xor bit(%ld & %ld) lat : %ld avr : %ld\n", bank_bit[i], bank_bit[j], conf, avr);
		}
	}*/
	//for(i=0;i<set_bit;i++){

	//	sprintf(str, "%lu %lu\n", i, conflict[i]);
	//	sys_write(fd, str, strlen(str));
	//}

	sys_close(fd);
	set_fs(old_fs);
	iounmap(lower_map);
	iounmap(upper_map);
	return 0;
}

void __exit ioremap_cache_control_exit()
{
	printk("Module test end\n");
}

module_init(ioremap_cache_control_init);
module_exit(ioremap_cache_control_exit);

