// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <rex/ppc.h>
static PPCVRegister vector_return(){PPCVRegister v{};return v;}
REX_HOOK(vector_return_hook,vector_return)
