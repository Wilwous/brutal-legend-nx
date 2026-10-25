# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original four-instruction fixture; no title bytes or assets."""
WORDS=(0x80640000,0x38630001,0x90640000,0x4e800020)
BINARY_SHA256='165a81367d48577ba244f980d7e8f521965d8815b2dfdee75c4502b1ac406a24'
def write(directory):
    directory.mkdir()
    (directory/'leaf.bin').write_bytes(b''.join(w.to_bytes(4,'big') for w in WORDS))
    (directory/'leaf.map').write_text('00000000 T test_leaf\n')
    (directory/'leaf.s').write_text('test_leaf:\n    lwz r3,0(r4)\n    addi r3,r3,1\n    stw r3,0(r4)\n    blr\n')
