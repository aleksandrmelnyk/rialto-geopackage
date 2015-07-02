#!/bin/bash

set -e

#
# base libs
#
yum -y install epel-release
yum -y install \
    cmake \
    gcc-c++ \
    git \
    make \
    scons \
    subversion \
    unzip \
    wget
yum -y install \
    boost-devel \
    expat-devel \
    freetype-devel \
    gdal-devel \
    geos-devel \
    hdf5-devel \
    laszip-devel \
    libcurl-devel \
    libjpeg-devel \
    libgeotiff-devel \
    libpng-devel \
    libtiff-devel \
    libxml2-devel \
    libzip-devel \
    minizip-devel \
    proj-devel \
    sqlite-devel
rpm -Uhv http://artifacts.codice.org/content/repositories/yum/com/radiantblue/openthreads/2.4.0/openthreads-2.4.0-x86_64.codice.rpm
exit 0

mkdir -p /tmp/sources
mkdir -p /tmp/builds

#
# PDAL
#
curl -L https://github.com/radiantbluetechnologies/archive/master.zip -o /tmp/sources/pdal.zip
cd /tmp/sources
unzip /tmp/sources/pdal.zip
mv PDAL-master/ pdal/
mkdir -p /tmp/builds/pdal
cd /tmp/builds/pdal
cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    /tmp/sources/pdal/
make -j 4 install
mv /usr/lib/libpdalcpp.so /usr/lib64/
mv /usr/lib/libpdal_util.so /usr/lib64/
make test

#
# rialto geopckage
#
curl -L https://github.com/radiantbluetechnologies/rialto-geopackage/archive/master.zip -o /tmp/sources/geopackage.zip
cd /tmp/sources
unzip /tmp/sources/geopackage.zip
mv /tmp/sources/rialto-geopackage-master/ /tmp/sources/geopackage/
cd /tmp/sources/geopackage
scons -j 4 debug=0 install_prefix=/usr pdal_prefix=/usr
mv -f /usr/lib/librialto.so /usr/lib64/
./mybuild/test/rialtotest

#
# OSSIM
#
cd /tmp/sources
svn checkout https://svn.osgeo.org/ossim/trunk ossim
mkdir -p $OSSIM_BUILD_DIR
cd $OSSIM_BUILD_DIR
cp -f $OSSIM_DEV_HOME/ossim_package_support/cmake/CMakeLists.txt $OSSIM_DEV_HOME/
cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE="Release" \
    -DCMAKE_ECLIPSE_MAKE_ARGUMENTS=-j8 \
    -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DBUILD_CSMAPI=OFF \
    -DBUILD_LIBRARY_DIR=lib \
    -DBUILD_OMS=ON \
    -DBUILD_OSSIM=ON \
    -DBUILD_OSSIM_PLUGIN=OFF  \
    -DBUILD_OSSIM_TEST_APPS=ON \
    -DBUILD_OSSIMCONTRIB_PLUGIN=OFF \
    -DBUILD_OSSIMCSM_PLUGIN=OFF \
    -DBUILD_OSSIMGDAL_PLUGIN=ON \
    -DBUILD_OSSIMGEOPDF_PLUGIN=OFF \
    -DBUILD_OSSIMHDF4_PLUGIN=OFF \
    -DBUILD_OSSIMHDF5_PLUGIN=ON \
    -DBUILD_OSSIMJNI=ON \
    -DBUILD_OSSIMKAKADU_PLUGIN=OFF \
    -DBUILD_OSSIMKMLSUPEROVERLAY_PLUGIN=ON \
    -DBUILD_OSSIMLAS_PLUGIN=ON \
    -DBUILD_OSSIMLIBRAW_PLUGIN=OFF \
    -DBUILD_OSSIMMRSID_PLUGIN=OFF \
    -DBUILD_OSSIMNDF_PLUGIN=OFF \
    -DBUILD_OSSIMPDAL_PLUGIN=ON \
    -DBUILD_OSSIMPNG_PLUGIN=ON \
    -DBUILD_OSSIMREGISTRATION_PLUGIN=OFF \
    -DBUILD_OSSIMQT4=OFF \
    -DBUILD_OSSIMGUI=OFF \
    -DBUILD_OSSIM_MPI_SUPPORT=OFF \
    -DBUILD_OSSIMPLANET=OFF \
    -DBUILD_OSSIMPLANETQT=OFF \
    -DBUILD_OSSIMPREDATOR=OFF \
    -DBUILD_RUNTIME_DIR=bin \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_WMS=ON \
    -DCMAKE_INCLUDE_PATH=${OSSIM_DEV_HOME}/ossim/include \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_LIBRARY_PATH=${OSSIM_BUILD_DIR}/lib \
    -DCMAKE_MODULE_PATH=${OSSIM_DEV_HOME}/ossim_package_support/cmake/CMakeModules \
    -DOSSIM_COMPILE_WITH_FULL_WARNING=ON \
    -DOSSIM_DEV_HOME=${OSSIM_DEV_HOME} \
    -DPDAL_PGPOINTCLOUD_LIBRARY=OFF \
    -DPDAL_WRITER_LIBRARY=OFF \
    -DPDAL_PCDWRITER_LIBRARY=OFF \
    -DPDAL_SQLITE_READER_LIBRARY=OFF \
    ${OSSIM_DEV_HOME}
make -j 4 install

#
# cleanup
#
rm -rf \
    /tmp/builds \
    /tmp/sources
