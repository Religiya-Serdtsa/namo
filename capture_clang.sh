#!/bin/bash
cat > margo_input.c
REAL_CLANG_BIN="${REAL_CLANG:-clang}"
# Use the same logic as scripts/margo_clang_wrapper.sh but with captured input
read -r -a EXTRA_CFLAGS <<< "${MARGO_EXTRA_CFLAGS:-}"
read -r -a EXTRA_OBJS <<< "${MARGO_EXTRA_OBJS:-}"
read -r -a EXTRA_LIBS <<< "${MARGO_EXTRA_LIBS:-}"

"${REAL_CLANG_BIN}" "${EXTRA_CFLAGS[@]}" "$@" "${EXTRA_OBJS[@]}" "${EXTRA_LIBS[@]}" < margo_input.c
