#!/usr/bin/env python3
import argparse
import os
import shutil
import subprocess
import sys

# Add scripts to sys.path to import boards
sys.path.append(os.path.join(os.path.dirname(__file__), "scripts"))
from boards import BOARD_TARGET, SUPPORTED_BOARDS

BOARDS = list(SUPPORTED_BOARDS.keys())

STEPS = ["webapp", "splash", "firmware"]


def build_webapp():
    """Build the webapp (npm install + npm run build)."""
    print("\n=== Building webapp ===")
    try:
        subprocess.run("npm install", shell=True, check=True, cwd="webapp")
        subprocess.run("npm run build", shell=True, check=True, cwd="webapp")
    except subprocess.CalledProcessError as e:
        print(f"  ✗ Webapp build failed with exit code {e.returncode}")
        sys.exit(e.returncode)
    except FileNotFoundError:
        print(
            "  ✗ 'npm' not found. Please ensure Node.js is installed and in your PATH."
        )
        sys.exit(1)


def generate_splash(board):
    """Generate splash screen EPDGZ for the target board."""
    print(f"\n=== Generating splash screen for {board} ===", flush=True)
    output_dir = os.path.join(os.path.dirname(__file__), "main", "splash_data")
    script = os.path.join(os.path.dirname(__file__), "scripts", "generate_splash.py")
    process_cli_dir = os.path.join(os.path.dirname(__file__), "process-cli")

    # Ensure process-cli dependencies are installed
    node_modules = os.path.join(process_cli_dir, "node_modules")
    if not os.path.isdir(node_modules):
        print("  Installing process-cli dependencies...")
        try:
            subprocess.run("npm ci", shell=True, check=True, cwd=process_cli_dir)
        except subprocess.CalledProcessError as e:
            print(f"  ✗ npm ci failed in process-cli with exit code {e.returncode}")
            sys.exit(e.returncode)

    try:
        subprocess.run(
            [sys.executable, script, "--board", board, "--output-dir", output_dir],
            check=True,
        )
    except subprocess.CalledProcessError as e:
        print(f"  ✗ Splash generation failed with exit code {e.returncode}")
        sys.exit(e.returncode)


def idf_py_command():
    """Return the argv prefix that runs idf.py.

    On Windows, CreateProcess cannot execute the idf.py script directly
    (WinError 193). Prefer the idf.py.exe wrapper that the Windows installers
    put on PATH; otherwise run $IDF_PATH/tools/idf.py through the ESP-IDF
    Python interpreter (IDF_PYTHON_ENV_PATH is set by the activation script).
    """
    if os.name != "nt":
        return ["idf.py"]
    found = shutil.which("idf.py")
    if found and found.lower().endswith(".exe"):
        return [found]
    idf_path = os.environ.get("IDF_PATH")
    script = os.path.join(idf_path, "tools", "idf.py") if idf_path else found
    if not script or not os.path.isfile(script):
        return ["idf.py"]  # let subprocess raise FileNotFoundError
    env_path = os.environ.get("IDF_PYTHON_ENV_PATH")
    python = os.path.join(env_path, "Scripts", "python.exe") if env_path else ""
    if not os.path.isfile(python):
        python = sys.executable
    return [python, script]


