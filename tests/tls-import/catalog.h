// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <array>
#include <cstdint>
namespace tls_fixture {
struct Import { const char* library; uint32_t ordinal; };
inline constexpr std::array<Import,33> matrix_imports{{
 {"xboxkrnl.exe",0x3}, // DbgPrint
 {"xboxkrnl.exe",0x10}, // ExGetXConfigSetting
 {"xboxkrnl.exe",0x28}, // HalReturnToFirmware
 {"xboxkrnl.exe",0x52}, // KeBugCheck
 {"xboxkrnl.exe",0x53}, // KeBugCheckEx
 {"xboxkrnl.exe",0x5a}, // KeDelayExecutionThread
 {"xboxkrnl.exe",0x66}, // KeGetCurrentProcessType
 {"xboxkrnl.exe",0x83}, // KeQueryPerformanceFrequency
 {"xboxkrnl.exe",0x152}, // KeTlsAlloc
 {"xboxkrnl.exe",0x153}, // KeTlsFree
 {"xboxkrnl.exe",0x154}, // KeTlsGetValue
 {"xboxkrnl.exe",0x155}, // KeTlsSetValue
 {"xboxkrnl.exe",0xcc}, // NtAllocateVirtualMemory
 {"xboxkrnl.exe",0xcf}, // NtClose
 {"xboxkrnl.exe",0xd1}, // NtCreateEvent
 {"xboxkrnl.exe",0xdc}, // NtFreeVirtualMemory
 {"xboxkrnl.exe",0xee}, // NtQueryVirtualMemory
 {"xboxkrnl.exe",0xfd}, // NtWaitForSingleObjectEx
 {"xboxkrnl.exe",0x11b}, // RtlCompareMemoryUlong
 {"xboxkrnl.exe",0x125}, // RtlEnterCriticalSection
 {"xboxkrnl.exe",0x12b}, // RtlImageXexHeaderField
 {"xboxkrnl.exe",0x12c}, // RtlInitAnsiString
 {"xboxkrnl.exe",0x12e}, // RtlInitializeCriticalSection
 {"xboxkrnl.exe",0x12f}, // RtlInitializeCriticalSectionAndSpinCount
 {"xboxkrnl.exe",0x130}, // RtlLeaveCriticalSection
 {"xboxkrnl.exe",0x135}, // RtlNtStatusToDosError
 {"xboxkrnl.exe",0x136}, // RtlRaiseException
 {"xam.xex",0x3cb}, // XGetAVPack
 {"xam.xex",0x3cd}, // XGetLanguage
 {"xam.xex",0x1a9}, // XamLoaderTerminateTitle
 {"xam.xex",0x2dc}, // XamShowMessageBoxUIEx
 {"xboxkrnl.exe",0x194}, // XexCheckExecutablePrivilege
 {"xboxkrnl.exe",0x195}, // XexGetModuleHandle
}};
}
