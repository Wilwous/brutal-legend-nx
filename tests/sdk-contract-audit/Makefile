# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
# Link the real existing SDK bridges and original generated corpus; ELF only.
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := audit.elf
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := $(ARCH) -std=gnu++23 -O2 -ffunction-sections -fdata-sections -fexceptions -frtti -Wall -Wextra -Werror -D__SWITCH__ -isystem $(DEVKITPRO)/libnx/include -isystem /toolchain/include -iquote /src -I/generated -I/data -ffile-prefix-map=/src=/src -ffile-prefix-map=/build=/build
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -L$(DEVKITPRO)/libnx/lib -Wl,-Map,audit.map
main.o: /src/tests/sdk-contract-audit/compare.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
compare.o: /src/tests/horizon-xex/import.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
initialize.o: /src/tests/horizon-expansion/import.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
query.o: /src/tests/horizon-runtime-ii/query.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
fill.o: /src/tests/horizon-acceptance/fill.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
leaf.o: /generated/fixture_recomp.0.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
mapping.o: /generated/fixture_init.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
hooks.o: /data/hooks.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
audit.elf: main.o compare.o initialize.o query.o fill.o leaf.o mapping.o hooks.o
	$(CXX) $(LDFLAGS) $^ -lnx -o $@
