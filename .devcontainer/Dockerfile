ARG UBUNTU_VARIANT=24.04
ARG BUILD_PLATFORM
FROM --platform=${BUILD_PLATFORM} ubuntu:${UBUNTU_VARIANT}

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

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
    lsb-release wget software-properties-common gnupg \
    && pip install meson --break-system-packages \
    && wget apt.llvm.org/llvm.sh -O "/tmp/llvm.sh" \
    && chmod +x /tmp/llvm.sh \
    && /tmp/llvm.sh 21\
    && rm /tmp/llvm.sh \
    && rm -rf /var/lib/apt/lists/*


#symlink all clang 21 things to better paths e.g. clang-21 to /usr/bin/clang
COPY ./symlink_clang_utils.sh /tmp/

RUN chmod +x /tmp/symlink_clang_utils.sh \
    && /tmp/symlink_clang_utils.sh 21 \
    && rm /tmp/symlink_clang_utils.sh

USER ubuntu
WORKDIR /work
