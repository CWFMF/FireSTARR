from common import DIR_SCRIPTS, logging, run_process


# HACK: just call script for now
def publish_folder(dir_output):
    stdout, stderr = run_process(["/bin/bash", "-c", "'./publish_geoserver.sh'"], DIR_SCRIPTS)
    logging.info(stdout)
    logging.error(stderr)
    return True
