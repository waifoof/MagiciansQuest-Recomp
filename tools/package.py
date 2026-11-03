#!/usr/bin/env python3
"""Builds a release package: the program and the compiler it needs, and nothing of the game.

  python tools/package.py windows [--version 0.1.0-alpha] [--llvm-dir DIR] [--no-build]
  python tools/package.py linux   [--version V] [--native] [--smoke ROM]   (see docs/PACKAGING.md and docs/STEAM_DECK.md)

The package holds the program, a small piece of LLVM (clang and the linker, with clang's own headers: the player's PC
builds the game module with them from the player's own ROM) and the licences. It never holds a ROM, a save, generated
game code or a built game module: the package is scanned for them before it is written.

The LLVM comes from --llvm-dir (an installed LLVM of the pinned version) or is downloaded once into build/pkgcache and
checked against the SHA-256 pinned below.
"""
from __future__ import annotations

import argparse
import contextlib
import hashlib
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_no_game_data as guard  # noqa: E402

DEFAULT_VERSION = "0.1.0-alpha"
LLVM_VERSION = "22.1.8"
LLVM_MAJOR = LLVM_VERSION.split(".")[0]
LLVM_RELEASE = f"https://github.com/llvm/llvm-project/releases/download/llvmorg-{LLVM_VERSION}/"
LLVM_ARCHIVES = {  # platform -> (file, sha256, name of the folder inside)
    "windows": ("clang+llvm-22.1.8-x86_64-pc-windows-msvc.tar.xz",
                "d96c2cc1736f4eb7fa43cb9bbdf56d93551a9ae0a9aadb9c99c3c3b2b712a234",
                "clang+llvm-22.1.8-x86_64-pc-windows-msvc"),
    "linux": ("LLVM-22.1.8-Linux-X64.tar.xz",
              "df0e1ecf16caf3489a272a5eea4eec9b0d82878f6477fa309504f918a0006384",
              "LLVM-22.1.8-Linux-X64"),
}

# What of LLVM goes into the package (paths inside an LLVM install).
LLVM_FILES = {
    "windows": ["bin/clang.exe", "bin/lld-link.exe"],
    "linux": ["bin/clang", "bin/lld"],  # (clang-22, ld.lld and the shared libraries are added by name below)
}

# Licence texts copied into the package: (name in licenses/, path from the repo root or the build tree's _deps).
LICENSES = [
    ("melonDS-GPL-3.0.txt", "third_party/melonDS/LICENSE"),
    ("melonDS-blip-buf.txt", "third_party/melonDS/src/blip-buf/license.txt"),
    ("melonDS-dolphin.txt", "third_party/melonDS/src/dolphin/license_dolphin.txt"),
    ("melonDS-fatfs.txt", "third_party/melonDS/src/fatfs/LICENSE.txt"),
    ("melonDS-libslirp.txt", "third_party/melonDS/src/net/libslirp/COPYRIGHT"),
    ("melonDS-teakra.txt", "third_party/melonDS/src/teakra/LICENSE"),
    ("SDL3.txt", "_deps/sdl3-src/LICENSE.txt"),
    ("RmlUi.txt", "_deps/rmlui-src/LICENSE.txt"),
    ("FreeType-FTL.txt", "_deps/freetype-src/docs/FTL.TXT"),
    ("font-Cinzel-OFL.txt", "game/ui/res/fonts/OFL-Cinzel.txt"),
    ("font-AlegreyaSans-OFL.txt", "game/ui/res/fonts/OFL-AlegreyaSans.txt"),
    ("font-MaterialSymbols-Apache-2.0.txt", "game/ui/res/fonts/LICENSE-Apache-2.0.txt"),
]

README_TXT = """Magician's Quest: Mysterious Times, PC port ({version}, alpha)

1. {start}
2. Choose your own copy of the game (checked by SHA-1):
   - USA: Magician's Quest - Mysterious Times (USA) (En,Fr,Es).nds
     ab2e495f6206b5c9b69043f8b4114d760f7298b2
   It is checked and built into a game for your PC (a few minutes the first time). Nothing else has to be installed.

This package contains no game data. It is an early alpha: report problems on the project's GitHub page.
{unsigned}
Your saves and settings are kept in {data}.
Put a file named portable.txt next to the program to keep them in a "data" folder beside it instead.

The licences of what the package contains are in the licenses folder and NOTICE.md.
"""


