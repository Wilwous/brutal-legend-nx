// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "runtime/horizon/generated.h"
#include "runtime/horizon/scene3d.h"
#include "runtime/horizon/platform_input.h"
#include "runtime/horizon/audio_mixer.h"
#include "runtime/horizon/resource_stream.h"
#include <memory>
namespace brutal::platform_v2 {
struct Stats {
 uint64_t ticks{},guest_calls{},reads{},writes{},hooks{},service_entered{},service_completed{},draws{};
 uint32_t result{},resource_bytes{},assets_ready{},audio_native_error{},cleanup_errors{},deferred_actions{};
 bool initialized{},scene_ready{},audio_available{},closed{},poisoned{},stopped{};
};
class Demo {
 struct Impl;std::unique_ptr<Impl> impl_;friend void poll(uint32_t);
 public:
 Demo(horizon::platform::Scene3D&,horizon::platform::Input&,horizon::platform::Mixer&,horizon::resources::Service&);
 ~Demo();Demo(const Demo&)=delete;
 void initialize();void tick();void shutdown();bool request_stop();Stats stats()const;
 void observer(std::function<void(uint32_t,horizon::Space&,horizon::Space::Handle)>);
 horizon::Space& memory();horizon::Space::Handle shared();PPCContext& context();
};
}
