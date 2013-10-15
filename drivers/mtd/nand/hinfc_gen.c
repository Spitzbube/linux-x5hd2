/******************************************************************************
*    Copyright (c) 2009-2011 by Hisi.
*    All rights reserved.
* ***
*    Create by Czyong. 2011-12-03
*
******************************************************************************/

#include "hinfc_gen_os.h"
#include "match_table.h"
#include "hinfc_gen.h"

/*****************************************************************************/

int (*nand_oob_resize)(struct mtd_info *mtd, struct nand_chip *chip,
	struct nand_flash_dev_ex *flash_dev_ex) = NULL;

/*****************************************************************************/

static struct match_type_str ecc2name[] = {
	{NAND_ECC_NONE, "none" },
	{NAND_ECC_1BIT, "1bit" },
	{NAND_ECC_4BIT, "4bit" },
	{NAND_ECC_8BIT, "8bit" },
	{NAND_ECC_24BIT, "24bit" },
	{NAND_ECC_40BIT, "40bit" },
	{NAND_ECC_4BYTE, "4byte" },
	{NAND_ECC_8BYTE, "8byte" },
	{NAND_ECC_13BIT, "13bit" },
	{NAND_ECC_18BIT, "18bit" },
	{NAND_ECC_27BIT, "27bit" },
	{NAND_ECC_32BIT, "32bit" },
	{NAND_ECC_41BIT, "41bit" },
	{NAND_ECC_48BIT, "48bit" },
	{NAND_ECC_60BIT, "60bit" },
	{NAND_ECC_72BIT, "72bit" },
	{NAND_ECC_80BIT, "80bit" },
};

const char *nand_ecc_name(int type)
{
	return type2str(ecc2name, ARRAY_SIZE(ecc2name), type, "unknown");
}
/*****************************************************************************/

static struct match_type_str page2name[] = {
	{ NAND_PAGE_512B, "512" },
	{ NAND_PAGE_2K,   "2K" },
	{ NAND_PAGE_4K,   "4K" },
	{ NAND_PAGE_8K,   "8K" },
	{ NAND_PAGE_16K,  "16K" },
	{ NAND_PAGE_32K,  "32K" },
};

const char *nand_page_name(int type)
{
	return type2str(page2name, ARRAY_SIZE(page2name), type, "unknown");
}
/*****************************************************************************/

static struct match_reg_type page2size[] = {
	{ _512B, NAND_PAGE_512B },
	{ _2K, NAND_PAGE_2K },
	{ _4K, NAND_PAGE_4K },
	{ _8K, NAND_PAGE_8K },
	{ _16K, NAND_PAGE_16K },
	{ _32K, NAND_PAGE_32K },
};

int nandpage_size2type(int size)
{
	return reg2type(page2size, ARRAY_SIZE(page2size), size, NAND_PAGE_2K);
}
