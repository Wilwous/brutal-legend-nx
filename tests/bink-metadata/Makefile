# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
# ELF-only checks. No runnable package is produced.
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := metadata.elf
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := $(ARCH) -std=gnu++23 -O2 -ffunction-sections -fdata-sections -fexceptions -frtti -Wall -Wextra -Werror -D__SWITCH__ -isystem $(DEVKITPRO)/libnx/include -isystem /toolchain/include -iquote /src -I/generated -I/data
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -L$(DEVKITPRO)/libnx/lib
metadata.o: /src/tests/bink-metadata/contract.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
metadata.elf: metadata.o
	$(CXX) $(LDFLAGS) $^ -lnx -o $@
module.o: /src/tests/module-startup/host.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
leaf.o: /generated/fixture_recomp.0.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
mapping.o: /generated/fixture_init.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
hooks.o: /data/hooks.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
module.elf: module.o leaf.o mapping.o hooks.o
	$(CXX) $(LDFLAGS) $^ -lnx -o $@
