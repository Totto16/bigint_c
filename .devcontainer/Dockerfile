ARG UBUNTU_VARIANT=24.04
ARG BUILD_PLATFORM

FROM --platform=${BUILD_PLATFORM} ubuntu:${UBUNTU_VARIANT}

LABEL devcontainer.metadata='[{ \
    "remoteUser": "ubuntu", \
    "features": {},\
    "mounts": [   "source=${localWorkspaceFolder},target=/work,type=bind,consistency=cached" ], \
    "customizations": {  \
    "vscode": { \
    "extensions": [\
    "xaver.clang-format",\
    "llvm-vs-code-extensions.vscode-clangd",\
    "mesonbuild.mesonbuild"\
    ],\
    "settings": { \
    "terminal.integrated.shell.linux": "/bin/bash", \
    "clangd.path": "/usr/bin/clangd-21",\
    "clangd.arguments": [\
    "--all-scopes-completion",\
    "--background-index",\
    "--background-index-priority=background",\
    "--clang-tidy",\
    "--completion-style=bundled",\
    "--function-arg-placeholders",\
    "--enable-config",\
    "-j",\
    "3",\
    "--malloc-trim",\
    "--pretty",\
    "--header-insertion=never",\
    "--compile-commands-dir=build_sym_link"\
    ],\
    "[c]": {\
    "editor.defaultFormatter": "xaver.clang-format"\
    },\
    "[cpp]": {\
    "editor.defaultFormatter": "xaver.clang-format"\
    }\
    }\
    }\
    }\
    }]'

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

ENV CC_LD="mold"
ENV LD="mold"
ENV CXX_LD="mold"

# Install common utilities, gcc 14 and clang 21, also prepare meson
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    sudo \
    bash-completion \
    build-essential \
    ca-certificates \
    curl \
    git \
    less \
    locales \
    wget \
    gcc-14 g++-14 \
    python3-pip \
    libgmp-dev \
    ninja-build \
    pkg-config \
    mold \
    lsb-release wget software-properties-common gnupg \
    && pip install meson --break-system-packages \
    && wget apt.llvm.org/llvm.sh -O "/tmp/llvm.sh" \
    && chmod +x /tmp/llvm.sh \
    && /tmp/llvm.sh 21\
    && rm /tmp/llvm.sh \
    && apt-get install -y --no-install-recommends \
    clang-tidy-21 clang-format-21 clangd-21 clang-21 \
    clang-tools-21 lld-21 lldb-21 \
    && rm -rf /var/lib/apt/lists/*


#symlink all clang 21 things to better paths e.g. clang-21 to /usr/bin/clang
COPY ./symlink_clang_utils.sh /tmp/

RUN chmod +x /tmp/symlink_clang_utils.sh \
    && /tmp/symlink_clang_utils.sh 21 \
    && rm /tmp/symlink_clang_utils.sh

RUN if [ -e "/work/build_sym_link" ]; then \
    rm /work/build_sym_link; \
    fi \
    && \
    ARCH="$(uname -m)" \
    && \
    if [ "$ARCH" = "riscv64" ]; then \
    ln -s /work/build_riscv /work/build_sym_link; \
    elif [ "$ARCH" = "aarch64" ]; then \
    ln -s /work/build_arm64 /work/build_sym_link; \
    elif [ "$ARCH" = "x86_64" ]; then \
    ln -s /work/build /work/build_sym_link; \
    else \
    echo "invalid arch: '$ARCH'"; \
    exit 2; \
    fi


USER ubuntu
WORKDIR /work
