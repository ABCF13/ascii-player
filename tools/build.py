#!/usr/bin/env python3
"""
build.py - convert every new .mp4 in source/ into clips/ in one go.

  python tools/build.py                 convert new videos (skips ones already done)
  python tools/build.py --force         redo everything
  python tools/build.py --map 001=video.mp4   tell it a video already has an ID (use once
                                        if you converted video.mp4 before this script)

For each video it writes clips/clip_AAA.txt, then
keeps two lists up to date:
  clips/library.json   which video has which ID (so IDs never shift)
  clips/playlist.json  play order for the website's Next/Prev buttons
"""
import argparse, json, os, shutil, subprocess, sys

W, H = 154, 43                      # characters per frame (1.7x the area of 118x33)
TOOLS = os.path.dirname(os.path.abspath(__file__))
HERE = os.path.dirname(TOOLS)
os.chdir(HERE)
SOURCE, CLIPS = "source", "clips"
EXE = os.path.join(TOOLS, "ascii_conv" + (".exe" if sys.platform == "win32" else ""))


def ensure_converter():
    src = os.path.join(TOOLS, "ascii_conv.c")
    if os.path.exists(EXE) and os.path.getmtime(EXE) >= os.path.getmtime(src):
        return
    if not shutil.which("gcc"):
        sys.exit("gcc not found - install it or compile ascii_conv.c yourself.")
    print("Compiling ascii_conv...")
    subprocess.run(["gcc", "-O2", "-o", EXE, src, "-lm"], check=True)


def convert_ascii(video, cid):
    raw = os.path.join(CLIPS, "temp_%s.raw" % cid)
    try:
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", video,
                        "-vf", "fps=12,scale=%d:%d" % (W, H),
                        "-f", "rawvideo", "-pix_fmt", "gray", raw], check=True)
        subprocess.run([EXE, raw, cid, str(W), str(H),
                        os.path.join(CLIPS, "clip_%s.txt" % cid)], check=True)
    finally:
        if os.path.exists(raw):
            os.remove(raw)


def load_json(path, default):
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return default


def save_lists(library):
    with open(os.path.join(CLIPS, "library.json"), "w", encoding="utf-8") as f:
        json.dump(dict(sorted(library.items())), f, indent=1)
    ids = [i for i in sorted(library)
           if os.path.exists(os.path.join(CLIPS, "clip_%s.txt" % i))]
    with open(os.path.join(CLIPS, "playlist.json"), "w", encoding="utf-8") as f:
        json.dump({"clips": ids}, f)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--force", action="store_true")
    ap.add_argument("--map", action="append", default=[], metavar="ID=FILE")
    a = ap.parse_args()

    if not shutil.which("ffmpeg"):
        sys.exit("ffmpeg not found (winget install Gyan.FFmpeg, then reopen VS Code).")
    if not os.path.isdir(SOURCE):
        sys.exit("No '%s' folder next to build.py." % SOURCE)
    os.makedirs(CLIPS, exist_ok=True)
    ensure_converter()

    library = load_json(os.path.join(CLIPS, "library.json"), {})
    for item in a.map:
        cid, _, name = item.partition("=")
        library[cid.zfill(3)] = name
    by_name = {v: k for k, v in library.items()}

    videos = sorted(f for f in os.listdir(SOURCE) if f.lower().endswith(".mp4"))
    if not videos:
        sys.exit("No .mp4 files in %s/" % SOURCE)

    for name in videos:
        cid = by_name.get(name)
        if cid is None:
            used = [int(k) for k in library] or [0]
            cid = "%03d" % (max(used) + 1)
            if int(cid) > 999:
                sys.exit("Out of IDs (max 999).")
            library[cid] = name
            by_name[name] = cid
        video = os.path.join(SOURCE, name)
        txt = os.path.join(CLIPS, "clip_%s.txt" % cid)

        if a.force or not os.path.exists(txt):
            print("[%s] %s: converting video..." % (cid, name))
            try:
                convert_ascii(video, cid)
            except subprocess.CalledProcessError as e:
                print("[%s] FAILED (%s), skipping" % (cid, e))
                continue
        else:
            print("[%s] %s: ASCII already done" % (cid, name))

        save_lists(library)             # after every song, so progress is never lost

    save_lists(library)
    print("\nDone. Playlist:")
    for cid in sorted(library):
        print("  %s  %s" % (cid, library[cid]))


if __name__ == "__main__":
    main()
