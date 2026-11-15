#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Artem Novak
# SPDX-License-Identifier: BSD-3-Clause
"""Generate the separate 1.5 MiB HH3 probe payload."""

from pathlib import Path
import sys

SIZE = 0x180000
MARKERS = {0x000000: 0x5A, 0x080000: 0xA5, 0x17FFFF: 0x3C}


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: make_romprobe.py OUTPUT")
    output = Path(sys.argv[1])
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as file:
        file.truncate(SIZE)
        for offset, value in MARKERS.items():
            file.seek(offset)
            file.write(bytes((value,)))
    assert output.stat().st_size == SIZE


if __name__ == "__main__":
    main()
