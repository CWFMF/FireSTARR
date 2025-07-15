import os
import time

from common import (
    CREATION_OPTIONS,
    DIR_OUTPUT,
    DIR_ZIP,
    FILE_LOCK_PUBLISH,
    PUBLISH_AZURE_WAIT_TIME_SECONDS,
    list_dirs,
    locks_for,
    logging,
    zip_folder,
)
from redundancy import get_stack


# distinguish erros with publishing from other problems
class PublishError(RuntimeError):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)


def publish_all(
    dir_output=None,
    changed_only=False,
    force=False,
    force_project=False,
    force_publish=False,
    merge_only=False,
):
    dir_output = find_latest_outputs(dir_output)
    # check_copy_interim(dir_output, include_interim)
    with locks_for(FILE_LOCK_PUBLISH):
        changed = merge_dirs(
            dir_output,
            changed_only=changed_only,
            force=force,
            force_project=force_project,
            create_zip=not merge_only,
        )
        if merge_only:
            logging.info("Stopping after merge for %s", dir_output)
            return
        # HACK: changed is checked in upload_dir so don't filter on that
        # if changed or force or force_publish:
        import publish_azure

        changed = publish_azure.upload_dir(dir_output)
        if changed:
            logging.info("Uploaded to azure")
            logging.info("Publishing to geoserver from %s", dir_output)
            # HACK: might be my imagination, but maybe there's a delay so wait a bit
            time.sleep(PUBLISH_AZURE_WAIT_TIME_SECONDS)
            import publish_geoserver

            publish_geoserver.publish_folder(dir_output)
        else:
            logging.info("No changes for %s so not publishing", os.path.basename(dir_output))


def find_latest_outputs(dir_output=None):
    if dir_output is None:
        dir_default = DIR_OUTPUT
        dirs_with_initial = [
            x for x in list_dirs(dir_default) if os.path.isdir(os.path.join(dir_default, x, "initial"))
        ]
        if dirs_with_initial:
            dir_output = os.path.join(dir_default, dirs_with_initial[-1])
            logging.info("Defaulting to directory %s", dir_output)
            return dir_output
        else:
            raise PublishError(f'find_latest_outputs("{dir_output}") failed: No run found')
    return dir_output


def merge_dirs(
    dir_input=None,
    changed_only=False,
    force=False,
    force_project=False,
    creation_options=CREATION_OPTIONS,
    create_zip=True,
):
    any_change = False
    dir_input = find_latest_outputs(dir_input)
    # expecting dir_input to be a path ending in a runid of form '%Y%m%d%H%M'
    dir_base = os.path.join(dir_input, "initial")
    if not os.path.isdir(dir_base):
        raise PublishError(f"Directory {dir_base} missing")
    run_name = os.path.basename(dir_input)
    if create_zip:
        try:
            run_id = os.path.basename(dir_input)
            file_zip = os.path.join(DIR_ZIP, f"{run_name}.zip")
            if any_change or not os.path.isfile(file_zip):
                logging.info("Creating archive %s", file_zip)
                zip_folder(file_zip, dir_base)
        except KeyboardInterrupt as ex:
            raise ex
        except Exception as ex:
            logging.error("Ignoring zip error")
            logging.error(get_stack(ex))

    return any_change
