import collections
import contextlib
import itertools
import math

import multiprocess
import multiprocess.pool
import multiprocess.queues
import numpy as np
import pandas as pd
from log import logging
from tqdm.auto import tqdm

MAX_ATTEMPTS = 1
MAX_PROCESSES = multiprocess.cpu_count()
TQDM_DEPTH = multiprocess.Value("i", 0)
DEFAULT_KEEP_ALL = True
KEEP_LEVELS = 2
MINITERS = 2
TqdmArgs = collections.namedtuple("TqdmArgs", ["position", "leave"])


def max_concurrent():
    # HACK: so we can lower number of concurrent processes when things fail
    n = MAX_PROCESSES
    return n


def update_max_attempts(n):
    global MAX_ATTEMPTS
    old = MAX_ATTEMPTS
    # HACK: so we can lower number of concurrent processes when things fail
    if n > old:
        MAX_ATTEMPTS = n
        logging.warning(
            f"Increasing number of attempts so far to {n} from {old} means limiting to {max_concurrent()} concurrent now"
        )


@contextlib.contextmanager
def tqdm_depth(keep_all=DEFAULT_KEEP_ALL):
    global TQDM_DEPTH
    position = TQDM_DEPTH.value
    obj = TqdmArgs(position, keep_all or KEEP_LEVELS > position)
    try:
        TQDM_DEPTH.value += 1
        yield obj
    finally:
        TQDM_DEPTH.value -= 1


# def apply_direct(fct, values, try_only=False, *args, **kwargs):
#     total = kwargs.get("total", None) or (
#         len(values) if hasattr(values, "__len__") else None
#     )
#     if 0 == total:
#         # nothing to do
#         return []
#     if 1 == total:
#         # don't loop if just one thing
#         return [fct(values[0])]
#     if try_only:
#         return None
#     return (
#         [fct(x) for x in tqdm(values, *args, **kwargs)]
#         if fct is not None
#         else tqdm(values, *args, **kwargs)
#     )


def apply(onto, fct=None, *args, **kwargs):
    with tqdm_depth() as tq:
        kwargs["position"] = tq.position
        kwargs["leave"] = tq.leave
        kwargs["miniters"] = MINITERS
        if isinstance(onto, pd.DataFrame) or isinstance(onto, pd.Series):
            if fct is None:
                raise RuntimeError("Must specify function if using Series or DataFrame")
            #  prep so progress_apply() works
            tqdm.pandas(*args, **kwargs)
            # apply to axis if not Series
            return onto.progress_apply(fct) if isinstance(onto, pd.Series) else onto.progress_apply(fct, axis=1)
        # return apply_direct(fct, onto, *args, **kwargs)
        return [fct(x) for x in tqdm(onto, *args, **kwargs)] if fct is not None else tqdm(onto, *args, **kwargs)


def wrap_write(chunks, save_as, mode, *args, **kwargs):
    with tqdm_depth() as tq:
        kwargs["position"] = tq.position
        kwargs["leave"] = tq.leave
        kwargs["miniters"] = MINITERS
        with tqdm.wrapattr(open(save_as, mode), "write", *args, **kwargs) as fout:
            for chunk in chunks:
                fout.write(chunk)
        return save_as


def initializer():
    # get an error if don't import in initalizer
    import signal

    signal.signal(signal.SIGINT, signal.SIG_IGN)


def init_pool(processes=None, no_limit=False):
    if processes is None:
        processes = max_concurrent()
    # allow overriding above number of cpus if really wanted
    if not no_limit:
        # no point in starting more than the number of cpus?
        processes = min(processes, max_concurrent())
    # ignore Ctrl+C in workers
    return multiprocess.pool.Pool(
        initializer=initializer,
        # context=SafeForkContext(),
        processes=processes,
    )


def pmap(fct, values, max_processes=None, no_limit=False, *args, **kwargs):
    # # check if there's no point in looping
    # result_direct = apply_direct(fct, values, try_only=True, *args, **kwargs)
    # if result_direct is not None:
    #     return result_direct
    is_single = (1 == max_processes) or (not no_limit and 1 == max_concurrent())
    if is_single:
        # don't bother with pool if only one process
        return [fct(x) for x in apply(values, *args, **kwargs)]
    pool = init_pool(max_processes, no_limit)
    try:
        kwargs["total"] = kwargs.get("total", len(values))
        kwargs["miniters"] = MINITERS
        results = apply(
            pool.imap_unordered(
                lambda p: (p[0], fct(p[1])),
                [(i, v) for i, v in enumerate(values)],
            ),
            *args,
            **kwargs,
        )
        result = [v[1] for v in sorted(results, key=lambda v: v[0])]
        return result
    finally:
        # avoid Exception ignored in: <function Pool.__del__ at 0x7f8fd4c75d30>
        pool.terminate()


