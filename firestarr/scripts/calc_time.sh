#/bin/bash
FILE=$*
MATCH=".*\([0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9]\).*"
REGEX="s/${MATCH}/\1/g"
# get a few lines at either end in case first/last don't have timestamp
START=`head -n10 ${FILE} | sed -n "/${MATCH}/p;" | head -n1 | sed "${REGEX}"`
END=`tail -n10 ${FILE} | sed -n "/${MATCH}/p;" | tail -n1 | sed "${REGEX}"`
DATE_START=$(date -d "${START}" '+%s')
DATE_END=$(date -d "${END}" '+%s')
DATE_DIFF=$(( (DATE_END - DATE_START) ))
echo $DATE_DIFF