def run(cmd, **kw):
    print("+", " ".join(str(c) for c in cmd), flush=True)
    subprocess.run([str(c) for c in cmd], check=True, **kw)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def version_from_git() -> str:
    try:
        out = subprocess.run(["git", "tag", "--points-at", "HEAD"], cwd=ROOT, capture_output=True, text=True)
        # (only a version tag: main's rolling "latest" pre-release tags a commit too, and a package rebuilt from that
        # commit must keep the version in its file name, or the download link would change)
        for tag in out.stdout.split():
            if out.returncode == 0 and tag.startswith("v") and tag[1:2].isdigit():
                return tag[1:]
    except OSError:
        pass
    return DEFAULT_VERSION


def llvm_source(target: str, llvm_dir: str | None) -> Path:
    """The folder of an LLVM install (with bin/ and lib/) of the pinned version."""
    if llvm_dir:
        return Path(llvm_dir)
    name, digest, inner = LLVM_ARCHIVES[target]
    # On Windows the compiler's own path ends up in the PDB, so the download is kept in a folder that says nothing about who
    # built the package (not in the repo, whose folder name is the builder's choice).
    cache = Path(os.environ["ProgramData"]) / "ffcc-pkgcache" if platform.system() == "Windows" and "ProgramData" in os.environ else ROOT / "build" / "pkgcache"
    unpacked = cache / inner
    if (unpacked / "bin").is_dir():
        return unpacked
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / name
    if not archive.exists() or sha256(archive) != digest:
        url = LLVM_RELEASE + name.replace("+", "%2B")
        print(f"downloading {url} ({name})", flush=True)
        urllib.request.urlretrieve(url, archive)
        if sha256(archive) != digest:
            archive.unlink()
            sys.exit(f"{name}: the download does not match the pinned SHA-256")
    wanted = tuple(f"{inner}/{p}" for p in ("bin/", "lib/clang/", "lib/libLLVM", "lib/libclang-cpp", "LICENSE.TXT"))
    print(f"unpacking the parts of {name} the package needs", flush=True)
    with tarfile.open(archive, "r:xz") as tar:
        for member in tar:
            if member.name.startswith(wanted):
                tar.extract(member, cache)
    return unpacked


def check_llvm(llvm: Path, target: str):
    exe = llvm / "bin" / ("clang.exe" if target == "windows" else "clang")
    out = subprocess.run([str(exe), "--version"], capture_output=True, text=True)
    first = out.stdout.splitlines()[0] if out.stdout else ""
    if LLVM_VERSION not in first:
        sys.exit(f"{exe}: expected LLVM {LLVM_VERSION}, found '{first}'")


