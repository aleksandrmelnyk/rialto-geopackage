#!/bin/bash

set -e

#
# Java
#
yum -y install java-1.7.0-openjdk-devel

#
# GeoServer
#
curl -L http://sourceforge.net/projects/geoserver/files/GeoServer/2.7.1/geoserver-2.7.1-bin.zip -o /tmp/geoserver.zip
unzip -d /usr/local /tmp/geoserver.zip
ln -s /usr/local/geoserver-2.7.1 /usr/local/geoserver
rm -f /tmp/geoserver.zip

#
# WPS extension
#

# this dir normally gets created when server is first run, but we haven't run the server yet
mkdir -p $GEOSERVER_SCRIPT_DIR/wps/
curl -L http://sourceforge.net/projects/geoserver/files/GeoServer/2.7.1/extensions/geoserver-2.7.1-wps-plugin.zip -o /tmp/geoserver-wps.zip
unzip -d $GEOSERVER_PLUGIN_DIR /tmp/geoserver-wps.zip
rm -f /tmp/geoserver-wps.zip

#
# Groovy extension
#
curl -L http://ares.boundlessgeo.com/geoserver/2.7.x/community-latest/geoserver-2.7-SNAPSHOT-groovy-plugin.zip -o /tmp/geoserver-groovy.zip
unzip -o -d $GEOSERVER_PLUGIN_DIR /tmp/geoserver-groovy.zip
rm -f /tmp/geoserver-groovy.zip

#
# download our WPS scripts
#
curl -L https://github.com/radiantbluetechnologies/rialto-geopackage/archive/master.zip -o /tmp/geopackage.zip
unzip -o -d /tmp /tmp/geopackage.zip
cp /tmp/rialto-geopackage-master/wps/* $GEOSERVER_SCRIPT_DIR/wps
rm -rf /tmp/geopackage.zip /tmp/rialto-geopackage-master
