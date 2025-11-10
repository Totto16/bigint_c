ARG UBUNTU_VARIANT=24.04
ARG BUILD_PLATFORM

FROM --platform=${BUILD_PLATFORM} ubuntu:${UBUNTU_VARIANT}

LABEL devcontainer.metadata='[{ \
    "remoteUser": "ubuntu", \
    "features": {} \
    }]'

COPY ./post_run.sh ./install_clang.sh /tmp/

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

ENV CC_LD="mold"
ENV LD="mold"
ENV CXX_LD="mold"

# Install common utilities, gcc 14 and clang, also prepare meson
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
    && pip install meson --break-system-packages \
    && chmod +x /tmp/install_clang.sh \
    && /tmp/install_clang.sh \
    && rm /tmp/install_clang.sh \
    && rm -rf /var/lib/apt/lists/* 

# setup post run script and directories
RUN chmod +x /tmp/post_run.sh \
    && mkdir -pv /runtime/ \
    && chown ubuntu:ubuntu /runtime/ \
    && chmod 755 /runtime/ 

USER ubuntu
WORKDIR /work