@contextlib.contextmanager
def neutral_root(target: str):
    r"""The repo as seen by the build. On Windows it is a drive of its own (subst), so the object and debug info paths of the
    package read `Z:\build\pkg-windows\...` and not a folder of the machine that built it."""
    if target != "windows":
        yield ROOT
        return
    used = {c.upper() for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ" if Path(f"{c}:/").exists()}
    letter = next((c for c in "ZYXWVUTSRQ" if c not in used), None)
    if letter is None:
        sys.exit("no free drive letter for the neutral build path")
    run(["subst", f"{letter}:", ROOT])
    try:
        yield Path(f"{letter}:/")
    finally:
        subprocess.run(["subst", f"{letter}:", "/D"])


def build(target: str, root: Path, build_dir: Path, llvm: Path | None = None):
    if target == "windows":
        extra = []
        if shutil.which("clang-cl") is None and llvm is not None:  # (not on PATH: name it)
            extra = [f"-DCMAKE_C_COMPILER={(llvm / 'bin' / 'clang-cl.exe').as_posix()}", f"-DCMAKE_CXX_COMPILER={(llvm / 'bin' / 'clang-cl.exe').as_posix()}",
                     f"-DCMAKE_LINKER={(llvm / 'bin' / 'lld-link.exe').as_posix()}", f"-DCMAKE_RC_COMPILER={(llvm / 'bin' / 'llvm-rc.exe').as_posix()}",
                     f"-DCMAKE_MT={(llvm / 'bin' / 'llvm-mt.exe').as_posix()}"]
        run(["cmake", "--preset", "release", "-B", build_dir, "-DFFCC_PACKAGE=ON", "-DFFCC_ROM=", "-DCMAKE_BUILD_TYPE=RelWithDebInfo"] + extra, cwd=root)
    else:
        run(["cmake", "--preset", "linux-release", "-B", build_dir, "-DFFCC_PACKAGE=ON", "-DFFCC_ROM="], cwd=root)
    run(["cmake", "--build", build_dir], cwd=root)
    # (On CI runners without OpenGL the UI smoke test cannot run: it is left out there.)
    run(["ctest", "--test-dir", build_dir, "--output-on-failure"] + (["-E", "ui_smoke"] if os.environ.get("CI") else []), cwd=root)


def copy_llvm(llvm: Path, target: str, stage: Path):
    tc = stage / "toolchain"
    for rel in LLVM_FILES[target]:
        dst = tc / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(llvm / rel, dst)
    inc = llvm / "lib" / "clang" / LLVM_MAJOR / "include"
    if not inc.is_dir():
        sys.exit(f"{inc} is missing (clang's own headers)")
    shutil.copytree(inc, tc / "lib" / "clang" / LLVM_MAJOR / "include")
    lic = llvm / "LICENSE.TXT"
    if lic.exists():
        (stage / "licenses").mkdir(exist_ok=True)
        shutil.copy2(lic, stage / "licenses" / "LLVM-Apache-2.0-with-LLVM-exception.txt")


def copy_licenses(build_dir: Path, stage: Path):
    out = stage / "licenses"
    out.mkdir(exist_ok=True)
    for name, rel in LICENSES:
        src = build_dir / rel if rel.startswith("_deps/") else ROOT / rel
        if not src.exists():
            sys.exit(f"licence text missing: {src}")
        shutil.copy2(src, out / name)
    shutil.copy2(ROOT / "LICENSE", stage / "LICENSE")
    shutil.copy2(ROOT / "NOTICE.md", stage / "NOTICE.md")


def scan(stage: Path):
    """Refuses a package with game data in it."""
    bad = []
    for p in stage.rglob("*"):
        if not p.is_file():
            continue
        name = p.name.lower()
        if p.suffix.lower() in guard.FORBIDDEN_EXTS or name.startswith(("cfie_game", "libcfie_game", "func_table")) \
                or name in ("manifest.ini", "cfie.sav", "cfip.sav", "cfie.nds", "ynne.sav"):
            bad.append(p)
        elif p.suffix.lower() == ".c" and "generated" in p.read_text(errors="ignore")[:400].lower():
            bad.append(p)
    if bad:
        sys.exit("game data in the package:\n  " + "\n  ".join(str(b) for b in bad))


def check_no_build_paths(stage: Path):
    """Refuses a program or debug file that holds the folder of the machine that built it."""
    needles = {str(ROOT).lower().encode(), str(ROOT).replace("\\", "/").lower().encode()}
    for f in stage.iterdir():
        if f.suffix.lower() in (".exe", ".pdb"):
            data = f.read_bytes().lower()
            for n in needles:
                if n in data:
                    sys.exit(f"{f.name} contains the build folder {n.decode()}")


def zip_dir(stage: Path, out: Path):
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists():
        out.unlink()
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for p in sorted(stage.rglob("*")):
            if p.is_file():
                z.write(p, Path(stage.name) / p.relative_to(stage))


def package_windows(args):
    version = args.version or version_from_git()
    build_dir = ROOT / "build" / "pkg-windows"
    llvm = llvm_source("windows", args.llvm_dir)
    check_llvm(llvm, "windows")
    if not args.no_build:
        shutil.rmtree(build_dir, ignore_errors=True)  # (a clean configure: nothing cached from another folder or compiler path)
        with neutral_root("windows") as root:
            try:
                tools = root / llvm.relative_to(ROOT)  # (the compiler is seen through the neutral drive too: its path ends up in the PDB)
            except ValueError:
                tools = llvm
            build("windows", root, root / "build" / "pkg-windows", tools)
    exe = build_dir / "game" / "MagiciansQuest.exe"
    if not exe.exists():
        sys.exit(f"{exe} was not built")

    stage = ROOT / "dist" / f"MagiciansQuest-Recomp-{version}-windows-x64"
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True)
    shutil.copy2(exe, stage / "MagiciansQuest.exe")
    pdb = build_dir / "game" / "MagiciansQuest.pdb"
    if pdb.exists():
        shutil.copy2(pdb, stage / "MagiciansQuest.pdb")  # (crash reports show source lines with it)
    check_no_build_paths(stage)
    copy_llvm(llvm, "windows", stage)
    copy_licenses(build_dir, stage)
    (stage / "README.txt").write_text(README_TXT.format(
        version=version,
        start="Extract this folder somewhere you can write to (not Program Files) and run MagiciansQuest.exe.",
        unsigned="\nThe program is not signed: Windows SmartScreen or your antivirus may warn about it the first time\n(choose More info, then Run anyway).\n",
        data="%APPDATA%\\MagiciansQuest-Recomp\\MysteriousTimes"), encoding="utf-8")
    scan(stage)
    out = ROOT / "dist" / f"MagiciansQuest-Recomp-{version}-windows-x64.zip"
    zip_dir(stage, out)
    print(f"\n{out} ({out.stat().st_size / 1e6:.1f} MB)")


