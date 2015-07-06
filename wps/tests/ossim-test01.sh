#!/bin/sh

echo Hi.
echo ${1}
if [ "${1}" != "--inputFile" ]
then
    echo "bad option 1"
    exit 1
fi

if [ "${3}" != "--outputFile" ] ; then
    echo "bad option 3"
    exit 1
fi

if [ "${5}" != "--setting" ] ; then
    echo "bad option 5"
    exit 1
fi

echo Input: ${2}
echo Output: ${4}
echo Setting: ${6}

url="xyzzy"
echo "[url] $url"

#cp $infile $outfile

#echo "vvvv" > $outfile
#cat $infile >> $outfile
#echo $@ >> $outfile
#echo "^^^^" >> $outfile

echo Bye.
