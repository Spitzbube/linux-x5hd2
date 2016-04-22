/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name             :    runtime_module.c
  Version               :     Initial Draft
  Author                :     Hisilicon multimedia software group
  Created               :     2012/09/07
  Last Modified        :
  Description          :
  Function List        :    
  History                :
  1.Date                 :     2012/09/07
    Author               :    
    Modification        :    Created file

******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <linux/module.h>
#include <linux/signal.h>
#include <linux/spinlock.h>
#include <linux/personality.h>
#include <linux/ptrace.h>
#include <linux/kallsyms.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/seq_file.h>

#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/traps.h>
#include <linux/semaphore.h>

#include <linux/miscdevice.h>

#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/io.h>
#include <asm/pgalloc.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/time.h>

#include <linux/statfs.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/namei.h>

#include <mach/cpu-info.h>

#include "sha1.h"

/*NB!: customer may be insert module with large size. 
In this case, you should increase the macro value */
#define MODULE_RESERVED_DDR_LENGTH      0x600000  //size for runtime check area

#define MAX_BUFFER_LENGTH	(0x20000) //128k

#define SC_SYS_BASE     IO_ADDRESS(0xF8000000)
#define SC_SYSRES       (SC_SYS_BASE + 0x0004)
#define SC_GEN15		(SC_SYS_BASE + 0x00BC) //This register is set by the fastboot, indicated the C51 code is loaded.

#define MCU_START_REG	0xf840f000

#define C51_CODE_LOAD_FLAG	0x80510002

#define RUNTIME_CHECK_EN_REG_ADDR   0xF8AB0084//OTP:runtime_check_en indicator :0xF8AB0084[20]

#define MAX_IOMEM_SIZE	0x400

#define HI_REG_WRITE32(addr, val) (*(volatile unsigned int*)(addr) = (val))
#define HI_REG_READ32(addr, val) ((val) = *(volatile unsigned int*)(addr))

#define C51_BASE             0xf8400000
#define C51_SIZE             0x10000
#define C51_DATA             0xe000

static struct task_struct *g_pModuleCopyThread = NULL;
static struct task_struct *g_pFsCheckThread = NULL;
static unsigned int g_u32ModuleVirAddr = 0, g_u32ModulePhyAddr=0;
static unsigned int g_u32C51CheckVectorVirAddr = 0;
static int g_bRuntimeCheckInit = false;
static int g_bC51CodeLoaded = false;
static int g_bRuntimeCheckEnable = false;

int module_copy_thread_proc(void* argv);
int fs_check_thread_proc(void* argv);
int get_kernel_info(unsigned int *pu32StartAddr, unsigned int *pu32EndAddr);
int calc_kernel_hash(unsigned int u32Hash[5]);
int calc_fs_hash(unsigned int u32Hash[5]);
int calc_ko_hash(unsigned int u32Hash[5]);
int store_check_vector(void);
int get_bootargs_info(void);

extern long long get_chipid(void);


static int GetRuntimeCheckEnableFlag(int *pbRuntimeCheckFlag)
{
    unsigned int *pu32RegVirAddr = NULL;

    if(NULL == pbRuntimeCheckFlag)
        return -1;

    *pbRuntimeCheckFlag = false;
    
    pu32RegVirAddr = (unsigned int *)ioremap_nocache(RUNTIME_CHECK_EN_REG_ADDR, 32);
    if(pu32RegVirAddr == NULL)
    {
        return -1;
    }
    if(*pu32RegVirAddr & 0x100000)
    {
        *pbRuntimeCheckFlag = true;
    }
    iounmap((void*)pu32RegVirAddr);

    return 0;
}

int  RuntimeModule_Init(void)
{
	long long chipid;
	
	chipid = get_chipid(); 
    if(chipid == _HI3716CV200ES || chipid == _HI3716CV200 || chipid == _HI3719CV100
    	|| chipid == _HI3719MV100A || chipid == _HI3719MV100 || chipid == _HI3716MV400)
    {    	
        (void)GetRuntimeCheckEnableFlag(&g_bRuntimeCheckEnable);        
    }
    else    
    {
        g_bRuntimeCheckEnable = false;
    }
    
    if (NULL == g_pModuleCopyThread)
    {
	    g_pModuleCopyThread = kthread_create(module_copy_thread_proc, NULL, "ModuleCopyThread");
	    if (NULL == g_pModuleCopyThread)
	    {
	        return -1;
	    }
	    wake_up_process(g_pModuleCopyThread);
	}

	if (NULL == g_pFsCheckThread)
	{
	    g_pFsCheckThread = kthread_create(fs_check_thread_proc, NULL, "FsCheckThread");
	    if (NULL == g_pFsCheckThread)
	    {
	        return -1;
	    }
	    wake_up_process(g_pFsCheckThread);	    
    }
        
    return 0;
}

