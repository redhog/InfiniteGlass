FROM ubuntu:18.04

RUN apt update -y
RUN apt install -y build-essential
RUN apt install -y python
RUN apt install -y python3
RUN apt install -y python3-pip
RUN apt install -y virtualenv
RUN apt install -y pkg-config
RUN apt install -y librsvg2-dev
RUN apt install -y libpango1.0-dev
RUN apt install -y libxi-dev
RUN apt install -y libxtst-dev
RUN apt install -y libxrender-dev
RUN apt install -y libxrandr-dev
RUN apt install -y libxcomposite-dev
RUN apt install -y libxdamage-dev
RUN apt install -y libglew-dev
RUN apt install -y xserver-xephyr < /dev/null
RUN apt install -y xinit < /dev/null
RUN apt install -y python3-setuptools
RUN apt install -y x11-xkb-utils
RUN apt install -y rofi
RUN apt install -y xterm
RUN apt install -y chromium-browser
RUN apt install -y emacs
RUN apt install -y wget
RUN apt install -y unzip
RUN apt install -y sudo
RUN apt install -y dbus-x11
RUN apt install -y pluma
RUN apt install -y locales
RUN apt install -y locales-all
RUN locale-gen en_US.UTF-8
RUN apt install -y git
RUN apt install -y curl
RUN apt install -y zsh
RUN apt install -y sakura
RUN apt install -y vim
RUN apt install -y figlet

ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8

RUN echo Version 2

ADD . /InfiniteGlass

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
RUN usermod --shell /usr/bin/zsh glass

USER glass

RUN cd /home/glass; \
    set -uex; \
    wget https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh; \
    sh ./install.sh; \
    rm ./install.sh
RUN sed -i 's/robbyrussell/dieter/g' /home/glass/.zshrc
RUN echo "\nfiglet -f slant InfiniteGlass" >> /home/glass/.zshrc

USER root

CMD /InfiniteGlass/scripts/docker-runner.sh
