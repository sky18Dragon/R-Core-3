#!/usr/bin/env python3
"""Check local Markdown links and anchors in the current docs.

Scans root *.md and docs/*.md. docs/history, docs/archive and docs/reference
keep original text with possibly stale links and are not scanned.
Exit status 1 when any link or anchor is broken.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LINK = re.compile(r"\]\(([^)\s]+)\)")
FENCE = re.compile(r"^```")


def anchors(path):
    found = set()
    in_fence = False
    for line in path.read_text(encoding="utf-8").splitlines():
        if FENCE.match(line):
            in_fence = not in_fence
        if in_fence:
            continue
        m = re.match(r"^#{1,6}\s+(.*?)\s*$", line)
        if m:
            slug = re.sub(r"[^\w一-鿿\- ]", "", m.group(1).lower())
            found.add(slug.replace(" ", "-"))
    return found


def main():
    files = sorted(ROOT.glob("*.md")) + sorted((ROOT / "docs").glob("*.md"))
    bad = 0
    total = 0
    for md in files:
        in_fence = False
        for no, line in enumerate(md.read_text(encoding="utf-8").splitlines(), 1):
            if FENCE.match(line):
                in_fence = not in_fence
            if in_fence:
                continue
            for target in LINK.findall(line):
                if re.match(r"^(https?:|mailto:)", target):
                    continue
                total += 1
                path_part, _, anchor = target.partition("#")
                dest = md if not path_part else (md.parent / path_part).resolve()
                if not dest.exists():
                    print(f"{md.relative_to(ROOT)}:{no}: missing {target}")
                    bad += 1
                elif anchor and dest.suffix == ".md" and anchor not in anchors(dest):
                    print(f"{md.relative_to(ROOT)}:{no}: missing anchor {target}")
                    bad += 1
    print(f"checked {total} links in {len(files)} files, {bad} broken")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
