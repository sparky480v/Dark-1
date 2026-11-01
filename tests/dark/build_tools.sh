#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Native test tools for DARK-1.  Usage: tests/dark/build_tools.sh [TREE]   (TREE defaults to this tree)
set -e
T="${1:-$(dirname "$0")/../..}"
H="$(dirname "$0")"
mkdir -p "$T/build/dark"
cc -O1 -std=gnu99 -w -DEN_HOST -I"$T/firmware/src" -I"$T/firmware/src/dsp" -o "$T/build/dark/recipe_dump" "$H/recipe_dump.c" "$T/firmware/src/dsp/amb.c" -lm
echo "built $T/build/dark/recipe_dump"
