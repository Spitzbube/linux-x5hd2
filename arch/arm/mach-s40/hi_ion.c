#include <linux/ion.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/slab.h>

#define ION_TYPE_LEN	16
#define ION_NAME_LEN	64
#define ION_DATA_REGION_MAX	16
#define HI_NUM_ION_HEAPS	2

#define MMZ_SETUP_CMDLINE_LEN	256

#define HI_ION_DEVICE_NAME	"hisi-ion"

struct hisi_ion_data {
	char type[ION_TYPE_LEN];
	char name[ION_NAME_LEN];
	ion_phys_addr_t base;
	size_t size;
	struct platform_device *pdev;
};

static  struct hisi_ion_data  ion_data[ION_DATA_REGION_MAX];
static int num_ion_data;
static int ion_use_bootargs;

#ifdef CONFIG_MMZ_PARAM
static char __initdata ion_default_cmd[MMZ_SETUP_CMDLINE_LEN] = CONFIG_MMZ_PARAM;
#else
static char __initdata ion_default_cmd[MMZ_SETUP_CMDLINE_LEN] = "ddr,0,0,160M";
#endif


#define ION_HEAP_TYPE_NAME_CMA		"cma"
#define ION_HEAP_TYPE_NAME_CARVEOUT	"carveout"
#define ION_HEAP_TYPE_NAME_KMALLOC	"kmalloc"
#define ION_HEAP_TYPE_NAME_CHUNK	"chunk"

#define HISI_ION_HEAP_NAME(name)	godbox_ion_heap_##name
#define HISI_ION_HEAP(heap_name)	\
static struct ion_platform_heap HISI_ION_HEAP_NAME(heap_name) = {	\
	.id = ION_HIS_ID_##heap_name,\
	.name = ION_HIS_NAME_##heap_name, \
	.type = ION_HEAP_TYPE_DMA,	\
	.base = 0,	\
	.size = 0,		\
}


#define HISI_ION_DATA_NAME(name)	godbox_ion_data_##name
#define HISI_ION_DATA(name)	\
static struct ion_platform_data HISI_ION_DATA_NAME(name) = {	\
	.heaps = &godbox_ion_heap_##name,	\
	.nr = 1,				\
}

static u64  dma_mask = DMA_BIT_MASK(64);

#define HISI_ION_DEVICE_NAME(name)	godbox_ion_device_##name
#define HISI_ION_DEVICE(device_name, device_id)	\
static struct platform_device	HISI_ION_DEVICE_NAME(device_name) = { \
	.name = HI_ION_DEVICE_NAME,\
	.id = device_id,		\
	.dev = {	\
		.dma_mask = &dma_mask,   \
		.coherent_dma_mask = DMA_BIT_MASK(64),\
		.platform_data = &godbox_ion_data_##device_name,\
	},\
	.num_resources = 0,\
}

/** ion heap defination **/
HISI_ION_HEAP(JPEG);
HISI_ION_HEAP(VDEC);
HISI_ION_HEAP(FB);
HISI_ION_HEAP(GPU);
HISI_ION_HEAP(COMMON);
HISI_ION_HEAP(DDR);

/** ion platform data defination **/
HISI_ION_DATA(JPEG);
HISI_ION_DATA(VDEC);
HISI_ION_DATA(FB);
HISI_ION_DATA(GPU);
HISI_ION_DATA(COMMON);
HISI_ION_DATA(DDR);

/** ion platform device defination,id must be different **/
HISI_ION_DEVICE(JPEG, 1);
HISI_ION_DEVICE(VDEC, 2);
HISI_ION_DEVICE(FB, 3);
HISI_ION_DEVICE(GPU, 4);
HISI_ION_DEVICE(COMMON, 5);
HISI_ION_DEVICE(DDR, 6);

extern struct device *hisi_get_cma_device(const char *name);

static int __init hisi_ion_parse_cmdline(char *s);

static struct ion_platform_heap hi_ion_heaps[] = {
	[0] = {
		.id     = ION_HEAP_TYPE_SYSTEM,
		.type   = ION_HEAP_TYPE_SYSTEM,
		.name   = "vmalloc",
	},
	[1] = {
		.id     = ION_HEAP_TYPE_SYSTEM_CONTIG,
		.type   = ION_HEAP_TYPE_SYSTEM_CONTIG,
		.name   = "kmalloc",
	}
};

static struct ion_platform_data hi_ion_pdata = {
	.nr = HI_NUM_ION_HEAPS,
	.heaps = hi_ion_heaps,
};

static struct platform_device hi_ion_dev = {
	.name = HI_ION_DEVICE_NAME,
	.id = 1,
	.dev = { .platform_data = &hi_ion_pdata },
};

/**  borrow from hisi mmz driver **/
static unsigned long _strtoul_ex(const char *s, char **ep, unsigned int base)
{
		char *__end_p;
		unsigned long __value;

		__value = simple_strtoul(s, &__end_p, base);

		switch (*__end_p) {
		case 'm':
		case 'M':
			__value <<= 10;
		case 'k':
		case 'K':
			__value <<= 10;
			if (ep)
				(*ep) = __end_p + 1;
		default:
			break;
		}

		return __value;
}

