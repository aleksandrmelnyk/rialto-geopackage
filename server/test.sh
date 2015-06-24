#!/bin/sh

# run from root dir of rialto-geopackage repo

letter=$1

gserver=~/work/dev/rialto-geopackage/server/geopackage_server.py
fserver=~/work/dev/rialto-geopackage/server/file_server.py
pserver=~/work/dev/rialto-geopackage/server/proxy_server.py

datadir=~/work/dev/rialto-data/demo

tmpdir=/tmp/rialtoservertest
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

#function finish {
#  kill $pid
#  echo abort
#}
#trap finish EXIT

function gpkg_test {
    $gserver localhost 12342 $datadir &
    pid=$!
    echo "G server PID: $pid"
    sleep 2

    tester "get gpkg names - 1" "curl -s -S --url http://localhost:12342/" "serp-small"
    tester "get gpkg names - 2" "curl -s -S --url http://localhost:12342/" "twotables"

    tester "get table names" "curl -s -S --url http://localhost:12342/twotables" "myfirsttable"
    tester "get table names" "curl -s -S --url http://localhost:12342/twotables" "mysecondtable"

    tester "get header - 1" "curl -s -S --url http://localhost:12342/serp-small/mytablename" "\"maximum\": -83.4243"
    tester "get header - 2" "curl -s -S --url http://localhost:12342/serp-small/mytablename" "\"num_rows_L0\": 1"

    kill $pid
}

function file_test {
    $fserver localhost 12341 $datadir &
    pid=$!
    echo "F server PID: $pid"
    sleep 2

    tester "get gpkg names - 1" "curl -s -S --url http://localhost:12341/README.txt" "serp-small dataset"

    kill $pid
}

function proxy_test {
    $pserver localhost 12343 &
    pid=$!
    echo "P server PID: $pid"
    sleep 2

    tester "get google" "curl -s -S --url http://localhost:12343/?http://www.google.com/index.html" "Search the world"

    kill $pid
}

if [[ "$letter" == "g" ]]; then
    gpkg_test
elif [[ "$letter" == "f" ]]; then
    file_test
elif [[ "$letter" == "p" ]]; then
    proxy_test
else
    echo "error: unknown server"
    exit 1
fi

echo pass
