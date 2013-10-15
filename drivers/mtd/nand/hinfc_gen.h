/******************************************************************************
 *    NAND Flash Controller V504 Device Driver
 *    Copyright (c) 2009-2010 by Hisilicon.
 *    All rights reserved.
 * ***
 *    Create By Czyong.
 *
******************************************************************************/

#ifndef HINFC_COMMONH
#define HINFC_COMMONH
/******************************************************************************/
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/string_helpers.h>
#include <asm/setup.h>
#include <linux/module.h>
/******************************************************************************/

#define HINFC_VER_300                   (0x300)
#define HINFC_VER_301                   (0x301)
#define HINFC_VER_310                   (0x310)
#define HINFC_VER_504                   (0x504)
#define HINFC_VER_505                   (0x505)
#define HINFC_VER_600                   (0x600)
#define HINFC_VER_610                   (0x610)

#define _512B                           (512)
#define _2K                             (2048)
#define _4K                             (4096)
#define _8K                             (8192)
#define _16K                            (16384)
#define _32K                            (32768)
/******************************************************************************/

#define NAND_PAGE_SHIFT                      9 /* 512 */
#define NAND_PAGE_512B                       0
#define NAND_PAGE_1K                         1
#define NAND_PAGE_2K                         2
#define NAND_PAGE_4K                         3
#define NAND_PAGE_8K                         4
#define NAND_PAGE_16K                        5
#define NAND_PAGE_32K                        6

#define NAND_ECC_NONE                        0
#define NAND_ECC_1BIT                        1
#define NAND_ECC_4BIT                        2
#define NAND_ECC_4BYTE                       3
#define NAND_ECC_8BIT                        4
#define NAND_ECC_8BYTE                       5
#define NAND_ECC_13BIT                       6
#define NAND_ECC_18BIT                       7
#define NAND_ECC_24BIT                       8
#define NAND_ECC_27BIT                       9
#define NAND_ECC_32BIT                       10
#define NAND_ECC_40BIT                       11
#define NAND_ECC_41BIT                       12
#define NAND_ECC_48BIT                       13
#define NAND_ECC_60BIT                       14
#define NAND_ECC_72BIT                       15
#define NAND_ECC_80BIT                       16

const char *nand_ecc_name(int type);

const char *nand_page_name(int type);

#define nandpage_type2size(_type) \
	(1 << (NAND_PAGE_SHIFT + _type))

int nandpage_size2type(int size);

struct nand_config_table {
	int pagetype;
	int ecctype;
	unsigned int oobsize;
	struct nand_ecclayout *layout;
};

struct hinfc_host;

struct read_retry_t {
	int type;
	int count;
	int (*set_rr_param)(struct hinfc_host *host, int param);
	int (*get_rr_param)(struct hinfc_host *host);
	int (*reset_rr_param)(struct hinfc_host *host);
};

struct nand_flash_dev_ex {
	struct nand_flash_dev flash_dev;

	char *start_type;
	unsigned char ids[8];
	int oobsize;
	int ecctype;

#define NAND_RANDOMIZER                0x01
#define NAND_HW_AUTO                   0x02
#define NAND_SYNCHRONOUS               0x04
#define NAND_ASYNCHRONOUS              0x08
#define NAND_SYNCHRONOUS_BOOT          0x10
#define NAND_CONFIG_DONE               0x20
	int flags;

#define NAND_RR_NONE                   0x00
#define NAND_RR_HYNIX_BG_BDIE          0x10
#define NAND_RR_HYNIX_BG_CDIE          0x11
#define NAND_RR_HYNIX_CG_ADIE          0x12
#define NAND_RR_MICRON                   0x2
#define NAND_RR_SAMSUNG                  0x3
#define NAND_RR_TOSHIBA_24nm             0x4
	int read_retry_type;
};

#define IS_RANDOMIZER(_dev)           ((_dev)->flags & NAND_RANDOMIZER)
#define IS_HW_AUTO(_dev)              ((_dev)->flags & NAND_HW_AUTO)
#define IS_SYNCHRONOUS(_dev)          ((_dev)->flags & NAND_SYNCHRONOUS)
#define IS_CONFIG_DONE(_dev)          ((_dev)->flags & NAND_CONFIG_DONE)
#define IS_SYNCHRONOUS_BOOT(_dev)     ((_dev)->flags & NAND_SYNCHRONOUS_BOOT)

#define ERSTR_HARDWARE  "Hardware configuration error. "
#define ERSTR_DRIVER    "Driver does not support. "

#define ENABLE                    1
#define DISABLE                   0

extern int (*nand_oob_resize)(struct mtd_info *mtd, struct nand_chip *chip,
	struct nand_flash_dev_ex *flash_dev_ex);

struct nand_flash_dev * nand_get_flash_type_ex(struct mtd_info *mtd,
	struct nand_chip *chip, struct nand_flash_dev_ex *flash_dev_ex);

void nand_show_info(struct nand_flash_dev_ex *flash_dev_ex,
	struct mtd_info *mtd, char *vendor, char *chipname);

void nand_show_chip(struct nand_chip *chip);

/******************************************************************************/
#endif /* HINFC_COMMONH */

