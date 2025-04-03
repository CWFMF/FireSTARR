#!/bin/bash
# DIR=`dirname $(realpath "$0")`
# instead of looking at logs, look at process times
# 1     get all processes
# 2     match name of simulation process
# 3     don't match other grep processes
# 4     get start time column
# 5     convert start time to seconds
# 6     subtract seconds from current time
ps aux | grep /appl/firestarr/firestarr | grep -v grep | awk '{print $9}' | xargs -I {} date --date="{}" '+%s' | xargs -I time expr $(date '+%s') - time
