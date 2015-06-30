FROM centos:centos7
MAINTAINER mpg@flaxen.com

VOLUME /sampledata
EXPOSE 8080


RUN true \
    && ln -s /sampledata /opt/www

COPY geopackage_server.py /usr/local/bin/

CMD /usr/local/bin/geopackage_server.py 8080 /opt/www
