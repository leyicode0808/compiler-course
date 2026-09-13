#!/usr/bin/env sh
set -eu

IMAGE=${1:-cmm-irsim-py38}

docker build --network host -t "$IMAGE" - <<'DOCKERFILE'
FROM python:3.8

ENV QT_QPA_PLATFORM=offscreen

RUN apt-get update -qq \
    && apt-get install -y -qq libgl1 \
    && rm -rf /var/lib/apt/lists/*

RUN pip install -q PyQt5==5.15.10
DOCKERFILE