struct platform_device *hisi_init_ion_device(struct hisi_ion_data *idata)
{
	char *name = idata->name;
	struct platform_device *pdev;
	struct ion_platform_heap *heap = NULL;
	struct device *tmpdev;

	if (strcmp(name, ION_HIS_NAME_JPEG) == 0)
		pdev = &godbox_ion_device_JPEG;
	else if (strcmp(name, ION_HIS_NAME_VDEC) == 0)
		pdev = &godbox_ion_device_VDEC;
	else if (strcmp(name, ION_HIS_NAME_FB) == 0)
		pdev = &godbox_ion_device_FB;
	else if (strcmp(name, ION_HIS_NAME_GPU) == 0)
		pdev = &godbox_ion_device_GPU;
	else if (strcmp(name, ION_HIS_NAME_COMMON) == 0)
		pdev = &godbox_ion_device_COMMON;
	else if (strcmp(name, ION_HIS_NAME_DDR) == 0)
		pdev = &godbox_ion_device_DDR;
	else
		return NULL;


	heap = ((struct ion_platform_data *)(pdev->dev.platform_data))->heaps;
	heap->base = idata->base;
	heap->size = idata->size;

	/* get cma device for ion heap later accesss to cma  */
	tmpdev = hisi_get_cma_device(name);
	/** initialize dma_mask erea for dma_alloc_coherent interface **/
	if (tmpdev) {
		tmpdev->dma_mask = &dma_mask;
		tmpdev->coherent_dma_mask = DMA_BIT_MASK(64);
	}
	heap->priv = tmpdev;
	if (heap->priv == NULL)
		printk(KERN_ERR"ion heap:%s get no cma device\n", name);

	if (strcmp(name, ION_HIS_NAME_COMMON) == 0)
		heap->priv = NULL;
	/* heap->type = ?  FIXME only support cma heap now */

	return pdev;
}

int  hisi_register_ion_device(void)
{
	int ret = 0;
	int i, j;

	for (i = 0; i < num_ion_data; i++) {
		ret = platform_device_register(ion_data[i].pdev);
		if (ret)
			goto REGISTER_ION_DEVICE_FAIL;
	}

	ret = platform_device_register(&hi_ion_dev);
	if (ret)
		goto REGISTER_ION_DEVICE_FAIL;

	return ret;


REGISTER_ION_DEVICE_FAIL:
	for (j = 0; j < i; j++) {
		platform_device_unregister(ion_data[j].pdev);
	}
	return ret;;
}

static int init_common_ion_heap(void)
{
	if (ion_use_bootargs == 0)
		hisi_ion_parse_cmdline(ion_default_cmd);

	strlcpy(ion_data[num_ion_data].name, ION_HIS_NAME_COMMON, ION_NAME_LEN);
	strlcpy(ion_data[num_ion_data].type, "cma", ION_TYPE_LEN);
	ion_data[num_ion_data].pdev = hisi_init_ion_device(&ion_data[num_ion_data]);
	num_ion_data++;

	return 0;
}

int hisi_declare_ion_memory(void)
{
	int i ;

	init_common_ion_heap();

	for (i = 0; i < num_ion_data; i++) {
		if (strcmp(ion_data[i].type, "cma")) {
			printk(KERN_ERR "now only cma ion heap for hisi platform,so ignore\n");
			continue;
		}
		printk(KERN_ERR"ion platform device:%pf\n", &ion_data[i].pdev->dev);
#if 0
		if (strcmp(ion_data[i].name, ION_HIS_NAME_COMMON) == 0)
			continue;
		ret = dma_declare_contiguous(&ion_data[i].pdev->dev, ion_data[i].size, ion_data[i].base, 0);
		if (ret)
			panic("declare configuous memory name :%s  type :%s base: %lu size :%dMB failed", \
				ion_data[i].name, ion_data[i].type, ion_data[i].base, ion_data[i].size>>20);
#endif
	}

	return 0;
}

/**  borrow from hisi mmz driver **/
static int __init hisi_ion_parse_cmdline(char *s)
{
	char *line, *tmp;
	char tmpline[256];

	strncpy(tmpline, s, sizeof(tmpline));
	tmpline[sizeof(tmpline)-1] = '\0';
	tmp = tmpline;

	while ((line = strsep(&tmp, ":")) != NULL) {
		int i;
		char *argv[6];

		for (i = 0; (argv[i] = strsep(&line, ",")) != NULL;)
			if (++i == ARRAY_SIZE(argv))
				break;

		if (i == 4) {
			strlcpy(ion_data[num_ion_data].name, argv[0], ION_NAME_LEN);
			strlcpy(ion_data[num_ion_data].type, "cma", ION_TYPE_LEN);
			ion_data[num_ion_data].base = _strtoul_ex(argv[2], NULL, 0);
			ion_data[num_ion_data].size = _strtoul_ex(argv[3], NULL, 0);
			ion_data[num_ion_data].pdev = hisi_init_ion_device(&ion_data[num_ion_data]);
			if (!ion_data[num_ion_data].pdev)
				panic("alloc ion platform device failed\n");
			num_ion_data++;
		} else {
			printk(KERN_ERR"hisi ion parameter is not correct\n");
			continue;
		}

	}

	if (num_ion_data != 0)
		ion_use_bootargs = 1;

	return 0;
}
early_param("mmz", hisi_ion_parse_cmdline);

