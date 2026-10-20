// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <rex/ppc.h>
static uint64_t mixed(uint64_t a,PPCVRegister v,double b){return a+v.u32[0]+uint64_t(b);}
REX_HOOK(mixed_hook,mixed)
