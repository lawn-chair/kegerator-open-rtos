#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
RTOS_PATH=~/code/esp/esp-open-rtos

PROGRAM := kegerator

EXTRA_COMPONENTS = extras/libesphttpd extras/rboot-ota # extras/spiffs

LIBESPHTTPD_HTML_DIR = kegerator-web/dist

ESPPORT=/dev/cu.SLAB_USBtoUART
#FLASH_SIZE = 32

#SPIFFS_BASE_ADDR = 0x200000
#SPIFFS_SIZE = 0x100000

PROGRAM_CFLAGS += -DFREERTOS

LIBS += m

include $(RTOS_PATH)/common.mk

$(eval $(call make_spiffs_image,data))