int module_copy_thread_proc(void* argv)
{
    struct module *p = NULL;
    struct module *mod;
    static int bIsFirstCopy = true;
    struct list_head *pModules = NULL;
    int Ret = 0;
    unsigned int u32Addr;
        
    msleep(10000);
    
    printk("\n******** Runtime Check Initial ***********\n");
    
    (void)get_bootargs_info();
    HI_REG_READ32(SC_GEN15, g_bC51CodeLoaded);
    
    printk("g_bRuntimeCheckEnable=0x%x g_bC51CodeLoaded=0x%x g_u32ModulePhyAddr=0x%x\n\n", 
    	g_bRuntimeCheckEnable, 
    	g_bC51CodeLoaded, 
    	g_u32ModulePhyAddr);

    if(g_bC51CodeLoaded != C51_CODE_LOAD_FLAG)
    {
    	kthread_stop(g_pFsCheckThread); //step the fs check thread
        return -1;
    }    
    g_bRuntimeCheckInit = true;

    if((g_u32ModulePhyAddr==0)||(!g_bRuntimeCheckEnable))//runtime check is disable
    {
    	writel(0x1,IO_ADDRESS(MCU_START_REG));//Æô¶¯MCU3
    	return 0;	
    }    
   
    g_u32ModuleVirAddr = (unsigned int)ioremap_nocache(g_u32ModulePhyAddr, MODULE_RESERVED_DDR_LENGTH);    
    memset((void*)g_u32ModuleVirAddr, 0x0, MODULE_RESERVED_DDR_LENGTH);
  
    module_get_pointer(&pModules);    
    while(1)
    {     
        u32Addr = g_u32ModuleVirAddr;
        list_for_each_entry_rcu(mod, pModules, list)
        {
            p = find_module(mod->name);              
            if(p)
            {
                memcpy((void*)u32Addr, (void*)(p->module_core), p->core_text_size);                
                u32Addr += p->core_text_size;
            }
        }        
   
        if(bIsFirstCopy)
        {
	        store_check_vector();
			writel(0x1,IO_ADDRESS(MCU_START_REG));//Æô¶¯MCU3
			//writel(0x3,IO_ADDRESS(0xf8ab0000 + 0x184));//lpc_ram_wr_disable, lpc_rst_disable
            bIsFirstCopy = false;
        }
        
        msleep(1000);
    }
    
    return Ret;
}

int fs_check_thread_proc(void* argv)
{
    int Ret = 0;
    unsigned int u32HashValue[5];
    unsigned int u32FsHashValue[5];
    unsigned int i;
    unsigned int *pVIR_ADDRESS = NULL;
    static int bIsFirstCal = true;

    while(!g_bRuntimeCheckInit)
    {
        msleep(10);
    }
    
    while(g_bRuntimeCheckEnable)
    //while(1)
    {        
        calc_fs_hash(u32HashValue);
        if(bIsFirstCal)
        {
            for(i = 0; i < 5; i ++)
            {
                u32FsHashValue[i] = u32HashValue[i];
            }
            
            bIsFirstCal = false;
        }

        Ret = memcmp(u32FsHashValue, u32HashValue, sizeof(u32HashValue));
        if(Ret)
        {
            /*  error process: reset the whole chipset */
            pVIR_ADDRESS  = (void*)ioremap_nocache(SC_SYSRES, 0x100);
            *(unsigned int*)pVIR_ADDRESS = 0x1;
            iounmap((void*)pVIR_ADDRESS);
        }

        msleep(10000);
    }
	
    return 0;
}

