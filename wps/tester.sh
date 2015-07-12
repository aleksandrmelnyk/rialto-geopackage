#!/bin/sh

# assumes the ossim-* tools are in GeoServer's path
# assumes geoserver is up and running at $GSHOST

GSHOST=http://localhost:8080/geoserver/ows
WPSCMD="service=wps&version=1.0.0&request=Execute"
WPS_SCRIPT_DIR=/Applications/GeoServer.app/Contents/Java/data_dir/scripts/wps
OSSIM_BIN_DIR=/tmp

RETSTATUS="<wps:Output><ows:Identifier>_status</ows:Identifier><ows:Title># datatype: int</ows:Title><wps:Data><wps:LiteralData>0</wps:LiteralData></wps:Data></wps:Output>"

# panic on the least little error
set -e

function tester {
    name=$1
    cmd=$2
    expected=$3

    echo "Test: $name"
    
    out=`$cmd`
    #echo $out
    if [[ "$out" == *"$expected"* ]]
    then
        echo "  pass"
    else
        echo "  FAIL: $out"
        echo "  CMD: $cmd"
        echo "  OUTPUT: $out"
        exit 1
    fi    

  if [[ "$out" == *"$RETSTATUS"* ]]
  then
      echo "  pass"
  else
      echo "  FAIL: nonzero status"
      #echo "  CMD: $cmd"
      echo "  OUTPUT: $out"
      exit 1
  fi    
}

rm -fr ./tmp/
mkdir -p tmp

# verify some example JSON files: just make sure the generator script doesn't crash

for i in tests/example??.json tests/ossim-test??.json
do
  ./json2wpsscript.py $i tmp/`basename $i .json`.py
done

cp -f tests/ossim-test01.sh $OSSIM_BIN_DIR/
cp -f tmp/ossim-test01.py $WPS_SCRIPT_DIR
tester "ossim01" "curl -s -S --url $GSHOST?$WPSCMD&Identifier=py:ossim-test01&DataInputs=inputFile=in;outputFile=out;setting=9" "xyzzy"
#rm -f $OSSIM_BIN_DIR/ossim-test01.sh
#rm -f $WPS_SCRIPT_DIR/ossim-test01.py

cp -f tests/ossim-test02.sh $OSSIM_BIN_DIR/
cp -f tmp/ossim-test02.py $WPS_SCRIPT_DIR
tester "ossim02" "curl -s -S --url $GSHOST?$WPSCMD&Identifier=py:ossim-test02&DataInputs=myint=12;mydouble=34.0;mystring=Yow" "xyzzy"
#rm -f $OSSIM_BIN_DIR/ossim-test02.sh
#rm -f $WPS_SCRIPT_DIR/ossim-test02.py

cp -f tests/ossim-test03.sh $OSSIM_BIN_DIR/
cp -f tmp/ossim-test03.py $WPS_SCRIPT_DIR
ARGPOS="%2212.34,56.78%22"
ARGBOX="%221,2,3,4.5%22"
tester "ossim03" "curl -s -S --url $GSHOST?$WPSCMD&Identifier=py:ossim-test03&DataInputs=mypos=$ARGPOS;mybox=$ARGBOX;mycolor=blue" "xyzzy"
#rm -f $OSSIM_BIN_DIR/ossim-test03.sh
#rm -f $WPS_SCRIPT_DIR/ossim-test03.py
