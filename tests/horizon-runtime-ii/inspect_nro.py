#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import importlib.util
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'horizon-xex/inspect_nro.py'
s=importlib.util.spec_from_file_location('original_inspector',p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
m.inspect(('Brutal Legend NX - Integrated Runtime II','Wilwous','0.11.0'))
