FROM ubuntu
LABEL maintainer="gf"

USER root

COPY ./ /

RUN chmod 755 /node.bash
RUN mkdir -p /logs && \ 
    apt -y update && \
    apt -y install make g++ python3

ENTRYPOINT [ "/node.bash" ]