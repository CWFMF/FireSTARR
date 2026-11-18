sudo sysctl -w kernel.perf_event_paranoid=1
sudo sysctl -w kernel.kptr_restrict=0
IS_PASTED=
if [[ "$0" =~ "/bash" ]]; then
  DIR_TEST=`realpath test`
  IS_PASTED=1
else
  set -e
  DIR_TEST="$(dirname $(realpath "$0"))"
fi
USER=$(whoami)
DIR_ROOT=$(dirname "${DIR_TEST}")
DIR_BUILD="${DIR_ROOT}/build"
DIR_PERF=${DIR_ROOT}
FILE_PERF=${DIR_PERF}/perf.data
FREQ=100
DIR_FG=/appl/FlameGraph
OPTS="-DCPU_TUNE_ONLY=0 -DCPU_TARGET=native -DCMAKE_VERBOSE_OPTIMIZATIONS=1"
[ ! -d "${DIR_PERF}" ] && mkdir -p "${DIR_PERF}"
# don't make this an actual submodule because it's just this script that needs it
[ ! -d "${DIR_FG}" ] && sudo apt install git && sudo git clone https://github.com/brendangregg/FlameGraph.git ${DIR_FG} && sudo chown -R ${USER}:${USER} ${DIR_FG}
scripts/build.sh RelWithDebInfo all ${OPTS}
cp ${DIR_BUILD}/firestarr ${DIR_BUILD}/test_* ${DIR_ROOT}/
time sudo perf record -o ${FILE_PERF} -F ${FREQ} -g --call-graph=dwarf -- $* > ${DIR_PERF}/run.log
sudo chown -R ${USER}:${USER} ${DIR_PERF}
perf script -i ${FILE_PERF} | ${DIR_FG}/stackcollapse-perf.pl | ${DIR_FG}/flamegraph.pl > flame.html
