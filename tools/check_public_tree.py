#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Check the staged research tree without reading any private game directory.

This allowlist permits research, explicit original component sources and a pinned open-source SDK patch. It cannot
determine copyright ownership or guarantee that text contains no sensitive data.
"""

from pathlib import Path, PurePosixPath
import json
import hashlib
import re
import subprocess
import sys
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[1]
EXACT = {
    'runtime/horizon/native_image_binding.h',
    'tests/native-image-binding/probe.h',
    'tests/native-image-binding/probe.cpp',
    'tests/native-image-binding/host.cpp',
    'tests/native-image-binding/fixture.py',
    'tests/native-image-binding/run.py',
    'tests/native-image-binding/templates/codegen/pch_h.inja',
    'tests/native-image-binding/templates/codegen/init_cpp.inja',

    "runtime/horizon/native_image_plan.h",
    "tests/native-image-plan/run.py",
    "tests/native-image-plan/contract.cpp",
    "tests/native-image-contract/contract.cpp",
    "tests/native-image-contract/fixture.py",
    "tests/native-image-contract/run.py",
    'tests/loader-observation/README.md',
    'tests/loader-observation/NOTICE.txt',
    'tests/loader-observation/corpus.cpp',
    'tests/loader-observation/project.cpp',
    'tests/loader-observation/reference.cpp',
    'tests/loader-observation/extract.py',
    'tests/loader-observation/run.py',
    'tests/loader-observation/verify.py',
    'tests/loader-observation/test_corpus.py',
    'tests/loader-observation/defined-offset.patch',
    'tests/loader-observation/fixture-manifest.json',
    'tests/loader-observation/expected-observations.json',

    'tests/loader-observation/reference-lock.json',
    'tests/compatibility-services/contracts.cpp',
    'tests/compatibility-services/run.py',
    'tools/audit_xex_residency.py',
    'tests/residency-evidence/test_residency.py',
    'tests/residency-evidence/residency_contracts.cpp',
    'tests/residency-evidence/run.py',
    'tests/residency-evidence/sdk-lock.json',
    'tests/residency-evidence/load_probe.cpp',
    'tests/residency-evidence/README.md',

    'tests/horizon-acceptance/test_package.py',
    'tests/horizon-acceptance/test_deploy.py',
    'tests/horizon-acceptance/supervisor_core.py',
    'tests/horizon-acceptance/supervisor_core.cpp',
    'tests/horizon-acceptance/supervisor.h',
    'tests/horizon-acceptance/package.py',
    'tests/horizon-acceptance/deploy.py',
    'tests/horizon-acceptance/NOTICE.txt',
    'tests/horizon-acceptance/lifecycle_core.py',
    'tests/horizon-acceptance/lifecycle_core.cpp',
    'runtime/horizon/probe_control.h',
    'runtime/horizon/lifecycle.h',
    'runtime/horizon/fill.h',
    'tests/horizon-acceptance/sdk_fill.py',
    'tests/horizon-acceptance/fill.cpp',
    'tests/horizon-acceptance/fill-lock.json',
    'tests/horizon-acceptance/test_fixture.py',
    'tests/horizon-acceptance/Makefile',
    'tests/horizon-acceptance/audit.py',
    'tests/horizon-acceptance/compat-lock.json',
    'tests/horizon-acceptance/fixture.py',
    'tests/horizon-acceptance/host.cpp',
    'tests/horizon-acceptance/inspect_nro.py',
    'tests/horizon-acceptance/main.cpp',
    'tests/horizon-acceptance/probe.cpp',
    'tests/horizon-acceptance/probe.h',
    'tests/horizon-acceptance/run.py',
    'tests/horizon-acceptance/templates/codegen/init_cpp.inja',
    'tests/horizon-acceptance/templates/codegen/pch_h.inja',

    'tests/horizon-runtime-ii/package.py',
    'tests/horizon-runtime-ii/deploy.py',
    'tests/horizon-runtime-ii/test_package.py',
    'tests/horizon-runtime-ii/test_deploy.py',
    'tests/horizon-runtime-ii/NOTICE.txt',

    'tests/horizon-runtime-ii/benchmark.cpp',
    'tests/horizon-runtime-ii/benchmark.py',
    'tests/horizon-runtime-ii/diagnostics_core.cpp',
    'tests/horizon-runtime-ii/diagnostics_core.py',
    'runtime/diagnostics.h',
    'runtime/horizon/query.h',
    'tests/horizon-runtime-ii/query.cpp',
    'tests/horizon-runtime-ii/query-lock.json',
    'tests/horizon-runtime-ii/sdk_query.py',
    'tests/horizon-runtime-ii/query_core.cpp',
    'tests/horizon-runtime-ii/query_core.py',

    'tests/horizon-runtime-ii/audit.py',
    'tests/horizon-runtime-ii/test_fixture.py',
    'tests/horizon-runtime-ii/fixture.py',
    'tests/horizon-runtime-ii/probe.cpp',
    'tests/horizon-runtime-ii/probe.h',
    'tests/horizon-runtime-ii/host.cpp',
    'tests/horizon-runtime-ii/main.cpp',
    'tests/horizon-runtime-ii/Makefile',
    'tests/horizon-runtime-ii/run.py',
    'tests/horizon-runtime-ii/inspect_nro.py',
    'tests/horizon-runtime-ii/templates/codegen/pch_h.inja',
    'tests/horizon-runtime-ii/templates/codegen/init_cpp.inja',
    'tests/horizon-runtime-ii/compat-lock.json',

    'tests/horizon-expansion/NOTICE.txt',
    'tests/horizon-expansion/test_package.py',
    'tests/horizon-expansion/package.py',
    'tests/horizon-expansion/deploy.py',
    'tests/horizon-expansion/test_deploy.py',

    'tests/horizon-expansion/benchmark.cpp',
    'tests/horizon-expansion/benchmark.py',
    'tests/horizon-expansion/import.cpp',
    'tests/horizon-expansion/sdk-lock.json',
    'tests/horizon-expansion/test_fixture.py',
    'tests/horizon-expansion/fixture.py',
    'tests/horizon-expansion/audit.py',
    'tests/horizon-expansion/templates/codegen/pch_h.inja',
    'tests/horizon-expansion/templates/codegen/init_cpp.inja',

    'tests/horizon-expansion/probe.h',
    'tests/horizon-expansion/probe.cpp',
    'tests/horizon-expansion/main.cpp',
    'tests/horizon-expansion/host.cpp',
    'tests/horizon-expansion/run.py',
    'tests/horizon-expansion/Makefile',
    'tests/horizon-expansion/inspect_nro.py',

    'runtime/horizon/xex.h',
    'tests/horizon-xex/Makefile',
    'tests/horizon-xex/fixture.py',
    'tests/horizon-xex/host.cpp',
    'tests/horizon-xex/import.cpp',
    'tests/horizon-xex/inspect_nro.py',
    'tests/horizon-xex/main.cpp',
    'tests/horizon-xex/probe.cpp',
    'tests/horizon-xex/probe.h',
    'tests/horizon-xex/run.py',
    'tests/horizon-xex/sdk-lock.json',
    'tests/horizon-xex/templates/codegen/init_cpp.inja',
    'tests/horizon-xex/templates/codegen/pch_h.inja',
    'tests/horizon-xex/test_fixture.py',

    'runtime/horizon/generated.h',
    'runtime/horizon/guest.h',
    'runtime/horizon/native.h',
    'runtime/horizon/space.h',
    'tests/horizon-foundation/Makefile',
    'tests/horizon-foundation/core.cpp',
    'tests/horizon-foundation/core.py',
    'tests/horizon-foundation/fixture.py',
    'tests/horizon-foundation/host.cpp',
    'tests/horizon-foundation/inspect_nro.py',
    'tests/horizon-foundation/main.cpp',
    'tests/horizon-foundation/probe.cpp',
    'tests/horizon-foundation/probe.h',
    'tests/horizon-foundation/run.py',
    'tests/horizon-foundation/templates/test/ppc_config_h.inja',
    'tests/horizon-foundation/templates/test/ppc_test_functions_cpp.inja',
    'tools/switch/stage_package.py',

    'runtime/horizon/leaf.h',
    'patches/rexglue-sdk/horizon-leaf.patch',
    'tests/horizon-runtime/Makefile',
    'tests/horizon-runtime/NOTICE.txt',
    'tests/horizon-runtime/lock.json',
    'tests/horizon-runtime/fixture.py',
    'tests/horizon-runtime/prepare.py',
    'tests/horizon-runtime/run.py',
    'tests/horizon-runtime/probe.h',
    'tests/horizon-runtime/probe.cpp',
    'tests/horizon-runtime/main.cpp',
    'tests/horizon-runtime/host.cpp',
    'tests/horizon-runtime/inspect_nro.py',
    'tests/horizon-runtime/package.py',
    'tests/horizon-runtime/deploy.py',
    'tests/horizon-runtime/test_contract.py',
    'tests/horizon-runtime/templates/test/ppc_config_h.inja',
    'tests/horizon-runtime/templates/test/ppc_test_functions_cpp.inja',

    "tools/switch/sanitize_elf.py", "tests/switch-horizon/test_sanitize.py",
    "tools/switch/common.py", "tools/switch/deploy.py", "tools/switch/package.py",
    "tools/switch/icon.py", "tools/switch/requirements.txt", "tools/switch/NOTICE.txt",
    "tests/switch-horizon/result_ui.h", "tests/switch-horizon/ui.cpp",
    "tests/switch-horizon/test_deploy.py",
    "tests/switch-horizon/README.md", "tests/switch-horizon/Makefile",
    "tests/switch-horizon/probe.h", "tests/switch-horizon/probe.cpp",
    "tests/switch-horizon/host.cpp", "tests/switch-horizon/main.cpp",
    "tests/switch-horizon/toolchain.json", "tests/switch-horizon/run.py",
    "tests/switch-horizon/inspect_nro.py", "tests/switch-horizon/integration.py",
    "runtime/entry_contract.h", "tests/entry-contracts/README.md",
    "tests/entry-contracts/fixture.py", "tests/entry-contracts/contracts.cpp",
    "tests/entry-contracts/run.py", "tests/entry-contracts/test_fixture.py",
    "tools/bounded_cfg.py", "runtime/bounded_cfg.h",
    "tests/bounded-cfg/README.md", "tests/bounded-cfg/fixture.py",
    "tests/bounded-cfg/contracts.cpp", "tests/bounded-cfg/run.py", "tests/bounded-cfg/test_manifest.py",
    "tools/audit_readiness.py", "tests/readiness/test_audit.py",
    "runtime/generated_stop.h", "tests/generated-loop-stop/README.md",
    "tests/generated-loop-stop/fixture.py", "tests/generated-loop-stop/audit.py",
    "tests/generated-loop-stop/contracts.cpp", "tests/generated-loop-stop/run.py",
    "tests/generated-loop-stop/test_fixture.py",
    "runtime/cooperative_stop.h", "tests/image-pointer-shutdown/README.md",
    "tests/image-pointer-shutdown/contracts.cpp", "tests/image-pointer-shutdown/run.py",
    "runtime/synthetic_xex_binding.h", "tools/sdk/xex-binding-lock.json",
    "patches/rexglue-sdk/0019-leased-xex-copy.patch",
    "tests/xex-binding/README.md", "tests/xex-binding/fixture.py",
    "tests/xex-binding/contracts.cpp", "tests/xex-binding/run.py",
    "tests/xex-binding/workflow.py", "tests/xex-binding/test_fixture.py",
    "runtime/generated_imports.h", "tests/generated-dispatch/README.md",
    "tests/generated-dispatch/fixture.py", "tests/generated-dispatch/contracts.cpp",
    "tests/generated-dispatch/run.py", "tests/generated-dispatch/test_fixture.py",
    "runtime/pointer_imports.h", "tests/pointer-import/README.md",
    "tests/pointer-import/run.py", "tests/pointer-import/contracts.cpp",
    "runtime/import_admission.h", "tools/sdk/tls-import-lock.json",
    "patches/rexglue-sdk/0018-xex-tls-lifetime.patch",
    "tests/tls-import/README.md", "tests/tls-import/run.py", "tests/tls-import/audit.py",
    "tests/tls-import/contracts.cpp", "tests/tls-import/title-tls.cpp",
    "tests/tls-import/catalog.h", "tests/tls-import/fixture.h", "tests/tls-import/allocation-failure.cpp",
    "README.md", "LICENSE", "VERSION", ".gitignore", ".gitattributes", "CONTRIBUTING.md",
    "SECURITY.md", "THIRD_PARTY_NOTICES.md", "tools/check_public_tree.py",
    ".github/workflows/research.yml", ".github/ISSUE_TEMPLATE/research.yml",
    ".github/ISSUE_TEMPLATE/binary-analysis.yml", ".github/ISSUE_TEMPLATE/config.yml",
    "docs/research/reference-snapshots.json",
    "runtime/checked_memory.h", "runtime/checked_tls.h", "runtime/execution_boundary.h",
    "tests/runtime/run.py", "tests/runtime/harness.cpp", "tests/runtime/sdk_leaf_adapter.cpp",
    "tests/runtime/sdk_memory_probe.cpp", "tests/runtime/README.md",
    "patches/rexglue-sdk/0001-runtime-safety.patch", "patches/rexglue-sdk/LICENSE.BSD-3-Clause",
    "patches/rexglue-sdk/README.md",
    "tests/ppc/README.md", "tests/ppc/run.py", "tests/ppc/watchdog.py",
    "tests/ppc/test_watchdog.py", "tests/ppc/host_fault.cpp", "tests/ppc/execution.cpp",
    "patches/rexglue-sdk/0002-host-fault-and-protect.patch",
    "patches/rexglue-sdk/0003-ppc-callback-arguments.patch",
    "patches/rexglue-sdk/0004-initialization-failures.patch",
    "patches/rexglue-sdk/0005-signal-diagnostics.patch",
    "patches/rexglue-sdk/0006-darwin-signal-context.patch",
    "patches/rexglue-sdk/0007-defined-pe-header-offset.patch",
    "tools/sdk/README.md", "tools/sdk/build.py", "tools/sdk/stage.py",
    "tools/sdk/supervise.py", "tools/sdk/lock.json", "tools/sdk/requirements-macos.txt",
    "tests/unified/README.md", "tests/unified/abi.cpp", "tests/unified/errors.cpp",
    "tests/unified/faults.cpp", "tests/unified/emitter.cpp", "tests/unified/run.py",
    "tests/unified/vector-negative.cpp", "tests/unified/loader.cpp", "tests/unified/loader.py",
    "patches/rexglue-sdk/0008-xdbf-byte-parser.patch",
    "patches/rexglue-sdk/0009-guest-memory-ownership.patch",
    "patches/rexglue-sdk/0010-allocation-and-exception-failures.patch",
    "patches/rexglue-sdk/0011-checked-allocation-ownership.patch",
    "tools/sdk/pre-entry-lock.json", "tests/pre-entry/README.md", "tests/pre-entry/run.py",
    "tests/pre-entry/xdbf.cpp", "tests/pre-entry/contracts.cpp", "tests/pre-entry/memory-faults.cpp",
    "tests/pre-entry/vector-context.cpp", "tests/pre-entry/vector-negative-return.cpp",
    "tests/pre-entry/vector-negative-mixed.cpp", "tests/pre-entry/allocation-boundaries.cpp",
    "runtime/sdk_checked_memory.h", "tools/audit_entry.py",
    "tools/sdk/entry-readiness-lock.json", "patches/rexglue-sdk/0012-checked-guest-copy.patch",
    "patches/rexglue-sdk/0013-closed-handle-failure.patch",
    "tests/guest-memory/README.md", "tests/guest-memory/run.py",
    "tests/guest-memory/contracts.cpp", "tests/guest-memory/imports.cpp",
    "tests/guest-memory/test_audit.py",
    "runtime/generated_access.h", "tools/audit_instrumentation.py",
    "tools/sdk/instrumentation-lock.json", "patches/rexglue-sdk/0014-generated-access-seam.patch",
    "tests/instrumentation/README.md", "tests/instrumentation/run.py",
    "tests/instrumentation/contracts.cpp", "tests/instrumentation/emitter.cpp",
    "tests/instrumentation/vector-negative.cpp", "tests/instrumentation/test-template.cpp",
    "tests/instrumentation/test_audit.py",
    "tools/sdk/synthetic-lock.json", "patches/rexglue-sdk/0015-checked-virtual-stack-copy.patch",
    "tests/synthetic/fixture.py", "tests/synthetic/module.h", "tests/synthetic/contracts.cpp",
    "tests/synthetic/run.py", "tests/synthetic/test_fixture.py", "tests/synthetic/README.md",
    "tests/synthetic/lifetime.cpp", "tests/synthetic/create-failure.cpp",
    "tests/synthetic/reference-probe.cpp", "tests/synthetic/reference-trace.py",
    "patches/rexglue-sdk/0016-xthread-owned-exit.patch",
    "tools/sdk/image-runtime-lock.json",
    "patches/rexglue-sdk/0017-xex-transaction-ownership.patch",
    "tests/image-runtime/ownership.cpp", "tests/image-runtime/allocation-failure.cpp",
    "tests/image-runtime/README.md",
    "tests/image-runtime/run.py",
    "tests/image-runtime/audit.py",
    "tests/image-runtime/owned-module.h",
    "tests/image-runtime/thread-tls.h",
    "tests/image-runtime/contracts.cpp",
    "tests/image-runtime/imports.cpp",
    "tests/image-runtime/xex.cpp",
    "tests/image-runtime/xex-fixture.h",
    "tests/image-runtime/import-contracts.json",
    "tests/vmx128/compare_evidence.py",
    "tests/vmx128/run.py", "tests/vmx128/emit_fixture.cpp", "tests/vmx128/README.md",
}
PRIVATE_PATH = re.compile(r"/(?:Users|home)/[^\s/]+/|[A-Za-z]:\\Users\\[^\s\\]+\\")
SECRET = re.compile(r"gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----")
LINK = re.compile(r"\[[^\]\n]*\]\(([^)\n]+)\)")


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args])


def check():
    errors = []
    entries = git("ls-files", "--stage", "-z").split(b"\0")
    texts = {}
    for entry in entries:
        if not entry:
            continue
        metadata, raw_name = entry.split(b"\t", 1)
        mode, oid, stage = metadata.decode("ascii").split()
        name = raw_name.decode("utf-8")
        path = PurePosixPath(name)
        if mode not in {"100644", "100755"} or stage != "0":
            errors.append(f"{name}: symlink, submodule or unmerged entry is forbidden")
            continue
        allowed_doc = name.startswith("docs/") and path.suffix == ".md"
        if name not in EXACT and not allowed_doc:
            errors.append(f"{name}: outside the research-stage allowlist")
            continue
        data = git("cat-file", "blob", oid)
        if len(data) > 512 * 1024 or b"\0" in data:
            errors.append(f"{name}: oversized or binary content")
            continue
        try:
            content = data.decode("utf-8")
        except UnicodeDecodeError:
            errors.append(f"{name}: invalid UTF-8")
            continue
        if PRIVATE_PATH.search(content) or SECRET.search(content):
            errors.append(f"{name}: possible private path or credential; inspect locally")
        texts[name] = content

    if not texts:
        errors.append("no public files staged/tracked")
    for name, content in texts.items():
        if not name.endswith(".md"):
            continue
        for target in LINK.findall(content):
            target = target.strip().strip("<>")
            if re.match(r"[A-Za-z][A-Za-z0-9+.-]*:", target) or target.startswith("#"):
                continue
            target = unquote(target.split("#", 1)[0])
            resolved = (ROOT / name).parent.joinpath(target).resolve()
            try:
                relative = resolved.relative_to(ROOT).as_posix()
            except ValueError:
                errors.append(f"{name}: local link leaves the checkout")
                continue
            if relative not in texts:
                errors.append(f"{name}: local link target is not in the checked tree: {target}")

    snapshot = texts.get("docs/research/reference-snapshots.json")
    if snapshot:
        try:
            parsed = json.loads(snapshot)
            for row in parsed["repositories"]:
                if not re.fullmatch(r"[0-9a-f]{40}", row["commit"]):
                    raise ValueError("invalid reference commit")
        except (ValueError, KeyError, TypeError) as exc:
            errors.append(f"reference snapshot invalid: {exc}")

    for name in ["tools/sdk/lock.json", "tools/sdk/pre-entry-lock.json", "tools/sdk/entry-readiness-lock.json", "tools/sdk/instrumentation-lock.json", "tools/sdk/synthetic-lock.json", "tools/sdk/image-runtime-lock.json", "tools/sdk/tls-import-lock.json", "tools/sdk/xex-binding-lock.json"]:
        if name not in texts:
            continue
        try:
            locked = json.loads(texts[name])
            if not re.fullmatch(r"[0-9a-f]{40}", locked["sdk_revision"]):
                raise ValueError("invalid SDK revision")
            for patch in locked["patches"]:
                source = texts[patch["path"]].encode("utf-8")
                if hashlib.sha256(source).hexdigest() != patch["sha256"]:
                    raise ValueError("patch fingerprint mismatch: " + patch["path"])
        except (ValueError, KeyError, TypeError) as exc:
            errors.append(f"{name}: invalid patch lock: {exc}")

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Checked {len(texts)} public research files and their local documentation links.")
    print("This is not a game build, copyright audit or complete secret scan.")
    return 0


if __name__ == "__main__":
    sys.exit(check())
