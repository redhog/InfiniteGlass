FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

ENV LANGUAGE=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8

RUN apt update -y && \
   apt install -y build-essential pkg-config locales locales-all \
      gobject-introspection libgirepository1.0-dev \
      python python3 python3-pip virtualenv python3-setuptools \
      librsvg2-dev libpango1.0-dev libcairo2 libcairo2-dev \
      libxi-dev libxtst-dev libxrender-dev libxrandr-dev \
      libxcomposite-dev libxdamage-dev libglew-dev \
      x11-xkb-utils dbus-x11
RUN apt install -y xserver-xephyr < /dev/null
RUN apt install -y xinit < /dev/null
RUN apt install -y wget curl git zsh unzip sudo \
   xterm sakura pluma emacs vim rofi
RUN apt install -y chromium-browser

RUN locale-gen en_US.UTF-8

RUN pip3 install --upgrade pip

RUN echo Version 2

ADD . /InfiniteGlass

RUN sed -e '/PyGObject/d' /InfiniteGlass/glass-action/setup.py -i

RUN cd /InfiniteGlass; make all
RUN cd /InfiniteGlass; make devinstall

RUN useradd glass
RUN mkdir -p /home/glass
RUN mkdir -p /home/glass/.config/session-state
RUN mkdir -p /home/glass/.config/dconf
RUN chown -R glass:users /home/glass
RUN chmod -R ugo+rw /InfiniteGlass
RUN sed -e "s+\(sudo:.*\)+\1glass+g" /etc/group -i
RUN sed -e "s+ALL$+NOPASSWD: ALL+g" /etc/sudoers -i

ENTRYPOINT /InfiniteGlass/scripts/docker-runner.sh

