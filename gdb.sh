#!/usr/bin/env bash
cd "$(dirname "$0")"
exec /usr/bin/gdb -nx -x debug.gdb
