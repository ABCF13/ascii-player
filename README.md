# MP4 to ASCII Converter

scripts & functions for music videos conversion to ascii characters

## Contents

- `build.py` - converts videos from `source/` into clips in `clips/`
- `convert.ps1` - converts one video from PowerShell
- `ascii_conv.c` - ASCII frame converter source



## Usage

From the repository root:

```powershell
python tools/build.py
python tools/build.py --force
.\tools\convert.ps1 video.mp4 001
```

The scripts expect `source/` and `clips/` directories beside the tools.
