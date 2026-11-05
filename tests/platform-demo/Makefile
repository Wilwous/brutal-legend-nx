# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := all
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := $(ARCH) -std=gnu++23 -O2 -g -ffunction-sections -fdata-sections -fexceptions -frtti -Wall -Wextra -Werror -MMD -MP -D__SWITCH__ -isystem $(DEVKITPRO)/libnx/include -isystem /toolchain/include -iquote /src -I/generated -I/data -ffile-prefix-map=/src=/src -ffile-prefix-map=/build=/build
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -Wl,-Map,probe.map -L$(DEVKITPRO)/libnx/lib
main.o: /src/tests/platform-demo/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
demo.o: /src/tests/platform-demo/demo.cpp
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
leaf.o: /generated/fixture_recomp.0.cpp
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
mapping.o: /generated/fixture_init.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
hooks.o: /data/hooks.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
probe.elf: main.o demo.o leaf.o mapping.o hooks.o
	$(CXX) $(LDFLAGS) $^ -lnx -o $@
	python3 /src/tools/switch/sanitize_elf.py $@
	$(OBJCOPY) --strip-debug $@
probe.nacp:
	nacptool --create "Brutal NX - Synthetic Platform Demo" "Wilwous" "0.85.0" $@
probe.nro: probe.elf probe.nacp /icon.jpg /romfs/resources/demo.txt
	elf2nro probe.elf $@ --nacp=probe.nacp --icon=/icon.jpg --romfsdir=/romfs
all: probe.nro
.PHONY: all
-include main.d demo.d leaf.d mapping.d hooks.d
