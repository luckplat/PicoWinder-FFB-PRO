#!/usr/bin/env python3
# A12 release notes. Full source logic: a12_message_pacing.c
# Exact A11 base RAW SHA256: bbddb2d8b6840ce0000654d6679d4596f9178ac0a928fb754173e40d0de66ef1
# Patch: call @ 0x10005A30 -> linked a12_paced_drain.
# Link section base: 0x10006900. Place whole objcopy BIN at that section base.
# See A12_BUILD_AUDIT.txt before modifying/rebuilding.
