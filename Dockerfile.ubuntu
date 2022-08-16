FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

ENV LANGUAGE=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8

ADD dependencies.txt /dependencies.txt

RUN apt update -y && \
   apt install -y $(cat /dependencies.txt) < /dev/null

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

