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
check "--mypos" "$5"
check "12.34 56.78" "$6"
check "--mystring" "$7"
check "Yow" "$8"

echo Bye.
