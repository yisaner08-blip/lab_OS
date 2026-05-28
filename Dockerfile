FROM ubuntu:20.04

ENV TZ=Asia/Shanghai
RUN ln -fs /usr/share/zoneinfo/$TZ /etc/localtime && \
    DEBIAN_FRONTEND=noninteractive apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y \
    gcc-multilib \
    make \
    perl \
    vim \
    qemu-system-x86

WORKDIR /workspace
