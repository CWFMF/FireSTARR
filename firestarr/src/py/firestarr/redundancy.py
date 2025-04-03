import inspect
import traceback
from io import BytesIO

import dill._dill
from log import logging

# from multiprocess.reduction import ForkingPickler

NUM_RETRIES = 5


def get_stack(ex):
    return "".join(traceback.format_exception(ex))


def should_ignore(ex):
    str_stack = get_stack(ex).lower()
    # HACK: can't reset logs so ignore them
    if ".log" in str_stack:
        return False
    if isinstance(ex, OSError) and 5 == ex.errno:
        return True
    ignore = [
        "input/output error",
        "i/o error",
        "resource temporarily unavailable",
        "blockingioerror",
        "fread",
        # seems weird but fails when reading valid tiffs with this error sometimes
        "tiffreaddirectory",
        # also TIFFReadEncodedTile
        "tiffread",
    ]
    # no point in looping if we already know the answer
    for s in ignore:
        if s in str_stack:
            return True
    return False


def try_call_safe(quiet, fct, *args, **kwargs):
    retries = NUM_RETRIES
    while True:
        try:
            return fct(*args, **kwargs)
        except Exception as ex:
            # ignore because azure is throwing them all the time
            # OSError: [Errno 5] Input/output
            if retries <= 0 or not should_ignore(ex):
                if not quiet:
                    print(get_stack(ex))
                raise ex
            retries -= 1


def call_safe(fct, *args, **kwargs):
    return try_call_safe(False, fct, *args, **kwargs)


# def load_safe(*args, **kwargs):
#     return call_safe(dill._dill.Unpickler.load, *args, **kwargs)

if not hasattr(dill._dill.Unpickler, "old_init"):
    dill._dill.Unpickler.old_init = dill._dill.Unpickler.__init__


def call_copied(f):
    # HACK: copy first time function is called
    def do_call(self, *args, **kwargs):
        if self._copy is None:
            self._copy = BytesIO(self._orig.read())
        return getattr(self._copy, f)(*args, **kwargs)

    return do_call


class BytesForwarder:
    def __init__(self, orig):
        super().__init__()
        self._orig = orig
        self._copy = None
        for f in dir(self._orig):
            fct = getattr(self._orig, f)
            if inspect.isfunction(fct):
                setattr(self, f, call_copied(f))


def has_seek(obj):
    if hasattr(obj, "seek"):
        fct = getattr(obj, "seek")
        if callable(fct):
            return True
    return False


def safe_init(self, *args, **kwds):
    # # HACK: this is horrible because it copies everything that happens
    # #       but read the bytes into memory so we can reset it if needed
    # for i in range(len(args)):
    #     arg = args[i]
    #     if isinstance(arg, BytesIO):
    #         args[i] = BytesIO(arg)
    dill._dill.Unpickler.old_init(self, *args, **kwds)
    # remove unused argument as per old __init__
    kwds.pop("ignore", None)
    # make a copy of ByteIO objects
    self._init_args = args
    self._init_kwds = kwds
    fixed_args = []
    for i in range(len(self._init_args)):
        arg = self._init_args[i]
        if isinstance(arg, BytesIO) and not has_seek(arg):
            fixed_args.append(BytesForwarder(arg))
        else:
            fixed_args.append(arg)
    self._init_args = tuple(fixed_args)
    for k, arg in self._init_kwds.items():
        if isinstance(arg, BytesIO) and not has_seek(arg):
            self._init_args[k] = BytesForwarder(arg)


# HACK: tweak code to handle OSError
def load_safe(self):  # NOTE: if settings change, need to update attributes
    retries = NUM_RETRIES
    obj = None
    ex_orig = None
    while True:
        ex_current = None
        try:
            obj = dill._dill.StockUnpickler.load(self)
            break
        except Exception as ex:
            ex_current = ex
            if ex_orig is None:
                ex_orig = ex
            if retries <= 0 or not should_ignore(ex):
                raise ex_orig
        logging.debug(f"Reinitializing after:\n\t{ex_current}\n\t{self._init_args}\n\t{self._init_kwds}")
        # need to reinitialize
        args = list(self._init_args) + list(self._init_kwds.values())
        for arg in args:
            if has_seek(arg):
                arg.seek(0)
        dill._dill.StockUnpickler.__init__(self, *self._init_args, **self._init_kwds)
        retries -= 1
    if type(obj).__module__ == getattr(self._main, "__name__", "__main__"):
        if not self._ignore:
            # point obj class to main
            try:
                obj.__class__ = getattr(self._main, type(obj).__name__)
            except (AttributeError, TypeError):
                pass  # defined in a file
    # _main_module.__dict__.update(obj.__dict__) #XXX: should update globals ?
    return obj


# def save_safe(*args, **kwargs):
#     return call_safe(dill._dill.Pickler.save, *args, **kwargs)

dill._dill.Unpickler.__init__ = safe_init
dill._dill.Unpickler.load = load_safe
# dill._dill.Pickler.save = save_safe
