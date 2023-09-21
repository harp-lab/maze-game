# Getting the official image
FROM archlinux

RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm git vim make graphicsmagick clang boost ghostscript python ffmpeg

WORKDIR /root
RUN git clone https://github.com/harp-lab/maze-game

WORKDIR /root/maze-game

ENTRYPOINT ["tail", "-f", "/dev/null"]