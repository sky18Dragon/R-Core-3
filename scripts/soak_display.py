#!/usr/bin/env python3
"""Repeated-refresh soak test for a running frame (R2 long-run item).

Cycles the album's images through POST /api/display, records each status and
duration, and checks the device stays reachable between refreshes. Read-only
apart from changing which image is shown; never uploads, deletes or changes
settings. Standard library only.

    python3 scripts/soak_display.py --host 192.168.1.21 --count 100 --interval 10
"""
import argparse
import csv
import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request


def request(host, path, data=None, timeout=60):
    body = None if data is None else json.dumps(data).encode()
    req = urllib.request.Request(f"http://{host}{path}", data=body,
                                 headers={"Content-Type": "application/json"} if body else {})
    started = time.monotonic()
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, resp.read(), time.monotonic() - started
    except urllib.error.HTTPError as err:
        return err.code, err.read(), time.monotonic() - started
    except (urllib.error.URLError, OSError) as err:
        return 0, str(err).encode(), time.monotonic() - started


def album_images(host, album):
    status, body, _ = request(host, "/api/images?album=" + urllib.parse.quote(album), timeout=15)
    if status != 200:
        sys.exit(f"cannot list album {album!r}: HTTP {status} {body[:200]!r}")
    return [f"{album}/{item['filename']}" for item in json.loads(body)]


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--host", required=True)
    parser.add_argument("--album", default="Default")
    parser.add_argument("--count", type=int, default=100)
    parser.add_argument("--interval", type=float, default=10.0, help="seconds between refresh starts")
    parser.add_argument("--csv", help="write one row per refresh")
    args = parser.parse_args()

    images = album_images(args.host, args.album)
    if not images:
        sys.exit("album is empty")
    rows, failures, unreachable = [], 0, 0
    for i in range(args.count):
        started = time.monotonic()
        path = images[i % len(images)]
        status, body, seconds = request(args.host, "/api/display", {"filepath": path}, timeout=120)
        alive, _, _ = request(args.host, "/api/time", timeout=10)
        ok = status == 200
        failures += not ok
        unreachable += alive != 200
        rows.append({"n": i + 1, "image": path, "status": status, "seconds": round(seconds, 2),
                     "alive": alive, "detail": "" if ok else body[:120].decode(errors="replace")})
        print(f"{i + 1:4d}/{args.count} {status} {seconds:6.2f}s alive={alive} {path}", flush=True)
        time.sleep(max(0.0, args.interval - (time.monotonic() - started)))
    durations = sorted(r["seconds"] for r in rows if r["status"] == 200)
    summary = {
        "refreshes": args.count, "failures": failures, "unreachable_checks": unreachable,
        "median_s": durations[len(durations) // 2] if durations else None,
        "max_s": durations[-1] if durations else None,
    }
    print(json.dumps(summary))
    if args.csv:
        with open(args.csv, "w", newline="") as out:
            writer = csv.DictWriter(out, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
    return 1 if failures or unreachable else 0


if __name__ == "__main__":
    sys.exit(main())
