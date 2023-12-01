FROM ubuntu
LABEL maintainer="gf"

USER root

RUN mkdir -p /usr/src/loggerapp
RUN mkdir -p /usr/src/loggerapp/logs
WORKDIR /usr/src/loggerapp

COPY ./ /usr/src/loggerapp

RUN chmod 755 /usr/src/loggerapp/logger.bash
RUN apt -y update 
RUN apt -y install make g++

ENTRYPOINT [ "/usr/src/loggerapp/logger.bash" ]