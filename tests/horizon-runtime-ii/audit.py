# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Selected-opcode admission plus the independent original site census."""
import importlib.util
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('previous_independent_census',ROOT/'tests/horizon-expansion/audit.py');prior=importlib.util.module_from_spec(s);s.loader.exec_module(prior)
def admit_word(word):
 op=word>>26
 if op in (11,14,15,24,32,36,37,58,62):return
 if op==18 and not(word&2):return
 if op==16 and not(word&3) and ((word>>21)&31,(word>>16)&31) in ((4,2),(12,2)):return
 if word in (0x4e800020,0x4e800421):return
 if op==31 and ((word>>1)&1023) in (10,138,266,339,444,467,986) and not(word&1):return
 if op==63 and ((word&0x7ff)==42 or (word&0x7f)==50):return
 raise ValueError('unsupported original instruction: '+hex(word))
def audit(meta,cpp):
 for word in meta['words']:admit_word(word)
 return prior.audit(meta,cpp)
