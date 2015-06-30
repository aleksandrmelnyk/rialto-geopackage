FROM centos:centos7
MAINTAINER mpg@flaxen.com

VOLUME /sampledata

ENV OSSIM_DEV_HOME=/tmp/sources/ossim \
    OSSIM_BUILD_DIR=/tmp/builds/ossim \
    OSSIM_DEPENDENCIES=/usr

COPY ./build.sh /tmp/
RUN /tmp/build.sh

CMD /bin/bash