IMAGE = "mq-linux-build"
APP = "MagiciansQuest"  # the program's name in the AppImage (usr/bin/MagiciansQuest)
DESKTOP = """[Desktop Entry]
Type=Application
Name=Magician's Quest: Mysterious Times (PC port)
Comment=Magician's Quest: Mysterious Times, built from your own copy of the game
Exec=MagiciansQuest
Icon=magiciansquest
Categories=Game;
Terminal=false
"""
APPRUN = """#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
exec "$HERE/usr/bin/MagiciansQuest" "$@"
"""
# The linker the game's setup runs (toolchain/bin/ld.lld): lld with the libraries it needs from the package. A script and
# not a run path, because a run path is not passed on to the libraries a library loads (libxml2 -> libicuuc), and the package
# is built without patchelf on machines that lack it. (`-flavor gnu`: lld is called by another name than ld.lld.)
LD_LLD = """#!/bin/sh
d="$(dirname "$(readlink -f "$0")")"
LD_LIBRARY_PATH="$d/../lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" exec "$d/lld" -flavor gnu "$@"
"""
# Libraries every Linux system has (glibc, the C++ runtime, zlib): the toolchain takes these from the system. Everything
# else its binaries load comes along in toolchain/lib (lld: libxml2 and its ICU and liblzma, whose names differ between
# distributions: Arch and SteamOS have libxml2.so.16).
SYSTEM_LIBS = ("linux-vdso", "ld-linux", "libc.so", "libm.so", "libdl.so", "libpthread.so", "librt.so", "libutil.so",
               "libstdc++.so", "libgcc_s.so", "libz.so")
# The newest glibc a package may need. The Docker build (Ubuntu 22.04) gives 2.35; a --native build gives this machine's,
# and is refused above the Steam Deck's (SteamOS 3.6 and later have 2.39 or newer).
GLIBC_MAX = {"docker": (2, 35), "native": (2, 39)}
# appimagetool and its runtime, pinned (the same pins as tools/docker/linux.Dockerfile).
APPIMAGETOOL = ("https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage",
                "ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0")
APPIMAGE_RUNTIME = ("https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-x86_64",
                    "2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d")
LLVM_LICENSE = ("https://raw.githubusercontent.com/llvm/llvm-project/llvmorg-22.1.8/llvm/LICENSE.TXT",
                "8d85c1057d742e597985c7d4e6320b015a9139385cff4cbae06ffc0ebe89afee")


def appimage_name(version: str) -> str:
    return f"MagiciansQuest-Recomp-{version}-x86_64.AppImage"


def ico_png(ico: Path) -> bytes:
    """The largest picture of an .ico (they are PNGs)."""
    import struct
    data = ico.read_bytes()
    count = struct.unpack("<H", data[4:6])[0]
    best = (0, b"")
    for i in range(count):
        w, _h, _c, _r, _planes, _bpp, size, off = struct.unpack("<BBBBHHII", data[6 + 16 * i:22 + 16 * i])
        if (w or 256) > best[0] and data[off:off + 4] == b"\x89PNG":
            best = (w or 256, data[off:off + size])
    if not best[1]:
        sys.exit("no PNG picture in the icon")
    return best[1]


