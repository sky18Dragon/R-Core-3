#!/usr/bin/env bash
# Run the upstream tests, including targets omitted by `make test`.
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

node_major="$(node -p 'process.versions.node.split(".")[0]')"
if [[ "$node_major" != "22" ]]; then
    echo "Use Node 22 for this baseline (see .node-version)." >&2
    exit 1
fi

npm --prefix process-cli ci
make test
ctest --test-dir host_tests/build --output-on-failure
python3 host_tests/test_read_pico_build.py
python3 host_tests/test_read_pico_console.py
npm --prefix webapp ci
npm --prefix webapp test
npm --prefix webapp run build
