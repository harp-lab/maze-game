FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    git \
    llvm \
    clang \
    make \
    ffmpeg \
    graphicsmagick \
    libgraphicsmagick++1-dev \
    libboost-all-dev 


WORKDIR /root

RUN git clone https://github.com/harp-lab/maze-game

WORKDIR /root/maze-game

ENTRYPOINT ["tail", "-f", "/dev/null"]