def pinned_download(url: str, digest: str, dst: Path, required: bool = True) -> Path | None:
    if dst.exists() and sha256(dst) == digest:
        return dst
    dst.parent.mkdir(parents=True, exist_ok=True)
    print(f"downloading {url}", flush=True)
    try:
        urllib.request.urlretrieve(url, dst)
    except OSError as e:
        if required:
            sys.exit(f"{url}: {e}")
        print(f"warning: {url}: {e}", flush=True)
        return None
    if sha256(dst) != digest:
        dst.unlink()
        sys.exit(f"{dst.name}: the download does not match the pinned SHA-256")
    return dst


def glibc_needed(path: Path) -> tuple[int, ...]:
    """The newest GLIBC_x.y symbol version an ELF file asks for ((0,) for none)."""
    out = subprocess.run(["objdump", "-T", str(path)], capture_output=True, text=True).stdout
    import re
    vers = [tuple(int(x) for x in m.split(".")) for m in re.findall(r"GLIBC_([0-9]+(?:\.[0-9]+)+)", out)]
    return max(vers, default=(0,))


def check_glibc(appdir: Path, limit: tuple[int, ...]):
    worst, who = (0,), None
    for p in appdir.rglob("*"):
        if p.is_file() and not p.is_symlink() and p.read_bytes()[:4] == b"\x7fELF":
            v = glibc_needed(p)
            if v > worst:
                worst, who = v, p
    shown = ".".join(map(str, worst))
    print(f"newest glibc the package needs: {shown} ({who.relative_to(appdir) if who else '-'})", flush=True)
    if worst > limit:
        sys.exit(f"{who} needs glibc {shown}, newer than {'.'.join(map(str, limit))}: build it in Docker (Ubuntu 22.04)")


def docker_prepare():
    name, digest, _inner = LLVM_ARCHIVES["linux"]
    run(["docker", "build", "-t", IMAGE, "-f", ROOT / "tools" / "docker" / "linux.Dockerfile",
         "--build-arg", f"LLVM_URL={LLVM_RELEASE}{name}", "--build-arg", f"LLVM_SHA256={digest}",
         "--build-arg", f"APPIMAGETOOL_URL={APPIMAGETOOL[0]}", "--build-arg", f"APPIMAGETOOL_SHA256={APPIMAGETOOL[1]}",
         "--build-arg", f"RUNTIME_URL={APPIMAGE_RUNTIME[0]}", "--build-arg", f"RUNTIME_SHA256={APPIMAGE_RUNTIME[1]}",
         ROOT / "tools" / "docker"])


def sdl_lacks(build_dir: Path) -> list[str]:
    """What the build's SDL was configured without that a player would miss (a machine without the -dev packages)."""
    cache = (build_dir / "CMakeCache.txt").read_text(errors="ignore") if (build_dir / "CMakeCache.txt").exists() else ""
    off = lambda k: f"{k}:BOOL=OFF" in cache
    lacks = []
    if off("SDL_PULSEAUDIO") and off("SDL_PIPEWIRE") and off("SDL_ALSA"):
        lacks.append("sound (no PulseAudio, PipeWire or ALSA)")
    if off("SDL_X11_XINPUT"):
        lacks.append("touch screens and precise mice under X11 (no XInput2)")
    return lacks