int get_kernel_info(unsigned int *pu32StartAddr, unsigned int *pu32EndAddr)
{
    unsigned char *pTmpbuf = NULL;
    struct file * fp;
    mm_segment_t fs;
    loff_t pos;
    char s8TmpBuf[32];
    char *pstr = NULL;            
	long long chipid;
	
	chipid = get_chipid(); 
    if(chipid == _HI3716CV200ES || chipid == _HI3716CV200 || chipid == _HI3719CV100
    	|| chipid == _HI3719MV100A || chipid == _HI3719MV100 || chipid == _HI3716MV400)
   
    {
		memset(s8TmpBuf, 0, sizeof(s8TmpBuf));
		strncpy(s8TmpBuf, "Kernel code", strlen("Kernel code"));
    }    
    
    fs = get_fs();
    set_fs(KERNEL_DS);    
    set_fs(fs);

    /* get file handle */
    fp = filp_open("/proc/iomem", O_RDONLY | O_LARGEFILE, 0644);
    if (IS_ERR(fp))
    {
        return -1;
    }

    pTmpbuf = kmalloc(MAX_IOMEM_SIZE, GFP_TEMPORARY);        
    if(pTmpbuf == NULL)
    {
        filp_close(fp, NULL);
        return -1;
    }
    memset(pTmpbuf, 0, MAX_IOMEM_SIZE);

    /* get file content */
    pos = 0;    
    fs = get_fs();
    set_fs(KERNEL_DS);
    vfs_read(fp, pTmpbuf, MAX_IOMEM_SIZE, &pos);
    set_fs(fs);

    pstr = strstr(pTmpbuf, s8TmpBuf);
    if(pstr == NULL)
    {
    	kfree(pTmpbuf);
        filp_close(fp, NULL);
        return -1;    
    }
    pos = pstr - (char *)pTmpbuf;

    memset(s8TmpBuf, 0, sizeof(s8TmpBuf));
    memcpy(s8TmpBuf, pTmpbuf + pos - 20, 8);
    *pu32StartAddr = simple_strtoul(s8TmpBuf, 0, 16);
    
    memset(s8TmpBuf, 0, sizeof(s8TmpBuf));
    memcpy(s8TmpBuf, pTmpbuf + pos - 11, 8);
    *pu32EndAddr = simple_strtoul(s8TmpBuf, 0, 16);
    
    kfree(pTmpbuf);
    /* close file handle */
    filp_close(fp, NULL);

    return 0;
}

int calc_kernel_hash(unsigned int u32Hash[5])
{
	unsigned int u32TmpValue;
	unsigned int u32KernelStartAddr, u32KernelEndAddr, u32KernelSize;
	int Ret = 0;
	sha1_context ctx;
	unsigned char u8HashValue[20];
	unsigned int u32KernelVirAddr;
	unsigned int i;
	unsigned int u32OneTimeLen;
	unsigned int u32LeftLen;
	unsigned int u32Offset = 0;

	Ret = get_kernel_info(&u32KernelStartAddr, &u32KernelEndAddr);
	if(Ret != 0)
	{
		return -1;
	}
	
	u32KernelSize = u32KernelEndAddr - u32KernelStartAddr;	
    u32KernelVirAddr = (unsigned int)phys_to_virt(u32KernelStartAddr);

 	hi_sha1_starts(&ctx);

 	u32LeftLen = u32KernelSize;
 	while(u32LeftLen > 0)
 	{
		u32OneTimeLen = u32LeftLen > MAX_BUFFER_LENGTH ? MAX_BUFFER_LENGTH : u32LeftLen;
		hi_sha1_update(&ctx, (unsigned char*)u32KernelVirAddr + u32Offset, u32OneTimeLen);
		u32LeftLen -= u32OneTimeLen;
		u32Offset += u32OneTimeLen;
		msleep(10);
	}
	
	hi_sha1_finish(&ctx, u8HashValue);

	for(i = 0; i < 20;)
	{
		u32TmpValue = ((unsigned int)u8HashValue[i++]) << 24;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]) << 16;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]) << 8;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]);		
		u32Hash[i / 4 - 1] = u32TmpValue;		
	}
	
	return 0;	
}

