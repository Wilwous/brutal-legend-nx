#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Check the staged research tree without reading any private game directory.

This allowlist permits research, explicit original component sources and a pinned open-source SDK patch. It cannot
determine copyright ownership or guarantee that text contains no sensitive data.
"""

from pathlib import Path, PurePosixPath
import json
import re
import subprocess
import sys
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[1]
EXACT = {
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

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Checked {len(texts)} public research files and their local documentation links.")
    print("This is not a game build, copyright audit or complete secret scan.")
    return 0


if __name__ == "__main__":
    sys.exit(check())