def build_firmware(board, extra_args, read_pico_factory_diagnostic=False,
                   read_pico_display_validation=False, read_pico_storage_validation=False,
                   read_pico_photoframe_integration=False, read_pico_photo_validation=False):
    """Build firmware with idf.py."""
    if read_pico_photo_validation and (
        board != "read_pico" or not read_pico_factory_diagnostic
        or read_pico_display_validation or read_pico_storage_validation or read_pico_photoframe_integration
    ):
        raise ValueError("photo validation requires an exclusive Read Pico factory profile")
    if read_pico_storage_validation and (
        board != "read_pico" or not read_pico_factory_diagnostic or read_pico_display_validation
    ):
        raise ValueError("storage validation requires an exclusive Read Pico factory profile")
    if read_pico_photoframe_integration and (
        board != "read_pico" or not read_pico_factory_diagnostic
        or read_pico_display_validation or read_pico_storage_validation
    ):
        raise ValueError("PhotoFrame integration requires an exclusive Read Pico factory profile")
    print(f"\n=== Building firmware for {board} ===")
    sdkconfig_defaults = f"sdkconfig.defaults;boards/sdkconfig.defaults.{board}"
    if read_pico_factory_diagnostic:
        sdkconfig_defaults += ";boards/sdkconfig.defaults.read_pico_factory_diagnostic"
    if read_pico_display_validation:
        sdkconfig_defaults += ";boards/sdkconfig.defaults.read_pico_display_validation"
    if read_pico_storage_validation:
        sdkconfig_defaults += ";boards/sdkconfig.defaults.read_pico_storage_validation"
    if read_pico_photoframe_integration:
        sdkconfig_defaults += ";boards/sdkconfig.defaults.read_pico_photoframe_integration"
    if read_pico_photo_validation:
        sdkconfig_defaults += ";boards/sdkconfig.defaults.read_pico_photo_validation"

    # Boards differ in target chip (the M5Paper is a plain ESP32, everything
    # else is an ESP32-S3). Pass it explicitly: idf.py resolves the target
    # before it merges the defaults files, so CONFIG_IDF_TARGET in the board's
    # sdkconfig.defaults alone is not enough on a clean tree.
    target = BOARD_TARGET[board]

    idf_base = idf_py_command() + [
        f"-DIDF_TARGET={target}",
        f"-DSDKCONFIG_DEFAULTS={sdkconfig_defaults}",
    ]

    cmake_defines = [a for a in extra_args if a.startswith("-D")]
    post_build_args = [a for a in extra_args if not a.startswith("-D")]

    if board == "read_pico":
        if extra_args:
            print("Read Pico P1 is compile-only; extra/flash/monitor arguments are disabled.")
            sys.exit(1)
        os.makedirs(".cache", exist_ok=True)
        suffix = "-factory" if read_pico_factory_diagnostic else ""
        if read_pico_display_validation:
            suffix = "-display"
        if read_pico_storage_validation:
            suffix = "-storage"
        if read_pico_photoframe_integration:
            suffix = "-integration"
        if read_pico_photo_validation:
            suffix = "-photo"
        idf_base += ["-B", f"build-read-pico{suffix}",
                     f"-DSDKCONFIG=.cache/sdkconfig.read_pico{suffix}"]

    build_cmd = idf_base + cmake_defines + ["build"]
    print(f"Running: {' '.join(build_cmd)}")

    try:
        subprocess.run(build_cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Build failed with exit code {e.returncode}")
        sys.exit(e.returncode)
    except FileNotFoundError:
        print(
            "Error: 'idf.py' not found. Please ensure ESP-IDF is correctly installed and activated."
        )
        sys.exit(1)

    # Run post-build commands (flash, monitor, etc.)
    if post_build_args:
        post_cmd = idf_base + post_build_args
        print(f"Running: {' '.join(post_cmd)}")
        try:
            subprocess.run(post_cmd, check=True)
        except subprocess.CalledProcessError as e:
            print(f"Post-build command failed with exit code {e.returncode}")
            sys.exit(e.returncode)


def main():
    parser = argparse.ArgumentParser(description="Build firmware for different boards")
    parser.add_argument(
        "--board",
        choices=BOARDS,
        default="waveshare_photopainter_73",
        help="Board type to build",
    )
    parser.add_argument(
        "--read-pico-factory-diagnostic",
        action="store_true",
        help="Compile an isolated Read Pico diagnostic matching the verified Demo factory layout; never flash",
    )
    parser.add_argument(
        "--read-pico-display-validation",
        action="store_true",
        help="Compile command-armed display validation; requires the factory diagnostic profile; never flash",
    )
    parser.add_argument(
        "--read-pico-storage-validation",
        action="store_true",
        help="Compile isolated command-armed SD validation; requires factory profile; never flash",
    )
    parser.add_argument(
        "--read-pico-photoframe-integration",
        action="store_true",
        help="Compile/link full PhotoFrame stack with runtime disabled; requires isolated factory profile",
    )
    parser.add_argument(
        "--read-pico-photo-validation",
        action="store_true",
        help="Compile isolated single-photo diagnostic; requires factory profile; never flash",
    )
    parser.add_argument(
        "--fullclean",
        action="store_true",
        help="Remove sdkconfig and run idf.py fullclean before building",
    )
    parser.add_argument(
        "--step",
        choices=STEPS,
        action="append",
        help="Run only specific step(s). Can be specified multiple times. "
        "If omitted, all steps run.",
    )
    # Allow passing extra arguments to idf.py
    args, extra_args = parser.parse_known_args()

    steps = args.step if args.step else STEPS

    if args.board == "read_pico" and (args.fullclean or extra_args):
        parser.error("Read Pico P1 only permits building; no fullclean or extra commands")
    if args.read_pico_factory_diagnostic and (
        args.board != "read_pico" or steps != ["firmware"]
    ):
        parser.error("factory diagnostic requires --board read_pico --step firmware only")
    if args.read_pico_display_validation and not args.read_pico_factory_diagnostic:
        parser.error("display validation requires --read-pico-factory-diagnostic")
    if args.read_pico_storage_validation and not args.read_pico_factory_diagnostic:
        parser.error("storage validation requires --read-pico-factory-diagnostic")
    if args.read_pico_storage_validation and args.read_pico_display_validation:
        parser.error("storage and display validation must use separate profiles")
    if args.read_pico_photoframe_integration and (
        not args.read_pico_factory_diagnostic or args.read_pico_display_validation
        or args.read_pico_storage_validation
    ):
        parser.error("PhotoFrame integration requires an exclusive factory profile")
    if args.read_pico_photo_validation and (
        not args.read_pico_factory_diagnostic or args.read_pico_display_validation
        or args.read_pico_storage_validation or args.read_pico_photoframe_integration
    ):
        parser.error("photo validation requires an exclusive factory profile")

    if args.fullclean:
        print("Performing full clean...")
        for f in ["sdkconfig", "partitions.csv"]:
            if os.path.exists(f):
                os.remove(f)
                print(f"  ✓ Removed {f}")
        if os.path.isdir("build"):
            shutil.rmtree("build")
            print("  ✓ Removed build/")

    if "webapp" in steps:
        build_webapp()

    if "splash" in steps:
        generate_splash(args.board)

    if "firmware" in steps:
        build_firmware(args.board, extra_args, args.read_pico_factory_diagnostic,
                       args.read_pico_display_validation, args.read_pico_storage_validation,
                       args.read_pico_photoframe_integration, args.read_pico_photo_validation)


if __name__ == "__main__":
    main()