int calc_ko_hash(unsigned int u32Hash[5])
{
	unsigned int u32TmpValue;	
	sha1_context ctx;
	unsigned char u8HashValue[20];
	unsigned int i;
	unsigned int u32OneTimeLen;
	unsigned int u32LeftLen;	
	unsigned int u32Offset = 0;
		
	hi_sha1_starts(&ctx);
 	u32LeftLen = MODULE_RESERVED_DDR_LENGTH;
 	while(u32LeftLen > 0)
 	{	
 		u32OneTimeLen = u32LeftLen > MAX_BUFFER_LENGTH ? MAX_BUFFER_LENGTH : u32LeftLen;
		hi_sha1_update(&ctx, (unsigned char*)g_u32ModuleVirAddr + u32Offset, u32OneTimeLen);
		u32LeftLen -= u32OneTimeLen;
		u32Offset += u32OneTimeLen;
		msleep(10);
	}		
	hi_sha1_finish(&ctx, u8HashValue);
	
	for(i = 0; i < 20;)
	{
		u32TmpValue = ((unsigned int)u8HashValue[i++]) << 24;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]) << 16;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]) << 8;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]);
		
		u32Hash[i / 4 - 1] = u32TmpValue;
	}	
		
	return 0;
}

int calc_fs_hash(unsigned int u32Hash[5])
{
    int Ret = 0;
    unsigned char u8HashValue[20];
    unsigned int i;
    unsigned int u32TmpValue;
    static unsigned char *pTmpbuf = NULL;
    sha1_context ctx;
    struct file * fp;
    mm_segment_t fs;
    struct path path;
    struct kstatfs st;
    loff_t pos;
    unsigned int u32ReadLength;
    unsigned int u32LeftLength;

	fs = get_fs();
	set_fs(KERNEL_DS);
	set_fs(fs);
	
	Ret = user_path("/", &path);
	if(Ret == 0)
	{
		Ret = vfs_statfs(&path,&st);
		path_put(&path);
	}

	/* get file handle */
	fp = filp_open("/dev/ram0", O_RDONLY | O_LARGEFILE, 0644);
	if (IS_ERR(fp))
    {
		return -1;
	}

	if(pTmpbuf == NULL)
	{
	    pTmpbuf = vmalloc(MAX_BUFFER_LENGTH);
	    if(pTmpbuf == NULL)
	    {
	    	filp_close(fp, NULL);
	        return -1;
	    }
	}

	/* get file content */
	pos = 0;
    u32LeftLength = st.f_bsize * st.f_blocks;
	hi_sha1_starts(&ctx);
   	while(u32LeftLength > 0)
   	{
	    u32ReadLength = u32LeftLength >= MAX_BUFFER_LENGTH ? MAX_BUFFER_LENGTH : u32LeftLength;
	    u32LeftLength -= u32ReadLength;
	    fs = get_fs();
	    set_fs(KERNEL_DS);
	    vfs_read(fp, pTmpbuf, u32ReadLength, &pos);
	    set_fs(fs);
	    hi_sha1_update(&ctx, pTmpbuf, u32ReadLength);
        msleep(100);
    }
    hi_sha1_finish(&ctx, u8HashValue);
    
	for(i = 0; i < 20;)
	{
		u32TmpValue = ((unsigned int)u8HashValue[i++]) << 24;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]) << 16;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]) << 8;
		u32TmpValue |= ((unsigned int)u8HashValue[i++]);
		
		u32Hash[i / 4 - 1] = u32TmpValue;
	}

    //vfree(pTmpbuf);
	/* close file handle */
	filp_close(fp, NULL);
	
    return 0;	
}

