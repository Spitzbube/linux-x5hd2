/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-07-05
 *
******************************************************************************/

#ifndef HINFC610_ECC_INFOH
#define HINFC610_ECC_INFOH

int hinfc610_ecc_info_init(struct hinfc_host *host);

struct nand_ctrl_info_t *hinfc610_get_best_ecc(struct mtd_info *mtd);

struct nand_ctrl_info_t *hinfc610_force_ecc(struct mtd_info *mtd, int pagesize,
					     int ecctype, char *cfgmsg,
					     int allow_pagediv);

#endif /* HINFC610_ECC_INFOH */
