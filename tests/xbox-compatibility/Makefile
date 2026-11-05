# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
# Cross compile/link only: this target deliberately produces no NRO or package.
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := compatibility.elf
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := $(ARCH) -std=gnu++23 -O2 -g -ffunction-sections -fdata-sections -fexceptions -frtti -Wall -Wextra -Werror -D__SWITCH__ -isystem $(DEVKITPRO)/libnx/include -isystem /toolchain/include -iquote /src -I/generated -I/data -ffile-prefix-map=/src=/src -ffile-prefix-map=/build=/build
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -L$(DEVKITPRO)/libnx/lib -Wl,-Map,compatibility.map
host.o: /src/tests/xbox-compatibility/host.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
leaf.o: /generated/fixture_recomp.0.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
mapping.o: /generated/fixture_init.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
hooks.o: /data/hooks.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
compatibility.elf: host.o leaf.o mapping.o hooks.o
	$(CXX) $(LDFLAGS) $^ -lnx -o $@
