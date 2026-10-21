// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <rex/ppc.h>
#include "runtime/generated_access.h"
#if VECTOR_CASE == 0
PPCVRegister unsupported(){return {};}
#elif VECTOR_CASE == 1
void unsupported(PPCVRegister){}
#else
PPCVRegister unsupported(uint64_t,double,PPCVRegister v){return v;}
#endif
void rejected(PPCContext& context){brutal::instrumentation::direct(&unsupported,context,nullptr);}
