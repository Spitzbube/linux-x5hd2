#ifndef __HIGMAC_FEPHY_FIX_H
#define __HIGMAC_FEPHY_FIX_H

#ifndef CONFIG_S40_FPGA
#define MII_MISC_CTL           0x16
#define MII_EXPMD              0x1D
#define MII_EXPMA              0x1E

#define XMODE_8bit_RW_NO_AUTO_INC       0x0
#define XMODE_8bit_RW_AUTO_INC          0x1
#define XMODE_16bit_RW_AUTO_INC         0x2
#define XMODE_8bit_WO_BUNDLED_ADDR      0x3

int set_phy_expanded_access_mode(struct phy_device *phy_dev, int access_mode);
int phy_expanded_read(struct phy_device *phy_dev, u32 reg_addr);
int phy_expanded_write(struct phy_device *phy_dev, u32 reg_addr, u16 val);
int phy_expanded_write_bulk(struct phy_device *phy_dev, u32 reg_and_val[], int count);
void higmac_internal_fephy_performance_fix(struct higmac_netdev_local *ld);
#endif

#endif

