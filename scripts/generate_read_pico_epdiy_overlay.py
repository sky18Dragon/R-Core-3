#!/usr/bin/env python3
"""生成固定 EPDiy 源码的最小扫描覆盖副本。/ Generate minimal scan overlays for pinned EPDiy sources."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


EXPECTED_SHA256 = {
    "src/render.c": "f288db9ce8dcbe9af23d4226fe2416c90b6c68527050d11645cbd7c4fb2e146e",
    "src/output_lcd/lcd_driver.c":
        "7287dffa1d5380b94b36076493a81b9607ca108c9f677e89cf9b932f22628b0c",
}

REPLACEMENTS = {
    "src/render.c": (
        (
            "return (((epd_height() + 7) / 8) * 8);",
            "return (((epd_height() + 11) / 12) * 12);",
        ),
    ),
    "src/output_lcd/lcd_driver.c": (
        ("#define BOUNCE_BUF_LINES 4", "#define BOUNCE_BUF_LINES 6"),
        (
            "lcd.display_lines = (((display_height + 7) / 8) * 8);",
            "lcd.display_lines = (((display_height + 11) / 12) * 12);",
        ),
    ),
}


class OverlayError(RuntimeError):
    """固定源码或替换锚点发生漂移。/ The pinned source or replacement anchors drifted."""


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate(source_root: Path, output_dir: Path) -> tuple[Path, Path]:
    """校验固定源码并生成两个覆盖文件。/ Validate pinned sources and generate two overlay files."""
    source_root = source_root.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    generated: list[Path] = []
    for relative, expected_hash in EXPECTED_SHA256.items():
        source = source_root / relative
        actual_hash = _sha256(source)
        if actual_hash != expected_hash:
            raise OverlayError(
                f"refusing EPDiy overlay for {relative}: expected {expected_hash}, got {actual_hash}"
            )
        original = source.read_text(encoding="utf-8")
        patched = original
        for old, new in REPLACEMENTS[relative]:
            if patched.count(old) != 1:
                raise OverlayError(
                    f"refusing EPDiy overlay for {relative}: anchor count for {old!r} is "
                    f"{patched.count(old)}, expected 1"
                )
            patched = patched.replace(old, new)
        if patched == original:
            raise OverlayError(f"refusing empty EPDiy overlay for {relative}")
        destination = output_dir / Path(relative).name
        destination.write_text(patched, encoding="utf-8")
        generated.append(destination)
    return generated[0], generated[1]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    try:
        generated = generate(args.source_root, args.output_dir)
    except (OSError, UnicodeError, OverlayError) as exc:
        parser.error(str(exc))
    for path in generated:
        print(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
