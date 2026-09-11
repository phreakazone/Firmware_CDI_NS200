#!/usr/bin/env python3
import pathlib, sys

boot = pathlib.Path(sys.argv[1]).read_bytes()
app = pathlib.Path(sys.argv[2]).read_bytes()
out = pathlib.Path(sys.argv[3])
if len(boot) > 0x8000:
    raise SystemExit("bootloader exceeds 32 KiB")
if len(app) > 0x30000:
    raise SystemExit("application exceeds 192 KiB")
image = bytearray(b"\xff" * (0x8000 + len(app)))
image[:len(boot)] = boot
image[0x8000:] = app
out.write_bytes(image)