def assemble_appimage(exe: Path, build_dir: Path, llvm: Path, appimagetool: Path, runtime: Path, out: Path,
                      glibc_max: tuple[int, ...], work: Path, llvm_license: Path | None):
    """The AppDir (the program, the compiler the setup uses, licences, icon) and the AppImage made from it."""
    appdir = work / "AppDir"
    shutil.rmtree(appdir, ignore_errors=True)
    bin_dir = appdir / "usr" / "bin"
    bin_dir.mkdir(parents=True)
    shutil.copy2(exe, bin_dir / APP)
    # The compiler (setup.cpp looks for toolchain/bin/clang and toolchain/bin/ld.lld next to the program): clang-22 and
    # lld, clang's own headers (found from clang's real path, ../lib/clang/22), and the libraries the binaries load that
    # are not everywhere.
    tc = bin_dir / "toolchain"
    (tc / "bin").mkdir(parents=True)
    (tc / "lib").mkdir()
    for real in ("clang-" + LLVM_MAJOR, "lld"):
        shutil.copy2(llvm / "bin" / real, tc / "bin" / real)
    os.symlink("clang-" + LLVM_MAJOR, tc / "bin" / "clang")
    (tc / "bin" / "ld.lld").write_text(LD_LLD, encoding="utf-8")
    (tc / "bin" / "ld.lld").chmod(0o755)
    bundled = []
    for real in ("clang-" + LLVM_MAJOR, "lld"):
        found = subprocess.run(["ldd", str(tc / "bin" / real)], capture_output=True, text=True).stdout
        if "not found" in found:
            sys.exit(f"{real} needs a library this machine does not have:\n{found}")
        for line in found.splitlines():
            if "=>" not in line:
                continue
            soname, where = line.split("=>")[0].strip(), line.split("=>")[1].split()[0]
            if soname.startswith(SYSTEM_LIBS) or (tc / "lib" / soname).exists():
                continue
            shutil.copy2(Path(where).resolve(), tc / "lib" / soname)
            bundled.append(soname)
    print("toolchain libraries in the package:", ", ".join(bundled) or "none", flush=True)
    inc = llvm / "lib" / "clang" / LLVM_MAJOR / "include"
    if not inc.is_dir():
        sys.exit(f"{inc} is missing (clang's own headers)")
    shutil.copytree(inc, tc / "lib" / "clang" / LLVM_MAJOR / "include")
    # The toolchain as the setup uses it: a freestanding object and a shared library, with nothing else on the library path.
    with contextlib.ExitStack() as stack:
        import tempfile
        t = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        (t / "a.c").write_text("#include <stdint.h>\nextern uint32_t ntr_x(uint32_t);\nuint32_t f(uint32_t v) { return ntr_x(v) + 1; }\n")
        env = {"PATH": "/usr/bin:/bin"}
        run([tc / "bin" / "clang", "--target=x86_64-unknown-linux-gnu", "-c", "-O1", "-ffreestanding", "-nostdlibinc", "-fPIC",
             t / "a.c", "-o", t / "a.o"], env=env)
        out_ld = subprocess.run([str(tc / "bin" / "ld.lld"), "-shared", "-o", str(t / "a.so"), str(t / "a.o")],
                                capture_output=True, text=True, env=dict(env, LD_DEBUG="libs"))
        if out_ld.returncode != 0 or not (t / "a.so").exists():
            sys.exit(f"the packaged linker failed:\n{out_ld.stdout}{out_ld.stderr[-3000:]}")
        for soname in bundled:
            if not any("calling init" in l and soname in l and "/toolchain/" in l for l in out_ld.stderr.splitlines()):
                sys.exit(f"lld does not take {soname} from the package")
    docs = appdir / "usr" / "share" / "doc" / "MagiciansQuest-Recomp"
    docs.mkdir(parents=True)
    copy_licenses(build_dir, docs)
    if llvm_license is not None:
        shutil.copy2(llvm_license, docs / "licenses" / "LLVM-Apache-2.0-with-LLVM-exception.txt")
    (appdir / "magiciansquest.desktop").write_text(DESKTOP, encoding="utf-8")
    (appdir / "magiciansquest.png").write_bytes(ico_png(ROOT / "game" / "res" / "ffcc.ico"))
    apprun = appdir / "AppRun"
    apprun.write_text(APPRUN, encoding="utf-8")
    apprun.chmod(0o755)
    scan(appdir)
    check_glibc(appdir, glibc_max)
    out.parent.mkdir(exist_ok=True)
    if out.exists():
        out.unlink()
    appimagetool.chmod(0o755)
    run([appimagetool, "--appimage-extract-and-run", "--no-appstream", "--runtime-file", runtime, appdir, out],
        env=dict(os.environ, ARCH="x86_64"))
    print(f"\n{out} ({out.stat().st_size / 1e6:.1f} MB)")


