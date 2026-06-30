#/bin/bash
# # RUN THIS AFTER setup.sh so data can just be copied
# mkdir -p ./data_old/
# cp -rv ./data/generated ./data_old/
# # make directory so it can mount if it doesn't exist yet
# # - share between both containers
# mkdir -p ./firestarr/.vscode
# # undo max days from setup.sh
# git restore src/py/firestarr/common.py
# # turn ON data back on
# git restore -s ebd6cb405 src/py/firestarr/datasources/public/agency_on.py
# # go to version that doesn't use --tz
# git restore -s ebd6cb405 src/py/firestarr/sim_wrapper.py
# # include private datasources
# git clone git@github.com:jordan-evens/firestarr_datasources_private.git src/py/firestarr/datasources/private
docker compose down firestarr-app-hotfix
docker compose build --no-cache firestarr-app-hotfix
docker compose up -d firestarr-app-hotfix
# docker compose run -it --entrypoint /bin/bash firestarr-app-hotfix -c 'cd /appl/firestarr/; scripts/force_run.sh --no-publish --no-retry --prepare-only'
# # actually run
# # docker compose run -it --entrypoint /bin/bash firestarr-app-hotfix -c 'cd /appl/firestarr/; scripts/force_run.sh --resume --no-publish --no-retry'
# # revert so other container doesn't see the code we used for this one
# git restore src
