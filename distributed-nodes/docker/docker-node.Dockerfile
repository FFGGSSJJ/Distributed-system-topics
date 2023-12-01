FROM ubuntu
LABEL maintainer="gf"

USER root

RUN mkdir -p /usr/src/nodeapp
WORKDIR /usr/src/nodeapp

COPY ./ /usr/src/nodeapp

RUN chmod 755 /usr/src/nodeapp/node.bash
RUN apt -y update
RUN apt -y install make g++ python3

ENTRYPOINT [ "/usr/src/nodeapp/node.bash" ]