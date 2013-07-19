#
# drivers/devices/hisfc350/Makefile
#

obj-$(CONFIG_MTD_HISFC350)	+= hisfc350_spi_ids.o hisfc350.o hisfc350_os.o

ifdef CONFIG_ARCH_S40
obj-$(CONFIG_MTD_HISFC350)	+= hisfc350_s40.o
endif

ifdef CONFIG_ARCH_GODBOX
obj-$(CONFIG_MTD_HISFC350)	+= hisfc350_godbox.o
endif