def package_linux(args):
    """On the host: runs this script inside the build image (or, --native, builds here). In the image (FFCC_IN_DOCKER):
    builds and packages."""
    version = args.version or version_from_git()
    if args.native:
        return package_linux_native(args, version)
    if not os.environ.get("FFCC_IN_DOCKER"):
        docker_prepare()
        if args.smoke:
            return smoke_linux(args, version)
        run(["docker", "run", "--rm", "-v", f"{ROOT}:/src", "-v", f"{IMAGE}-cache:/build", IMAGE, "python3", "/src/tools/package.py", "linux",
             "--version", version] + (["--no-build"] if args.no_build else []))
        return

    build_dir = Path("/build/pkg-linux")  # (inside the container: a bind-mounted folder is slow)
    if not args.no_build:
        run(["git", "config", "--global", "--add", "safe.directory", "*"])
        run(["cmake", "--preset", "linux-release", "-S", "/src", "-B", build_dir, "-DFFCC_PACKAGE=ON", "-DFFCC_ROM="], cwd="/src")
        run(["cmake", "--build", build_dir], cwd="/src")
        env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1")
        run(["xvfb-run", "-a", "-s", "-screen 0 1280x720x24", "ctest", "--test-dir", build_dir, "--output-on-failure"], cwd="/src", env=env)
    exe = build_dir / "game" / APP
    if not exe.exists():
        sys.exit(f"{exe} was not built")
    lacks = sdl_lacks(build_dir)
    if lacks:
        sys.exit("the build's SDL lacks " + "; ".join(lacks))
    llvm = Path("/opt/llvm")
    check_llvm(llvm, "linux")
    assemble_appimage(exe, build_dir, llvm, Path("/opt/appimagetool"), Path("/opt/runtime-x86_64"),
                      ROOT / "dist" / appimage_name(version), GLIBC_MAX["docker"], Path("/build"), llvm / "LICENSE.TXT")


