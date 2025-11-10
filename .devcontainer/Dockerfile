ARG UBUNTU_VARIANT=24.04
ARG BUILD_PLATFORM

FROM --platform=${BUILD_PLATFORM} ubuntu:${UBUNTU_VARIANT}

LABEL devcontainer.metadata='[{ \
    "remoteUser": "ubuntu", \
    "features": {} \
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


COPY ./symlink_clang_utils.sh ./post_run.sh /tmp/

#symlink all clang 21 things to better paths e.g. clang-21 to /usr/bin/clang
RUN chmod +x /tmp/symlink_clang_utils.sh \
    && chmod +x /tmp/post_run.sh \
    && mkdir -pv /runtime/ \
    && chown ubuntu:ubuntu /runtime/ \
    && chmod 755 /runtime/ \
    && /tmp/symlink_clang_utils.sh 21 \
    && rm /tmp/symlink_clang_utils.sh

USER ubuntu
WORKDIR /work
