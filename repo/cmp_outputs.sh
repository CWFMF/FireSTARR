#!/bin/bash
RESULTS=$(ls -1 *.zip | sed "s/\.zip$//g")
for d in $RESULTS
do
  unzip -oq "${d}.zip" -d "${d}"
done

find -type f -name firestarr.log -print -delete
find -type f -name logfile.txt -print -delete
find -type f -name settings.ini -print -delete
rm -rf */bad_projection
rm -rf */America
rm -rf */UTC

RESULTS=$(ls -1 *.zip | sed "s/\.zip$//g")
for a in $RESULTS
do
  for b in $RESULTS
  do
    diff -rqw "${a}" "${b}" > "${a}_vs_${b}.diff" 2>&1
  done
done
