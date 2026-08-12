# OpenDMR / OP25 AMBE+2 encoder (vendored)

This directory vendors the DMR AMBE+2 software encoder used by the moddmr
plugin for microphone → AMBE voice encoding.

## Sources

- `encoder/` — OpenDMR / OP25 MBEEncoder and IMBE vocoder encode path
  - MBEEncoder: Copyright (C) 2013–2019 Max H. Parke KA1RBI (OP25 / GNU Radio)
  - IMBE vocoder core: Copyright (C) 2009 Pavel Yazev (via OP25)
  - Simplified DMR-only packaging from the OpenDMR project
- `mbelib/` — classic mbelib pieces used by the encoder feedback path
  - Copyright (C) 2010 mbelib author(s)

## License

Encoder code is GPLv2 / GPLv3 compatible (OP25 / OpenDMR). mbelib uses a
permissive license — see headers in each file.

## Patent note

AMBE+2 is patent-encumbered. This software encoder is provided for
amateur radio / experimental use in the same class of tools as mbelib
(already used by SDRangel for decode). Users are responsible for
compliance with applicable patent and local regulations.
