# Project Name
TARGET = DoubleExposure

# Sources
CPP_SOURCES = DoubleExposure.cpp

C_INCLUDES += -I../external
C_INCLUDES += -I../shared

C_DEFS += -DON_DAISY

# Library Locations
LIBDAISY_DIR = ../external/DaisyExamples/libDaisy
DAISYSP_DIR = ../external/DaisyExamples/DaisySP

$(info $(C_INCLUDES))

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
