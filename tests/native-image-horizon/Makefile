# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := all
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := $(ARCH) -std=gnu++23 -O2 -g -ffunction-sections -fdata-sections -fexceptions -frtti -Wall -Wextra -Werror -D__SWITCH__ -isystem $(DEVKITPRO)/libnx/include -isystem /toolchain/include -iquote /src -I/generated -I/data -ffile-prefix-map=/src=/src -ffile-prefix-map=/build=/build
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -Wl,-Map,probe.map -L$(DEVKITPRO)/libnx/lib
main.o: /src/tests/native-image-horizon/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
probe.o: /src/tests/native-image-binding/probe.cpp
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
leaf.o: /generated/fixture_recomp.0.cpp
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
mapping.o: /generated/fixture_init.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
hooks.o: /data/hooks.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
probe.elf: main.o probe.o leaf.o mapping.o hooks.o
	$(CXX) $(LDFLAGS) $^ -lnx -o $@
	python3 /src/tools/switch/sanitize_elf.py $@
probe.nacp:
	nacptool --create "Brutal Legend NX - Native Image Probe" "Wilwous" "0.1.0-n3" $@
probe.nro: probe.elf probe.nacp /icon.jpg
	elf2nro probe.elf $@ --nacp=probe.nacp --icon=/icon.jpg
all: probe.nro
.PHONY: all
