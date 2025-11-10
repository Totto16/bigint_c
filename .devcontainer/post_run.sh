#!/usr/bin/env bash

set -xe

TARGET_DIR="/runtime/build_sym_link"

if [ -e "$TARGET_DIR" ]; then
    rm "$TARGET_DIR"
fi

ARCH="$(uname -m)"

if [ "$ARCH" = "riscv64" ]; then
    ln -s /work/build_riscv "$TARGET_DIR"
elif [ "$ARCH" = "aarch64" ]; then
    ln -s /work/build_arm64 "$TARGET_DIR"
elif [ "$ARCH" = "x86_64" ]; then
    ln -s /work/build "$TARGET_DIR"
else
    echo "invalid arch: '$ARCH'"
    exit 2
fi
