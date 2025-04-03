import sys
from common import check_arg, logging
from main import make_resume

if __name__ == "__main__":
    args_orig = sys.argv[1:]
    args = args_orig[:]
    # do_resume, args = check_arg("--resume", args)
    # no_resume, args = check_arg("--no-resume", args)
    # if do_resume and no_resume:
    #     raise RuntimeError("Can't specify --no-resume and --resume")
    # # don't use resume arg if running again
    # do_resume = do_resume and run_current is None
    no_publish, args = check_arg("--no-publish", args)
    no_merge, args = check_arg("--no-merge", args)
    do_force, args = check_arg("--force", args)
    # no_wait, args = check_arg("--no-wait", args)
    # no_retry, args = check_arg("--no-retry", args)
    # prepare_only, args = check_arg("--prepare-only", args)
    # do_retry = False if no_retry else True
    do_publish = False if no_publish else None
    do_merge = False if no_merge else None
    # do_wait = not no_wait
    # did_wait = False
    self = make_resume(do_publish=do_publish, do_merge=do_merge)
    self.check_and_publish(force=do_force)
    logging.info("Finished successfully")
