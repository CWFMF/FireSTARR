import datetime
import json
import os
import sys
import time

import numpy as np
from common import (
    CONFIG,
    DEFAULT_FILE_LOG_LEVEL,
    DIR_LOG,
    DIR_OUTPUT,
    FILE_APP_BINARY,
    FILE_APP_SETTINGS,
    SECONDS_PER_MINUTE,
    WX_MODEL,
    check_arg,
    logging,
)
from datasources.cwfif import get_model_dir_uncached, set_model_dir
from log import add_log_file
from publish import PublishError
from redundancy import get_stack
from run import Run, make_resume

# NOTE: rotating log file doesn't help because this isn't continuously running
LOG_MAIN = add_log_file(
    os.path.join(DIR_LOG, f"firestarr_{datetime.date.today().strftime('%Y%m%d')}.log"),
    level=DEFAULT_FILE_LOG_LEVEL,
)
logging.info(f"Starting FireSTARR version {os.environ.get('VERSION', 'UNKNOWN')}")

sys.path.append(os.path.dirname(sys.executable))
sys.path.append("/usr/local/bin")

no_wait = None
run_current = None
run_attempts = 0
no_retry = False
do_retry = True
is_current = None
is_published = None
needs_publish = None
should_resume = None


def run_main(args):
    global no_wait
    global run_current
    global run_attempts
    global no_retry
    global do_retry
    global is_published
    global needs_publish
    global is_current
    global should_resume

    # HACK: just get some kind of parsing for right now
    do_resume, args = check_arg("--resume", args)
    no_resume, args = check_arg("--no-resume", args)
    if do_resume and no_resume:
        raise RuntimeError("Can't specify --no-resume and --resume")
    # don't use resume arg if running again
    do_resume = do_resume and run_current is None
    no_publish, args = check_arg("--no-publish", args)
    no_merge, args = check_arg("--no-merge", args)
    no_wait, args = check_arg("--no-wait", args)
    no_retry, args = check_arg("--no-retry", args)
    prepare_only, args = check_arg("--prepare-only", args)
    do_retry = False if no_retry else True
    do_publish = False if no_publish else None
    do_merge = False if no_merge else None
    do_wait = not no_wait

    def check_resume():
        global ran_outdated
        dir_model = get_model_dir_uncached(WX_MODEL)
        modelrun = os.path.basename(dir_model)
        # HACK: just trying to check if run used this weather
        prev = make_resume(do_publish=False, do_merge=False)
        if prev is None:
            return False
        wx_updated = prev._modelrun != modelrun
        logging.info(f"Current weather is {modelrun} vs old run {prev._modelrun}")
        if not wx_updated and not prev._published_clean:
            logging.info("Found previous run and trying to resume")
            ran_outdated = True
            # previous run is for same time, but didn't work
            return True
        if wx_updated:
            # HACK: need to set this so cache isn't used
            set_model_dir(dir_model)
            logging.info(f"Have new weather for {dir_model}")
        # have new weather so don't resume
        return False

    should_resume = check_resume()
    logging.info(f"Based on weather and previous run, should_resume == {should_resume}")
    # assume resuming if not waiting
    if no_resume and should_resume:
        logging.warning("Should resume but was told not to, so making new run")
    do_resume = (not no_resume) and (do_resume or should_resume)
    if do_resume:
        if 1 < len(args):
            logging.fatal(f"Too many arguments:\n\t {sys.argv}")
        dir_resume = args[0] if args else None
        run_current = make_resume(
            dir_resume,
            do_publish=do_publish,
            do_merge=do_merge,
            prepare_only=prepare_only,
            no_wait=no_wait,
        )
        logging.info(f"Resuming previous run in {run_current._dir_runs}")
    else:
        max_days = int(args[1]) if len(args) > 1 else None
        dir_arg = args[0] if len(args) > 0 else None
        if dir_arg and not os.path.isdir(dir_arg):
            logging.fatal(f"Expected directory but got {dir_arg}")
            sys.exit(-1)
        if dir_arg and DIR_OUTPUT in os.path.abspath(dir_arg):
            if max_days:
                logging.fatal("Cannot specify number of days if resuming")
                sys.exit(-1)
            # if we give it a simulation directory then resume those sims
            run_current = Run(
                dir=dir_arg,
                do_publish=do_publish,
                prepare_only=prepare_only,
                no_wait=no_wait,
            )
            logging.info(f"Resuming simulations in {dir_arg}")
            run_current.check_and_publish()
        else:
            logging.info("Starting new run")
            run_current = Run(
                dir_fires=dir_arg,
                max_days=max_days,
                do_publish=do_publish,
                do_merge=do_merge,
                prepare_only=prepare_only,
                no_wait=no_wait,
            )
    run_attempts += 1
    # returns true if just finished current run
    is_current, df_final = run_current.run_until_successful_or_outdated(no_retry=no_retry)
    is_outdated = not is_current
    if prepare_only:
        do_retry = False
        return True, df_final
    is_published = run_current._published_clean
    needs_publish = run_current.check_do_publish() and not is_published
    should_rerun = (not no_resume) and (is_outdated or needs_publish)
    logging.info(
        f"Run {run_current._name}:\n\t"
        f"is_outdated = {is_outdated}, is_published = {is_published}, should_rerun = {should_rerun}, no_retry == {no_retry}"
    )
    # whether things should stop running
    return no_retry or (not should_rerun), df_final


