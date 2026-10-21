// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Compile-only compatibility of the SDK's other generated-PCH template.
#include "synthetic_test_pch.h"
void original_vector_load(PPCContext& ctx,uint8_t* base){REX_LOAD_VECTOR(ctx.v3,ctx.r4.u32);}
void original_vector_store(PPCContext& ctx,uint8_t* base){REX_STORE_VECTOR(ctx.r4.u32,ctx.v3);}
void original_partial_guard(){REX_PARTIAL_VECTOR_GUARD();}
