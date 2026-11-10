#!/bin/bash
echo ${GHCR_TOKEN}  | regctl registry login ghcr.io -u jordan-evens --pass-stdin
for sha in $(curl -s 'https://github.com/CWFMF/FireSTARR/pkgs/container/firestarr%2Ffirestarr/versions?filters%5Bversion_type%5D=untagged' | grep sha256 | sed 's/.*sha256:\([^\<]*\).*/\1/g')
do
  echo $sha
done
