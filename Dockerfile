FROM ubuntu:18.10

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

RUN echo Version 2

ADD . /InfiniteGlass

RUN cd /InfiniteGlass; make all
RUN cd /InfiniteGlass; make devinstall

RUN useradd glass
RUN mkdir -p /home/glass
RUN chown glass:users /home/glass
RUN chmod -R ugo+rw /InfiniteGlass

CMD /InfiniteGlass/docker-runner.sh
