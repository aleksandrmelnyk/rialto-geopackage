#!/bin/sh

# run from root dir of rialto-geopackage repo

tooldir=~/work/dev/install/rialto/bin
tooldir=~/work/dev/rialto-geopackage/mybuild/tool
info=$tooldir/rialto_info
xlat=$tooldir/rialto_translate

datadir=~/work/dev/rialto-data/demo

tmpdir=/tmp/rialtotest
mkdir -p $tmpdir

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

tester "info on demo las (point count)" \
    "$info $datadir/serp-small.las" \
    "Point count: 326511"

tester "info on demo las (point length)"\
    "$info $datadir/serp-small.las" \
    "Point length (bytes): 34"

tester "translate demo las to gpkg" \
    "$xlat -i $datadir/serp-small.las -o $tmpdir/a.gpkg" \
    "Points processed: 326511"

tester "verify generated geopackage" \
    "$info $tmpdir/a.gpkg" \
    "Num dimensions: 16"

tester "translate generated geopackage to las" \
    "$xlat -i $tmpdir/a.gpkg -o $tmpdir/a.las --maxlevel 0" \
    "Points processed: 326511"

tester "verify generated las (point count)" \
    "$info $tmpdir/a.las" \
    "Point count: 326511"

tester "verify generated las (point length)"\
    "$info $tmpdir/a.las" \
    "Point length (bytes): 34"

tester "check if translate --maxlevel works for las to gpkg" \
    "$xlat -i $datadir/serp-small.las -o $tmpdir/a.gpkg --maxlevel 7" \
    "Points processed: 326511"

tester "verify generated geopackage for --maxlevel" \
    "$info $tmpdir/a.gpkg" \
    "Max level: 7"

tester "check if translate --verify works for las to gpkg" \
    "$xlat -i $datadir/serp-small.las -o $tmpdir/a.gpkg --verify" \
    "Points processed: 326511"
