# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := all
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := $(ARCH) -std=gnu++23 -O2 -g -ffunction-sections -fdata-sections -fexceptions -frtti -Wall -Wextra -Werror -D__SWITCH__ -DSTAGE_MAX=$(STAGE_MAX) -isystem $(DEVKITPRO)/libnx/include -isystem /toolchain/include -iquote $(REPO) -I/generated -I/data -ffile-prefix-map=$(REPO)=/src -ffile-prefix-map=$(CURDIR)=/build
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -Wl,-Map,probe.map -L$(DEVKITPRO)/libnx/lib
LD := $(CXX)
OFILES := main.o probe.o leaf.o mapping.o hooks.o import.o
ifeq ($(shell test $(STAGE_MAX) -ge 7 && echo yes),yes)
OFILES += initialize.o
endif
initialize.o: $(REPO)/tests/horizon-expansion/import.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
ifeq ($(shell test $(STAGE_MAX) -ge 10 && echo yes),yes)
OFILES += query.o
endif
query.o: $(REPO)/tests/horizon-runtime-ii/query.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
all: probe.nro
main.o: $(REPO)/tests/horizon-runtime-ii/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
probe.o: $(REPO)/tests/horizon-runtime-ii/probe.cpp
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
leaf.o: /generated/fixture_recomp.0.cpp
	$(CXX) $(CXXFLAGS) -fstack-usage -c $< -o $@
mapping.o: /generated/fixture_init.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
hooks.o: /data/hooks.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
import.o: $(REPO)/tests/horizon-xex/import.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
probe.elf: $(OFILES)
	$(LD) $(LDFLAGS) $(OFILES) -lnx -o $@
	python3 $(REPO)/tools/switch/sanitize_elf.py $@
probe.nacp:
	nacptool --create "Brutal Legend NX - Integrated Runtime II" "Wilwous" "0.11.0" $@
probe.nro: probe.elf probe.nacp $(ICON)
	elf2nro probe.elf $@ --nacp=probe.nacp $(if $(ICON),--icon=$(ICON))
.PHONY: all
