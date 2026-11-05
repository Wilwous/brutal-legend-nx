#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Read-only graphics package identity check within the pinned build container."""
from pathlib import Path
import hashlib,json
root=Path('/opt/devkitpro/portlibs/switch');lock=json.loads((Path(__file__).parent/'graphics-lock.json').read_text())
for folder,rows in [('include',lock['headers']),('lib',lock['libraries'])]:
 for name,expected in rows.items():assert hashlib.sha256((root/folder/name).read_bytes()).hexdigest()==expected,name
for name,version in lock['packages'].items():assert Path('/opt/devkitpro/pacman/var/lib/pacman/local',name+'-'+version,'desc').is_file(),name
print('Pinned graphics headers, libraries and package identities verified.')
