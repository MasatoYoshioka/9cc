FROM ubuntu:latest

RUN apt update && apt install -y gcc make binutils libc6-dev gdb
RUN mkdir -p /workspace

WORKDIR /workspace
