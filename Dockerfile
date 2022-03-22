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

RUN apt install -y libxcb-composite0-dev libxcb-cursor-dev \
   libxcb-damage0-dev libxcb-dpms0-dev libxcb-dri2-0-dev libxcb-dri3-dev \
   libxcb-ewmh-dev libxcb-glx0-dev libxcb-icccm4-dev libxcb-image0-dev \
   libxcb-imdkit-dev libxcb-keysyms1-dev libxcb-present-dev \
   libxcb-randr0-dev libxcb-record0-dev libxcb-render-util0-dev \
   libxcb-render0-dev libxcb-res0-dev libxcb-screensaver0-dev \
   libxcb-shape0-dev libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev \
   libxcb-util0-dev libxcb-xf86dri0-dev libxcb-xfixes0-dev \
   libxcb-xinerama0-dev libxcb-xinput-dev libxcb-xkb-dev libxcb-xrm-dev \
   libxcb-xtest0-dev libxcb-xv0-dev libxcb-xvmc0-dev libxcb1-dev libx11-xcb-dev

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