def package_linux_native(args, version):
    """The same package built on this machine, without Docker: for testing. Its glibc baseline is this system's (refused
    above the Steam Deck's), and its SDL has what this machine's -dev packages give it (sound needs libpulse-dev or
    libpipewire-0.3-dev): what it lacks is printed at the end. Releases come from the Docker build."""
    if args.smoke:
        return smoke_linux_native(args, version)
    build_dir = ROOT / "build" / "pkg-linux"
    if not args.no_build:
        run(["cmake", "--preset", "linux-release", "-B", build_dir, "-DFFCC_PACKAGE=ON", "-DFFCC_ROM="], cwd=ROOT)
        run(["cmake", "--build", build_dir], cwd=ROOT)
        if not args.no_tests:
            run(["xvfb-run", "-a", "-s", "-screen 0 1280x720x24", "ctest", "--test-dir", build_dir, "--output-on-failure"],
                cwd=ROOT, env=dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1"))
    exe = build_dir / "game" / APP
    if not exe.exists():
        sys.exit(f"{exe} was not built")
    llvm = llvm_source("linux", args.llvm_dir)
    check_llvm(llvm, "linux")
    cache = ROOT / "build" / "pkgcache"
    tool = pinned_download(*APPIMAGETOOL, cache / "appimagetool-x86_64.AppImage")
    runtime = pinned_download(*APPIMAGE_RUNTIME, cache / "runtime-x86_64")
    lic = llvm / "LICENSE.TXT" if (llvm / "LICENSE.TXT").exists() else pinned_download(*LLVM_LICENSE, cache / "LLVM-LICENSE.TXT", required=False)
    assemble_appimage(exe, build_dir, llvm, tool, runtime, ROOT / "dist" / appimage_name(version), GLIBC_MAX["native"],
                      ROOT / "build" / "pkg-linux-appdir", lic)
    notes = sdl_lacks(build_dir) + ([] if lic else ["LLVM's licence text (not downloadable here)"])
    if notes:
        print("\nA TEST PACKAGE, not for release: it lacks " + "; ".join(notes) + ".")


def smoke_linux(args, version):
    """Sets the game up from a ROM with the package's own compiler and boots it, in a container without LLVM on its PATH."""
    if not args.smoke or not Path(args.smoke).is_file():
        sys.exit("--smoke needs the path of your ROM (it is mounted read-only, never copied into an image)")
    rom = Path(args.smoke).resolve()
    appimage = ROOT / "dist" / appimage_name(version)
    if not appimage.exists():
        sys.exit(f"{appimage} does not exist: run `package.py linux` first")
    script = (
        "set -e; export PATH=/usr/bin:/bin; export LIBGL_ALWAYS_SOFTWARE=1; cd /tmp; "
        f"cp /src/dist/{appimage.name} /tmp/mq.AppImage; chmod +x /tmp/mq.AppImage; "
        "! command -v clang; "
        "/tmp/mq.AppImage --appimage-extract-and-run --data-dir /tmp/data --setup-rom /rom/game.nds --setup-only; "
        "ls -la /tmp/data/game; "
        "printf '900 shot smoke.png\\n910 quit\\n' > /tmp/smoke.txt; "
        "xvfb-run -a -s '-screen 0 1280x800x24' /tmp/mq.AppImage --appimage-extract-and-run --data-dir /tmp/data "
        "--first-run --fullscreen --unpaced --input-script /tmp/smoke.txt; "
        "cp /tmp/smoke.png /src/dist/smoke-linux.png; echo SMOKE OK"
    )
    run(["docker", "run", "--rm", "-v", f"{ROOT}:/src", "-v", f"{rom}:/rom/game.nds:ro", IMAGE, "bash", "-c", script])
    print(f"\nboot screenshot: {ROOT / 'dist' / 'smoke-linux.png'}")


def smoke_linux_native(args, version):
    """The smoke test on this machine: setup from the ROM into a new data folder, then a boot at 1280x800 (the Steam Deck's
    screen) under Xvfb. The setup must use the package's compiler: checked in the manifest."""
    if not args.smoke or not Path(args.smoke).is_file():
        sys.exit("--smoke needs the path of your ROM")
    rom = Path(args.smoke).resolve()
    appimage = ROOT / "dist" / appimage_name(version)
    if not appimage.exists():
        sys.exit(f"{appimage} does not exist: run `package.py linux --native` first")
    import tempfile
    work = Path(tempfile.mkdtemp(prefix="mq-smoke-"))
    data = work / "data"
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", SDL_AUDIO_DRIVER="dummy")
    t0 = __import__("time").time()
    setup = subprocess.run([str(appimage), "--appimage-extract-and-run", "--data-dir", str(data), "--setup-rom", str(rom),
                            "--setup-only"], env=env, cwd=work, capture_output=True, text=True)
    print(setup.stdout[-3000:] + setup.stderr[-3000:])
    print(f"setup took {__import__('time').time() - t0:.0f} s", flush=True)
    if setup.returncode != 0 or not list((data / "game").rglob("manifest.ini")):
        sys.exit("setup failed")
    used = [l for l in (setup.stdout + setup.stderr).splitlines() if "compiler: " in l]
    if not used or "/usr/bin/toolchain/bin/clang" not in used[0]:
        sys.exit(f"setup did not use the package's own compiler: {used}")
    # (a new player's first start: no settings.ini yet, full screen at the Deck's 1280x800)
    run(["xvfb-run", "-a", "-s", "-screen 0 1280x800x24", appimage, "--appimage-extract-and-run", "--data-dir", data,
         "--fullscreen", "--unpaced", "--screenshot-at-frame", "900", work / "smoke.png", "--exit-after-frames", "910"],
        env=env, cwd=work)
    out = ROOT / "dist" / "smoke-linux.png"
    shutil.copy2(work / "smoke.png", out)
    shutil.rmtree(work, ignore_errors=True)
    print(f"\nSMOKE OK; boot screenshot: {out}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("target", choices=["windows", "linux"])
    ap.add_argument("--version", help=f"default: the git tag, or {DEFAULT_VERSION}")
    ap.add_argument("--llvm-dir", help=f"an installed LLVM {LLVM_VERSION} to take the compiler from (default: download it)")
    ap.add_argument("--no-build", action="store_true", help="package the last build in build/pkg-<target>")
    ap.add_argument("--smoke", metavar="ROM", help="linux: instead of packaging, set up and boot the last AppImage from this ROM (in a container; with --native, here)")
    ap.add_argument("--native", action="store_true", help="linux: build on this machine instead of in Docker (a test package: its glibc baseline is this system's)")
    ap.add_argument("--no-tests", action="store_true", help="linux --native: do not run the unit tests")
    args = ap.parse_args()
    if args.target == "windows":
        if platform.system() != "Windows":
            sys.exit("the Windows package is built on Windows")
        package_windows(args)
    else:
        package_linux(args)


if __name__ == "__main__":
    main()
