# Use a devcontainer base image that includes a user and common tools.
FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

# --------------------------------------------------------------------
# Install prerequisites and custom tools
# --------------------------------------------------------------------
# Install LLVM, Clang, and other tools from the official repositories.
# This avoids compiling from source, which is much faster.
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ninja-build git wget curl ca-certificates \
    python3-pip \
    zlib1g-dev libedit-dev libxml2-dev libncurses5-dev libtinfo-dev \
    libssl-dev pkg-config lcov software-properties-common \
    gcc-arm-none-eabi libstdc++-arm-none-eabi-newlib  gdb-multiarch qemu-system-arm \
    clang clang-format clang-tidy clang-tools \
    lld \
    graphviz \
    libgtest-dev \
    libgmock-dev \
    cmake \
    babeltrace2 python3-bt2 vim \
    && rm -rf /var/lib/apt/lists/*

# --------------------------------------------------------------------
# Python tools: MkDocs, Doorstop, auto-versioning, coverage
# --------------------------------------------------------------------
RUN pip install --no-cache-dir --break-system-packages \
        mkdocs mkdocs-material doorstop gcovr pre-commit

# Set default user and workspace
USER vscode
WORKDIR /workspaces
