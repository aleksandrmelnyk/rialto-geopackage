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

check "--mydouble" "$1"
check "34.0" "$2"
check "--myint" "$3"
check "12" "$4"
check "--mystring" "$5"
check "Yow" "$6"

echo Bye.
