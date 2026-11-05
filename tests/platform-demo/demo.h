// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "runtime/horizon/generated.h"
#include "runtime/horizon/platform_graphics.h"
#include "runtime/horizon/platform_input.h"
#include "runtime/horizon/platform_audio.h"
#include "runtime/horizon/platform_resources.h"
#include <memory>
namespace brutal::platform_demo {
struct Stats {uint64_t frames{},guest_calls{},reads{},writes{},hooks{},service_entered{},service_completed{},audio_submitted{},audio_completed{};uint32_t result{},resource_bytes{},audio_native_error{},cleanup_errors{};bool audio_available{},poisoned{},closed{};};
class Demo {
 struct Impl;std::unique_ptr<Impl> impl_;friend void poll(uint32_t);
 public:
 Demo(horizon::platform::Graphics&,horizon::platform::Input&,horizon::platform::AudioService&,horizon::resources::Service&);
 ~Demo();Demo(const Demo&)=delete;
 void initialize();void frame();void shutdown();Stats stats()const;
 // Trusted synthetic test observer, called before native command validation.
 void observer(std::function<void(uint32_t,horizon::Space&,horizon::Space::Handle)>);
 horizon::Space& memory();horizon::Space::Handle shared();PPCContext& context();
};
std::array<horizon::resources::Manifest,1> manifest();
}
