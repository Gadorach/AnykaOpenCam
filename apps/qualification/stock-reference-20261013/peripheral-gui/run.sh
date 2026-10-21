#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../../../.." && pwd)
exec "$ROOT/work/peripheral-gui-venv/bin/python" -B "$HERE/app.py" "$@"
