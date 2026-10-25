# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
# Invoke from a private output directory with REPO=/readonly/source.
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := all
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := $(ARCH) -std=gnu++23 -O2 -g -ffunction-sections -fdata-sections -fexceptions -frtti -Wall -Wextra -Werror -D__SWITCH__ -isystem $(DEVKITPRO)/libnx/include -iquote $(REPO) -ffile-prefix-map=$(REPO)=/src -ffile-prefix-map=$(CURDIR)=/build
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -Wl,-Map,probe.map -L$(DEVKITPRO)/libnx/lib
OFILES := main.o probe.o
LIBS := -lnx
LD := $(CXX)
all: probe.nro
main.o: $(REPO)/tests/switch-horizon/main.cpp $(REPO)/tests/switch-horizon/probe.h $(REPO)/tests/switch-horizon/result_ui.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
probe.o: $(REPO)/tests/switch-horizon/probe.cpp $(REPO)/tests/switch-horizon/probe.h $(REPO)/runtime/checked_memory.h $(REPO)/runtime/checked_tls.h $(REPO)/runtime/cooperative_stop.h
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
probe.elf: $(OFILES) $(REPO)/tools/switch/sanitize_elf.py
	$(LD) $(LDFLAGS) $(OFILES) $(LIBS) -o $@
	python3 $(REPO)/tools/switch/sanitize_elf.py $@
probe.nacp:
	nacptool --create "Brutal Legend NX Probe" "Wilwous" "0.1.0" $@
probe.nro: probe.elf probe.nacp $(ICON)
	elf2nro probe.elf $@ --nacp=probe.nacp $(if $(ICON),--icon=$(ICON))
.PHONY: all
