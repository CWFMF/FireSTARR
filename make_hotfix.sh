#/bin/bash
# undo max days from setup.sh
git restore src/py/firestarr/common.py
# turn ON data back on
git restore -s ebd6cb405 src/py/firestarr/datasources/public/agency_on.py
# go to version that doesn't use --tz
git restore -s ebd6cb405 src/py/firestarr/sim_wrapper.py
docker compose down firestarr-app-hotfix
docker compose build --no-cache firestarr-app-hotfix
# docker compose up -d firestarr-app-hotfix
docker compose run -it --entrypoint /bin/bash firestarr-app-hotfix -c 'cd /appl/firestarr/; scripts/force_run.sh --no-publish --no-retry --prepare-only'
# actually run
docker compose run -it --entrypoint /bin/bash firestarr-app-hotfix -c 'cd /appl/firestarr/; scripts/force_run.sh --resume --no-publish --no-retry'
