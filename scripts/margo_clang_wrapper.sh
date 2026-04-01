#!/usr/bin/env bash
set -euo pipefail

REAL_CLANG_BIN="${REAL_CLANG:-clang}"
ARGS=("$@")

if [[ -n "${MARGO_EXTRA_CFLAGS:-}" ]]; then
	read -r -a EXTRA_CFLAGS <<< "${MARGO_EXTRA_CFLAGS}"
	ARGS=("${EXTRA_CFLAGS[@]}" "${ARGS[@]}")
fi

if [[ -n "${MARGO_EXTRA_OBJS:-}" ]]; then
	read -r -a EXTRA_OBJS <<< "${MARGO_EXTRA_OBJS}"
	ARGS+=("${EXTRA_OBJS[@]}")
fi

if [[ -n "${MARGO_EXTRA_LIBS:-}" ]]; then
	read -r -a EXTRA_LIBS <<< "${MARGO_EXTRA_LIBS}"
	ARGS+=("${EXTRA_LIBS[@]}")
fi

exec "${REAL_CLANG_BIN}" "${ARGS[@]}"
