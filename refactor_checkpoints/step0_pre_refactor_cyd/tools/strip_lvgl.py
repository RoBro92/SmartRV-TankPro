import shutil
from pathlib import Path

from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()


def prune_lvgl(target, source, env):  # pylint: disable=unused-argument
    lvgl_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV") / "lvgl"
    if not lvgl_dir.exists():
        return

    print(f"Trimming LVGL sources in: {lvgl_dir}")

    # Drop architecture-specific backends that break the Xtensa toolchain and waste build time.
    prune_paths = [
        "src/draw/sw/blend/helium",
        "src/draw/sw/blend/neon",
        "src/draw/convert/helium",
        "src/draw/arm2d",
        "src/draw/nxp",
        "src/draw/renesas",
        "src/draw/sdl",
        "src/draw/espressif/ppa",
    ]

    for rel in prune_paths:
        path = lvgl_dir / rel
        if path.exists():
            shutil.rmtree(path, ignore_errors=True)

    # Ensure the helium blend assembly file is gone (it does not build on Xtensa).
    helium_file = lvgl_dir / "src/draw/sw/blend/helium/lv_blend_helium.S"
    if helium_file.exists():
        helium_file.unlink()


prune_lvgl(None, None, env)
env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", prune_lvgl)
