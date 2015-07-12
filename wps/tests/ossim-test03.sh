#!/bin/sh

function check {
    expected="$1"
    actual="$2"

    if [ "${actual}" != "${expected}" ]; then
      echo "FAIL: expected $expected, got $actual"
      exit 1
    fi
}

url="xyzzy"
echo "[url] $url"

check "--mybox" "$1"
check "1,2,3,4.5" "$2"
check "--mycolor" "$3"
check "blue" "$4"
check "--mypos" "$5"
check "12.34,56.78" "$6"

echo Bye.
