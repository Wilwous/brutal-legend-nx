# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := all
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := $(ARCH) -std=gnu++23 -O2 -g -ffunction-sections -fdata-sections -fexceptions -frtti -Wall -Wextra -Werror -D__SWITCH__ -isystem $(DEVKITPRO)/libnx/include -isystem /toolchain/include -iquote $(REPO) -I/generated -ffile-prefix-map=$(REPO)=/src -ffile-prefix-map=$(CURDIR)=/build
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -Wl,-Map,probe.map -L$(DEVKITPRO)/libnx/lib
LD := $(CXX)
OFILES := main.o probe.o leaf.o
all: probe.nro
main.o: $(REPO)/tests/horizon-runtime/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
probe.o: $(REPO)/tests/horizon-runtime/probe.cpp
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
leaf.o: /generated/ppc_test_functions.cpp
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
probe.elf: $(OFILES)
	$(LD) $(LDFLAGS) $(OFILES) -lnx -o $@
	python3 $(REPO)/tools/switch/sanitize_elf.py $@
probe.nacp:
	nacptool --create "Brutal Legend NX - PPC Runtime Probe" "Wilwous" "0.2.0" $@
probe.nro: probe.elf probe.nacp $(ICON)
	elf2nro probe.elf $@ --nacp=probe.nacp $(if $(ICON),--icon=$(ICON))
.PHONY: all
