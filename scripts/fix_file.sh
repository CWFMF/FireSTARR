#!/bin/bash
# used to quickly replace the contents of a file when debugging
# 1) copy contents of file
# 2) run 'scripts/fix_file.sh <FILE>'
# 3) paster contents into nano window
# 4) save and exit
# 5) should be able to reload file in python with new contents
FILE=src/py/firestarr/$*
rm ${FILE} && nano -w ${FILE}
