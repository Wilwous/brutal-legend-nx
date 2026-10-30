// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include <iostream>
int main(){auto r=native_probe();std::cout<<"{\"positive\":"<<r.positive<<",\"negative\":"<<r.negative<<",\"failed\":"<<r.failed<<",\"result\":"<<r.actual<<",\"sdk_entered\":"<<r.sdk_entered<<",\"sdk_completed\":"<<r.sdk_completed<<",\"functions\":"<<r.functions<<",\"remaining\":"<<r.remaining<<",\"leases\":"<<r.leases<<"}\n";return r.failed?1:0;}
