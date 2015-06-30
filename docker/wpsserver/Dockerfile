FROM radiantbluetechnologies/rialto-geopackage-tools
MAINTAINER mpg@flaxen.com

EXPOSE 8080

VOLUME /sampledata

ENV JAVA_HOME /usr/lib/jvm/java-1.7.0-openjdk-1.7.0.79-2.5.5.2.el7_1.x86_64/jre/
ENV GEOSERVER_HOME /usr/local/geoserver 
ENV GEOSERVER_PLUGIN_DIR $GEOSERVER_HOME/webapps/geoserver/WEB-INF/lib/
ENV GEOSERVER_SCRIPT_DIR $GEOSERVER_HOME/data_dir/scripts/

COPY ./build.sh /tmp/
RUN /tmp/build.sh

CMD cd $GEOSERVER_HOME/bin; sh ./startup.sh
