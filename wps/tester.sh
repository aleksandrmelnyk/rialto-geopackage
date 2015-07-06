#!/bin/sh

# assumes the ossim-* tools are in GeoServer's path
# assumes geoserver is up and running at $GSHOST

GSHOST=http://localhost:8080/geoserver/ows
WPSCMD="service=wps&version=1.0.0&request=Execute"

# panic on the least little error
set -e

function tester {
    name=$1
    cmd=$2
    expected=$3

    echo "Test: $name"

    out=`$cmd`
    if [[ "$out" == *"$expected"* ]]
    then
        echo "  pass"
    else
        echo "  FAIL"
        echo "  CMD: $cmd"
        echo "  OUTPUT: $out"
        exit 1
    fi    
}

rm -fr ./tmp/
mkdir -p tmp

# verify some example JSON files: just make sure the generator script doesn't crash
./json2wpsscript.py tests/example01.json tmp/example01.py
./json2wpsscript.py tests/example02.json tmp/example02.py
./json2wpsscript.py tests/example03.json tmp/example03.py
./json2wpsscript.py tests/example04.json tmp/example04.py
./json2wpsscript.py tests/example05.json tmp/example05.py

./json2wpsscript.py tests/ossim-test01.json tmp/ossim-test01.py

cp -f tests/ossim-test01.sh /usr/local/bin/
cp -f tmp/ossim-test01.py /Applications/GeoServer.app/Contents/Java/data_dir/scripts/wps/
tester "T0" "curl -s -S --url $GSHOST?$WPSCMD&Identifier=py:ossim-test01&DataInputs=inputFile=in;outputFile=out;setting=9" "xyzzy"
rm -f /usr/local/bin/ossim-test01.sh
rm -f /Applications/GeoServer.app/Contents/Java/data_dir/scripts/wps/ossim-test01.py