int store_check_vector(void)
{
	unsigned int u32HashValue[5];
	unsigned int *pu32Ptr = NULL;
	unsigned int i;
	unsigned int u32SectionNum = 2; //kernel, KO
	unsigned int u32KernelStartAddr, u32KernelEndAddr, u32KernelSize;
	
    g_u32C51CheckVectorVirAddr = (unsigned int)ioremap_nocache(C51_BASE + C51_DATA + 0xE00, C51_SIZE - C51_DATA - 0xE00);    
    memset((void*)g_u32C51CheckVectorVirAddr, 0x0, C51_SIZE - C51_DATA - 0xE00);	
    
 	//kernel hash value
	calc_kernel_hash(u32HashValue);
	for(i = 0; i < 5; i++)
	{
	    pu32Ptr = (unsigned int*)(((unsigned char*)g_u32C51CheckVectorVirAddr) + 0x14) + i;
		HI_REG_WRITE32(pu32Ptr, u32HashValue[i]);
	}

	//ko hash value
	calc_ko_hash(u32HashValue);	
	for(i = 0; i < 5; i++)
	{
	    pu32Ptr = (unsigned int*)(((unsigned char*)g_u32C51CheckVectorVirAddr) + 0x28) + i;
		HI_REG_WRITE32(pu32Ptr, u32HashValue[i]);
	}

	//check section number, address, length
	pu32Ptr = (unsigned int*)(((unsigned char*)g_u32C51CheckVectorVirAddr) + 0x50);
	HI_REG_WRITE32(pu32Ptr, u32SectionNum);
	
	//kernel address, length
	get_kernel_info(&u32KernelStartAddr, &u32KernelEndAddr);
	u32KernelSize = u32KernelEndAddr - u32KernelStartAddr;
	pu32Ptr = (unsigned int*)(((unsigned char*)g_u32C51CheckVectorVirAddr) + 0x54);
	HI_REG_WRITE32(pu32Ptr, u32KernelStartAddr);
	
	pu32Ptr = (unsigned int*)(((unsigned char*)g_u32C51CheckVectorVirAddr) + 0x58);
	HI_REG_WRITE32(pu32Ptr, u32KernelSize);	

	//KO address, length
	pu32Ptr = (unsigned int*)(((unsigned char*)g_u32C51CheckVectorVirAddr) + 0x5c);
	HI_REG_WRITE32(pu32Ptr, g_u32ModulePhyAddr);
	
	pu32Ptr = (unsigned int*)(((unsigned char*)g_u32C51CheckVectorVirAddr) + 0x60);
	HI_REG_WRITE32(pu32Ptr, MODULE_RESERVED_DDR_LENGTH);

	iounmap((void*)g_u32C51CheckVectorVirAddr);	

	return 0;	
}

int get_bootargs_info(void)
{
    unsigned char *pTmpbuf = NULL;
    struct file * fp;
    mm_segment_t fs;
    loff_t pos;
    char s8TmpBuf[16];
    unsigned int u32MmzStartAddress;
    char *pstr = NULL;
 
  
    fs = get_fs();
    set_fs(KERNEL_DS);
    set_fs(fs);

	/* get file handle */
    fp = filp_open("/proc/cmdline", O_RDONLY | O_LARGEFILE, 0644);
    if (IS_ERR(fp))
    {
    	return -1;
    }

    pTmpbuf = vmalloc(MAX_IOMEM_SIZE);
    if(pTmpbuf == NULL)
    {
    	filp_close(fp, NULL);
    	return -1;
    }
    memset(pTmpbuf, 0, MAX_IOMEM_SIZE);    
 
    /* get file content */
    pos = 0;    
    fs = get_fs();
    set_fs(KERNEL_DS);
    vfs_read(fp, pTmpbuf, MAX_IOMEM_SIZE, &pos);
    set_fs(fs);    

    pstr = strstr(pTmpbuf, "mem=");
    if(pstr == NULL)
    {
        kfree(pTmpbuf);
        filp_close(fp, NULL);
        return -1;
    }
    pos = pstr - (char *)pTmpbuf;

    memset(s8TmpBuf, 0, sizeof(s8TmpBuf));
    memcpy(s8TmpBuf, pTmpbuf + pos + 4, 10);
    u32MmzStartAddress = simple_strtoul(s8TmpBuf, 0, 10);
    g_u32ModulePhyAddr = u32MmzStartAddress * 1024 * 1024;

    vfree(pTmpbuf);
    /* close file handle */
    filp_close(fp, NULL);     

    return 0;
}


void RuntimeModule_Exit(void)
{
    if(NULL != g_pModuleCopyThread)
    {
        kthread_stop(g_pModuleCopyThread);
    }
    
    if(NULL != g_pFsCheckThread)
    {
        kthread_stop(g_pFsCheckThread);
    }
    
    if(g_u32ModuleVirAddr != 0)
    {
        iounmap((void*)g_u32ModuleVirAddr);
    }
    
    if(g_u32C51CheckVectorVirAddr != 0)
    {
        iounmap((void*)g_u32C51CheckVectorVirAddr);
    }
}

module_init(RuntimeModule_Init);
module_exit(RuntimeModule_Exit);

MODULE_AUTHOR("HISILICON");
MODULE_LICENSE("GPL");

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

