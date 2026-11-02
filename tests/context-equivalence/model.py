# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Algebra/observation specification, not a PPC interpreter or admission API."""
from dataclasses import dataclass

@dataclass(frozen=True)
class Linear:
    s: int=0
    candidate: int=0
    constant: int=0
    def __add__(self,b):return Linear(self.s+b.s,self.candidate+b.candidate,self.constant+b.constant)
    def __sub__(self,b):return Linear(self.s-b.s,self.candidate-b.candidate,self.constant-b.constant)

STORES=((-40,8),(-32,8),(-24,8),(-16,8),(-8,4),(-496,4))
def obligations():
    delta=Linear(-1,1,0)
    identities=[Linear(1,0,o)+delta-Linear(0,1,o) for o,_ in STORES]
    assert all(x==Linear() for x in identities)
    assert Linear(1,0,-496)-Linear(1,0,0)==Linear(0,0,-496)
    assert Linear(0,1,-496)-Linear(0,1,0)==Linear(0,0,-496)
    assert Linear(1,0,0)!=Linear(0,1,0) # raw backchain equality is NOT an identity
    return {'effective_address_translation':'checked integer linear identity',
            'frame_offset':-496,'typed_backchain':'entry stack pointer only',
            'raw_byte_equality':False,'execution_admitted':False,
            'preconditions':['32-bit representable aligned pointers and no wrap',
             'owned committed writable spans, same endian/width semantics',
             'equal opaque payload/LR inputs and related untouched state',
             'explicitly proved pointer-field types; no heuristic rewriting',
             'nonalias with fixed memory/code/PCR/TLS',
             'closed normal trace with no absolute comparison or persistent pointer escape',
             'no unproved call, exceptional observer or continuation'],
            'scope':'conditional observation relation only; does not instantiate title inputs'}

def layout(sp,base,size):
    if sp<0 or sp>0xffffffff or sp%16 or base<0 or size<=0 or base+size>2**32:return False
    return all(base<=sp+o and sp+o+w<=base+size and (sp+o)%w==0 for o,w in STORES)

def normalize_stack(raw,base,sp):
    """Only the externally proved four-byte backchain field is renamed.

    Payload words which happen to resemble addresses are never guessed pointers.
    Persistent memory is deliberately outside this function's domain.
    """
    if not layout(sp,base,len(raw)):raise ValueError('invalid normalization layout')
    at=sp-base-496
    if int.from_bytes(raw[at:at+4],'big')!=sp:raise ValueError('unproved backchain value')
    result=bytearray(raw);result[at:at+4]=bytes(4);return bytes(result)