def pmap_by_group(
    fct,
    values,
    callback_group=None,
    max_processes=None,
    no_limit=False,
    *args,
    **kwargs,
):
    if not hasattr(values, "values"):
        return pmap(fct, values, max_processes=max_processes, no_limit=no_limit, *args, **kwargs)

    pool = init_pool(max_processes, no_limit)
    _desc = f"{kwargs['desc']}: " if "desc" in kwargs else ""
    # HACK: let dictionary show progress by groups
    values = values
    all_values = list(itertools.chain.from_iterable(values.values()))
    lengths = {k: len(v) for k, v in values.items()}
    groups = list(lengths.keys())
    num = list(lengths.values())
    breaks = list(np.cumsum(list(lengths.values())))
    ranges = {groups[i]: list(zip([0] + breaks, breaks))[i] for i in range(len(breaks))}
    groups_by_index = list(itertools.chain.from_iterable([[groups[i]] * num[i] for i in range(len(groups))]))
    groups_left = {groups[i]: num[i] for i in range(len(groups))}
    groups_done = list()
    groups_pending = list(groups)
    completed = []

    try:
        kwargs["total"] = len(all_values)
        kwargs["miniters"] = MINITERS
        result = {}
        results = [None] * len(all_values)
        for i, v in (
            pbar := apply(
                pool.imap_unordered(
                    lambda p: (p[0], fct(p[1])),
                    [(i, v) for i, v in enumerate(all_values)],
                ),
                *args,
                **kwargs,
            )
        ):
            results[i] = v
            g = groups_by_index[i]
            groups_left[g] -= 1
            if 0 == groups_left[g]:
                groups_done.append(g)
                groups_pending.remove(g)
                completed.extend(values[g])
                j, k = ranges[g]
                result[g] = results[j:k]
                if callback_group:
                    callback_group(g, result[g])
            pbar.set_description(f"{_desc}{groups_done}/{groups_pending}".replace("]/[", "::"))
        return result
    finally:
        # avoid Exception ignored in: <function Pool.__del__ at 0x7f8fd4c75d30>
        pool.terminate()


def keep_trying(fct, values, return_with_status=False, *args, **kwargs):
    done = False
    successful = []
    remaining = [(i, x) for i, x in enumerate(values)]
    num_prev = None

    def fct_try(v):
        i, dir_fire = v
        try:
            return (True, i, fct(dir_fire))
        except KeyboardInterrupt as ex:
            raise ex
        except Exception:
            return (False, i, dir_fire)

    while not done:
        try:
            run_completed = pmap(
                fct_try,
                remaining,
                *args,
                **kwargs,
            )
            done = True
            good = [r[1:] for r in run_completed if r[0]]
            if good:
                successful.extend(good)
            remaining = [r[1:] for r in run_completed if not r[0]]
            num_cur = len(remaining)
            if 0 == num_cur:
                break
            if num_cur == num_prev:
                logging.error(f"Settled on having {num_cur} results not working")
                break
            num_prev = num_cur
        except BrokenPipeError:
            pass
    out = [(i, True, v) for i, v in successful] + [(i, False, v) for i, v in remaining]
    in_order = [(f, v) for i, f, v in sorted(out, key=lambda x: x[0])]
    if return_with_status:
        return in_order
    # get rid of status
    return [v if f else None for f, v in in_order]


def keep_trying_groups(fct, values, *args, **kwargs):
    done = False
    remaining = {k: v for k, v in values.items()}
    num_prev = None
    unsuccessful = {}
    successful = {}

    def fct_try(dir_fire):
        try:
            return (True, fct(dir_fire))
        except KeyboardInterrupt as ex:
            raise ex
        except Exception as ex:
            # was returning directory but that doesn't help
            # return (False, dir_fire)
            return (False, ex)

    while not done:
        try:
            run_completed = pmap_by_group(
                fct_try,
                remaining,
                *args,
                **kwargs,
            )
            unsuccessful = {}
            done = True
            num_cur = 0
            for g, v in run_completed.items():
                good = [r[1] for r in v if r[0]]
                if good:
                    successful[g] = successful.get(g, []) + good
                bad = [r[1] for r in v if not r[0]]
                if bad:
                    unsuccessful[g] = bad
                    num_cur += len(bad)
                    done = False
            remaining = unsuccessful
            if num_cur > 0 and num_cur == num_prev:
                logging.error(f"Settled on having {num_cur} results not working")
                break
            num_prev = num_cur
        except BrokenPipeError:
            pass
    return successful, unsuccessful
