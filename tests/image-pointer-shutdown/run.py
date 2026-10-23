#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Use the Phase 16 production XEX/codegen workflow with Phase 17 contracts."""
import runpy,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'xex-binding'))
workflow=runpy.run_path(str(Path(__file__).resolve().parents[1]/'xex-binding/run.py'))
if __name__=='__main__':sys.exit(workflow['main'](shutdown=True))