def scan_queue():
    from azure.storage.queue import QueueClient, QueueServiceClient

    AZURE_QUEUE_CONNECTION = CONFIG.get("AZURE_QUEUE_CONNECTION")
    AZURE_QUEUE_NAME = CONFIG.get("AZURE_QUEUE_NAME")
    queue_service_client = QueueServiceClient.from_connection_string(AZURE_QUEUE_CONNECTION)
    queue_client = queue_service_client.get_queue_client(AZURE_QUEUE_NAME)
    response = queue_client.receive_messages(max_messages=1, visibility_timeout=60)
    for msg in response:
        try:
            txt = msg.content
            queue_client.delete_message(msg, msg.pop_receipt)
            # FIX: right now any message causes a full run
            logging.info(f"Message {txt}")
            queue_msg = json.loads(txt)
            return queue_msg
        except Exception as ex:
            logging.error(f"Unable to parse queue message:\n{msg.content}")
            logging.fatal(ex)
    logging.info("No message in queue, or failed to parse one")


def requeue():
    from azure.storage.queue import QueueClient, QueueServiceClient

    AZURE_QUEUE_CONNECTION = CONFIG.get("AZURE_QUEUE_CONNECTION")
    AZURE_QUEUE_NAME = CONFIG.get("AZURE_QUEUE_NAME")
    queue_service_client = QueueServiceClient.from_connection_string(AZURE_QUEUE_CONNECTION)
    queue_client = queue_service_client.get_queue_client(AZURE_QUEUE_NAME)
    # FIX: figure out a useful/standardized message format
    queue_client.send_message('{"msg": "Recheck outputs"}')
    response = queue_client.receive_messages(max_messages=1, visibility_timeout=60)
    logging.info("Done requeue")


FROM_QUEUE = False

if __name__ == "__main__":
    if not os.path.exists(FILE_APP_BINARY):
        raise RuntimeError(f"Unable to locate simulation model binary file {FILE_APP_BINARY}")
    if not os.path.exists(FILE_APP_SETTINGS):
        raise RuntimeError(f"Unable to locate simulation model settings file {FILE_APP_SETTINGS}")
    logging.info("Called with args %s", str(sys.argv))
    FROM_QUEUE = "--queue" in sys.argv or 1 == len(sys.argv)
    if FROM_QUEUE:
        # allow other arguments but remove duplicates
        QUEUE_ARGS = ["--no-publish", "--no-merge", "--no-retry", "--no-wait"]
        REMOVE_ARGS = QUEUE_ARGS + ["--queue"]
        for a in REMOVE_ARGS:
            try:
                sys.argv.remove(a)
            except ValueError:
                pass
        msg = scan_queue()
        logging.info(f"Queue triggered with message:\n{msg}")
        sys.argv.extend(QUEUE_ARGS)
    args_orig = sys.argv[1:]
    # rely on argument parsing later
    while do_retry:
        # HACK: just do forever for now since running manually
        logging.info("Attempting update")
        args = args_orig[:]
        try:
            # returns true if just finished current run
            is_current, df_final = run_main(args)
            if is_current:
                do_retry = False
                break
            logging.info("Trying again because used old weather")
        except KeyboardInterrupt as ex:
            raise ex
        except Exception as ex:
            logging.error(ex)
            logging.error(get_stack(ex))
            if no_retry:
                logging.error("Stopping because of error")
                sys.exit(-1)
            logging.info("Trying again because of error")
    if FROM_QUEUE:
        # publish_all(
        #     run_current._dir_output,
        #     changed_only=False,
        #     force=True,
        #     merge_only=False,
        # )
        run_current = make_resume(do_publish=True, do_merge=True, no_wait=True)
        try:
            run_current.check_and_publish(force=True)
        except PublishError as ex:
            if should_resume:
                # there shouldn't be an error if we were resuming
                logging.error(ex)
    if run_current.ran_all():
        logging.info("Finished all simulations successfully")
    else:
        logging.info("Done but not all simulations have run")
        if FROM_QUEUE:
            logging.info("Requeuing")
            requeue()
