FROM ubuntu:jammy AS build

# configure the shell before the first RUN
SHELL ["/bin/bash", "-ex", "-o", "pipefail", "-c"]

WORKDIR /home/root

COPY ./rpctest ./rpctest
COPY spvnode .

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    python3 \
    python3-requests \
    libevent-dev

RUN ./rpctest/fetch.py --host x86_64-linux-gnu
