import logging.handlers as handlers
from logging import (
    DEBUG,
    ERROR,
    FATAL,
    INFO,
    WARN,
    WARNING,
    FileHandler,
    Formatter,
    StreamHandler,
    getLogger,
)

DEFAULT_LEVEL = INFO
# fixed-width for levelname so '-' before message always lines up
DEFAULT_FORMAT = "%(asctime)s - %(levelname)8s - [%(filename)s:%(lineno)d] - %(message)s"


# HACK: want to have a thing called "logging" that refers to our own logger
#       so that we don't need to change usage everywhere
LOGGER_NAME = "firestarr"
logging = getLogger(LOGGER_NAME)
logging.DEBUG = DEBUG
logging.INFO = INFO
logging.WARN = WARN
logging.WARNING = WARNING
logging.ERROR = ERROR
logging.FATAL = FATAL
logging.getLogger = getLogger

# don't use basicConfig because we want level to be set so we can change overall
# level later
LOG_DEFAULT = StreamHandler(None)
LOG_FORMATTER_DEFAULT = Formatter(DEFAULT_FORMAT)


def add_handler(
    handler,
    level=DEFAULT_LEVEL,
    log_format=LOG_FORMATTER_DEFAULT,
    logger=getLogger(LOGGER_NAME),
):
    handler.setFormatter(log_format)
    if level:
        handler.setLevel(level)
    logger.addHandler(handler)
    level = min([h.level for h in logger.handlers])
    logger.setLevel(level)
    if logger.name == LOGGER_NAME:
        # HACK: keep these from being higher than WARNING
        # FIX: might not matter now that we're calling getLopgger(logger_name)?
        getLogger("gdal").setLevel(max(logging.WARNING, level))
        getLogger("fiona").setLevel(max(logging.WARNING, level))
    return handler


def add_log_file(file_log, *args, **kwargs):
    return add_handler(FileHandler(file_log), *args, **kwargs)


def add_log_rotating(file_log, when="d", interval=1, *args, **kwargs):
    return add_handler(handlers.TimedRotatingFileHandler(file_log, when=when, interval=interval), *args, **kwargs)


add_handler(LOG_DEFAULT)
