TARGET = LegioRotary
CPP_SOURCES = LegioRotary.cpp
LIBDAISY_DIR ?= ../../work/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
APP_TYPE = BOOT_NONE
include $(SYSTEM_FILES_DIR)/Makefile
