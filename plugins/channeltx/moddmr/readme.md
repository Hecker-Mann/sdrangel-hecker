# DMR TX Modulator Channel Plugin

Transmit channel plugin for SDRangel implementing DMR (ETSI TS 102 361) 4FSK at 4800 symbols/s.

## Features

- DMR idle transmission (PR FILL / data sync pattern)
- Voice call transmission with **AMBE+2 encoding** from the microphone
- Configurable color code, source ID, destination/talkgroup ID, slot, and simplex/duplex sync
- Calibration pattern (+3/+3/-3/-3 deviation test)
- 4FSK modem with root-raised-cosine pulse shaping

## Modes

| Mode | Description |
|------|-------------|
| Idle | Continuous DMR idle bursts on both TDMA slots |
| Voice | Press **TX Voice** to send LC header, AMBE voice frames, and terminator |
| Calibration | Constant calibration dibit pattern |

## Voice / AMBE+2

Microphone audio is downsampled to 8 kHz and encoded with the OpenDMR/OP25
AMBE+2 software encoder (`opendmr/`). Enable **Microphone (AMBE+2)**, set
mode to **Voice**, then hold **TX Voice**.

If the mic is disabled or audio underruns, standard silence AMBE payloads
are sent so the call structure remains valid.

**Patent note:** AMBE+2 is patent-encumbered. This uses a software vocoder
for amateur/experimental use — the same class of tool as mbelib (decode)
already used by SDRangel. You are responsible for compliance with patents
and local regulations.

## Integration into SDRangel

Copy or symlink this directory into the SDRangel source tree:

```bash
cp -r moddmr /path/to/sdrangel/plugins/channeltx/moddmr
```

Add to `plugins/channeltx/CMakeLists.txt`:

```cmake
option(ENABLE_CHANNELTX_MODDMR "Enable channeltx moddmr plugin" ON)
...
if (ENABLE_CHANNELTX_MODDMR)
    add_subdirectory(moddmr)
endif()
```

Add to root `CMakeLists.txt` with other `ENABLE_CHANNELTX_*` options:

```cmake
option(ENABLE_CHANNELTX_MODDMR "Enable channeltx moddmr plugin" ON)
```

Rebuild SDRangel. The plugin appears as **DMR Modulator** in the Tx channels list.

## Codec credits

- DMR link-layer encoding (`dmrcodec/`): adapted from [MMDVMHost](https://github.com/g4klx/MMDVMHost) by Jonathan Naylor G4KLX (GPLv2)
- AMBE+2 encoder (`opendmr/`): OpenDMR / OP25 MBEEncoder — see `opendmr/CREDITS.md`

## Usage tips

- Set baseband sample rate to at least 48000 Hz on the Tx device set
- Match color code and IDs to your DMR network or simplex partner
- Use slot 1 for direct mode; enable duplex sync when operating through a repeater
- Select **Voice** mode, enable the mic, then press **TX Voice**
- Verify with the **DSD Demod** receive plugin at 4800 baud (4.8k setting)
