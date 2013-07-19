
#define HI_REG_CRG_BASE                 IO_ADDRESS(0xF8A22000)
#define HI_SATA3_CTRL                   (HI_REG_CRG_BASE + 0xA8)
#define HI_SATA3_PHY                    (HI_REG_CRG_BASE + 0xAC)

#define HI_SATA3_CKO_ALIVE_SRST         (1 << 9)
#define HI_SATA3_BUS_SRST               (1 << 8)
#define HI_SATA3_REFCLK_CKEN            (1<<4)
#define HI_SATA3_MPLL_DWORD_CKEN        (1<<3)
#define HI_SATA3_CKO_ALIVE_CKEN         (1<<2)
#define HI_SATA3_RX0_CKEN               (1<<1)
#define HI_SATA3_BUS_CKEN               (1<<0)

#define HI_SATA3_PHY_REFCLK_SEL         (1<<8)
#define HI_SATA3_PHY_REF_CKEN           (1<<0)
