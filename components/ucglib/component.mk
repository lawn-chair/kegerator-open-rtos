# Component makefile for extras/rboot-ota

INC_DIRS += $(ucglib_ROOT)

ucglib_SRC_DIR = $(ucglib_ROOT)

ucglib_CFLAGS = $(CFLAGS) -DUSE_PIN_LIST

$(eval $(call component_compile_rules,ucglib))