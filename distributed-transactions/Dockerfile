FROM ubuntu
LABEL maintainer="gf"

USER root

COPY ./ /

RUN apt -y update && \
    apt -y install make g++ python3

ENTRYPOINT [ "/node.bash" ]