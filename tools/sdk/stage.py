#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Stage a private fixture SDK from freshly built outputs; never invoke global install rules."""
from pathlib import Path
import hashlib
import json
import shutil


def stage(source, build, prefix, config):
    source,build,prefix=map(Path,(source,build,prefix));inc=prefix/'include';lib=prefix/'lib'
    inc.mkdir(parents=True,exist_ok=True);lib.mkdir(parents=True,exist_ok=True)
    for relative,target in [
      ('include/rex','rex'),('thirdparty/fmt/include/fmt','fmt'),
      ('thirdparty/spdlog/include/spdlog','spdlog'),('thirdparty/simde/simde','simde'),
      ('thirdparty/tomlplusplus/include/toml++','toml++'),('thirdparty/sdl3/include/SDL3','SDL3'),
      ('thirdparty/vulkan-headers/include/vulkan','vulkan'),('thirdparty/vulkan-headers/include/vk_video','vk_video'),
      ('thirdparty/spirv-tools/include/spirv-tools','spirv-tools')]:
        shutil.copytree(source/relative,inc/target,dirs_exist_ok=True)
    shutil.copy2(build/'include/rex/version.h',inc/'rex/version.h')
    for file in (source/'thirdparty/imgui').glob('*.h'):shutil.copy2(file,inc/file.name)
    output=source/'out/mac-arm64'/config
    for file in output.iterdir():
        if file.is_file() and file.suffix in ['.a','.dylib']:shutil.copy2(file,lib/file.name)
    suffix='d' if config=='Debug' else 'rd' if config=='RelWithDebInfo' else ''
    runtime=lib/('librexruntime'+suffix+'.dylib')
    if not runtime.is_file():raise RuntimeError('newly built runtime missing')
    # Preserve the install-name filename, and provide the conventional linker name.
    if suffix:shutil.copy2(runtime,lib/'librexruntime.dylib')
    metadata={'config':config,'runtime_filename':runtime.name,'runtime_sha256':hashlib.sha256(runtime.read_bytes()).hexdigest(),
              'sdk_revision':'f5337cdc947ff6d4c4196737e2c807a48f2a1fc2'}
    (prefix/'fixture-sdk.json').write_text(json.dumps(metadata,indent=2)+'\n')
    return metadata
