/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-02-06
 *
******************************************************************************/


#include <linux/init.h>
#include <linux/module.h>
#include <linux/string_helpers.h>
#include "spi_ids.h"

/*****************************************************************************/

#if 1
#  define DBG_MSG(_fmt, arg...)
#else
#  define DBG_MSG(_fmt, arg...)   \
	printk(KERN_DEBUG "%s(%d): " _fmt, \
		__FILE__, __LINE__, ##arg);
#endif

#define DBG_BUG(fmt, args...) do { \
	printk(KERN_ERR "%s(%d): BUG !!! " fmt, \
		__FILE__, __LINE__, ##args); \
	while (1) { \
		; \
	} \
} while (0)

#define PR_MSG(_fmt, args...) \
	printk(KERN_INFO _fmt, ##args)

#define SPI_DRV_VERSION       "1.30"

/*****************************************************************************/

struct spi_info *spi_serach_ids(struct spi_info * spi_info_table,
				unsigned char ids[8])
{
	struct spi_info *info;
	struct spi_info *fit_info = NULL;

	for (info = spi_info_table; info->name; info++) {
		if (memcmp(info->id, ids, info->id_len))
			continue;

		if (fit_info == NULL
			|| fit_info->id_len < info->id_len)
			fit_info = info;
	}
	return fit_info;
}
/*****************************************************************************/

void spi_search_rw(struct spi_info *spiinfo,
		   struct spi_operation *spiop_rw,
		   unsigned int iftype,
		   unsigned int max_dummy,
		   int is_read)
{
	int ix = 0;
	struct spi_operation **spiop, **fitspiop;

	for (fitspiop = spiop = (is_read ? spiinfo->read : spiinfo->write);
		(*spiop) && ix < MAX_SPI_OP; spiop++, ix++) {
		DBG_MSG("dump[%d] %s iftype:0x%02X\n", ix,
			(is_read ? "read" : "write"),
			(*spiop)->iftype);

		if (((*spiop)->iftype & iftype)
			&& ((*spiop)->dummy <= max_dummy)
			&& (*fitspiop)->iftype < (*spiop)->iftype)
			fitspiop = spiop;
	}
	memcpy(spiop_rw, (*fitspiop), sizeof(struct spi_operation));
}
/*****************************************************************************/

void spi_get_erase(struct spi_info *spiinfo,
		   struct spi_operation *spiop_erase)
{
	int ix;

	spiop_erase->size = 0;
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spiinfo->erase[ix] == NULL)
			break;
		if (spiinfo->erasesize == spiinfo->erase[ix]->size) {
			memcpy(spiop_erase, spiinfo->erase[ix],
				sizeof(struct spi_operation));
			break;
		}
	}
	if (!spiop_erase->size)
		DBG_BUG("Spi erasesize error!");
}

/*****************************************************************************/

void spi_get_erase_sfcv300(struct spi_info *spiinfo,
			   struct spi_operation *spiop_erase,
			   unsigned int *erasesize)
{
	int ix;

	(*erasesize) = spiinfo->erasesize;
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spiinfo->erase[ix] == NULL)
			break;

		memcpy(&spiop_erase[ix], spiinfo->erase[ix],
			sizeof(struct spi_operation));

		switch (spiop_erase[ix].size) {
		case SPI_IF_ERASE_SECTOR:
			spiop_erase[ix].size = spiinfo->erasesize;
			break;
		case SPI_IF_ERASE_CHIP:
			spiop_erase[ix].size = spiinfo->chipsize;
			break;
		}


		if ((int)(spiop_erase[ix].size) < _2K) {
			char buf[20];
			DBG_BUG("erase block size mistaken: "
				"spi->erase[%d].size:%s\n",
				ix, ultohstr(spiop_erase[ix].size,
				buf, sizeof(buf)));
		}

		if (spiop_erase[ix].size < (*erasesize)) {
			(*erasesize) = spiop_erase[ix].size;
		}
	}
}

/*****************************************************************************/

static int __init spi_ids_init(void)
{
	PR_MSG("Spi id table Version %s\n", SPI_DRV_VERSION);
	return 0;
}
/*****************************************************************************/

static void __exit spi_ids_exit(void)
{
}
/*****************************************************************************/

module_init(spi_ids_init);
module_exit(spi_ids_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Czyong");
MODULE_DESCRIPTION("Spi id table");

