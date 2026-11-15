# MP4 to ASCII Converter

Tools used to convert MP4 videos into text-based ASCII animation clips for the
[ABCF13 portfolio site](https://github.com/ABCF13/portfolio-site).

## Contents

- `build.py` - converts videos from `source/` into clips in `clips/`
- `convert.ps1` - converts one video from PowerShell
- `ascii_conv.c` - ASCII frame converter source
- `ascii_conv.exe` - Windows converter binary
- `files.zip` - packaged converter files

## Usage

From the repository root:

```powershell
python tools/build.py
python tools/build.py --force
.\tools\convert.ps1 video.mp4 001
```

The scripts expect `source/` and `clips/` directories beside the tools.
