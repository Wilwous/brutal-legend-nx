// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Expected compilation failure documents the unsupported typed vector bridge.
#include <rex/ppc.h>
static uint32_t vector_parameter(PPCVRegister value){return value.u32[0];}
REX_HOOK(unsupported_typed_vector,vector_parameter)
