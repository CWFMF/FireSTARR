import os

from common import DIR_SCRIPTS, logging, run_process


# HACK: just call script for now
def publish_folder(dir_output):
    run_id = os.path.basename(dir_output)
    stdout, stderr = run_process(["/bin/bash", "-c", f"'./publish_geoserver.sh {run_id}'"], DIR_SCRIPTS)
    logging.info(stdout)
    logging.error(stderr)
    return True
