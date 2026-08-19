#!/usr/bin/env python3
# Rebuild A11 from exact A10 raw. Source logic is in a11_reset_guard.c.
# A11 base SHA256 expected: 35c6c5fa477e35c7985d390cfcf1dc08595debc03fda8b9c5c2411ecf03f9fee
# Patch site: 0x10005A30 old drain -> linked a11_guarded_drain.
# Link section base MUST remain 0x10006700; always place entire objcopy BIN there.
# See A11_BUILD_AUDIT.txt for the complete verification performed at release.
