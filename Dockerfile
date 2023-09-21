FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    git \
    vim \
    make \
    graphicsmagick \
    clang \
    libboost-all-dev \
    ghostscript \
    python3 \
    ffmpeg 


WORKDIR /root

RUN git clone https://github.com/harp-lab/maze-game

WORKDIR /root/maze-game

ENTRYPOINT ["tail", "-f", "/dev/null"]
