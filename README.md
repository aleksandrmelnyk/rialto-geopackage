# rialto-geopackage

This repo is a library that provides the ability to store lidar point clouds
in a GeoPackage. It includes a PDAL-based Reader and Writer.

The repo also includes a simple TMS-like tile server for use with the Rialto
viewer ([https://github.com/radiantbluetechnologies/rialto-cesium]).

Rialto is an open source project by RadiantBlue Technologies.



The GeoPackage Extension
------------------------

GeoPackage does not support point clouds, so we have made a simple (and as yet
unofficial) extension based on the tile-based raster model it does support.

The most recent description of the changes made for the extension are in these
slides: [http://www.slideshare.net/mpgx/geo-package-pointclouds].




Building and Running
--------------------

*SCons*

* to build: `$ scons [debug=1] pdal_prefix=FOO install_prefix=BAR`
* to install:  `$ scons [debug=1] pdal_prefix=FOO install_prefix=BAR install`
* to clean build:  `$ scons [debug=1] pdal_prefix=FOO install_prefix=BAR -c`
* to clean install:  `$ scons [debug=1] pdal_prefix=FOO install_prefix=BAR -c install`
* to test: `$ BAR/test/rialto_test`

*Make*

_IMPORTANT: For anything later than the 0.1 branch, the preferred build system
is to use `SCons`, not `make`._

  $ make all
  $ export DYLD_LIBRARY_PATH=...as needed...
  $ make test

The top-level Makefile will:
  * build the librialto.so library
  * build the unit tests for the library
  * build a simple command-line tool

You will need to edit `defs.mk` to point to your preferred install location,
then just do `make all ; make install`.

*PDAL*

Rialto requires the PDAL libraries ([https://github.com/PDAL/PDAL]). We normally
build against a fork of PDAL at https://github.com/radiantbluetechnologies/PDAL,
but the main repo should work fine as well (as of late June 2015).

*Docker*

Docker Hub is used to build a new container for Rialto on every push:

  https://registry.hub.docker.com/u/radiantbluetechnologies/rialto-geopackage-tools/

*Running*
  
The command line app `rialto_translate`, can be used to create a geopackage file:

    $ rialto_translate -i input.las -o output.gpkg
    
The app `rialto_info` will print information about a geopackage file:
    
    $ rialto_info foo.gpkg
    
Use `-h` for additional options to these tools.


The Library
-----------

The public headers are in include/rialto.

No documentation exists at this point. However, see the unit tests or the
command line app for example usage.



The Tile Server
---------------

A Python script, `geopackage_server.py`, is provided to serve up tiles from
a GeoPackage:

    $ geopackage_server.py localhost 12346 somedir
    
 where `localhost` is the local host, `12346` is the port to use, and
 `somedir` is the directory containing the data you wish to serve.
 
 The server responds to these GET requests:
 
    GET /              # returns a JSON list of the geopackages in the server's directory (e.g. `somedir`)
    GET /file          # returns a JSON list of the point cloud tables in the file named `file.gpkg`.
    GET /file/table    # returns a JSON object with the header info for the point cloud dataset named `table` in file `file.gpkg`
    GET /file/table/$L/$X/$Y     # returns a blob with the tile at (level $L, column $X, row $Y), just like the TMS protocol

The tile blob is formatted as follows:
    * bytes 0..3: a uint32 with the number of points in this tile
    * bytes 4..7: a uint32 with a mask of which child tiles are available
    * bytes 8..n: the actual point data

The point data is laid out in the simplest fashion: a packed array of bytesm
`x0, y0, z0, t0, x1, y1, z1, t1, x2, ...`, for however many dimensions are
provided (e.g. `x,y,z,t`) and in the native datatype of each dimension. The
dimensions are described in the JSON header info for the dataset.

The child mask follows the format used by the Cesium terrain server:
[https://cesiumjs.org/data-and-assets/terrain/formats/heightmap-1.0.html].

With the exception of the prepended 8 bytes for the point count and the child
mask, the blob format is exactly the same as how the tile is stored.

*NOTE*: the tile server requires (and assumes) that the dataset being served is
in 4326 and has a root tile matrix of two columns by one row (again following
the model of the Cesium terrain server).



Getting Help
------------

Confused, baffled, or puzzled? Feel free to submit an Issue at
[https://github.com/radiantbluetechnologies/rialto-geopackage/issues] or
just email me at mpg@flaxen.com.

-mpg
