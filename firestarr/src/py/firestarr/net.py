import datetime
import os
import ssl
import time
import urllib.parse
from functools import cache
from io import StringIO
from urllib.error import HTTPError

import dateutil
import dateutil.parser
import requests
import tqdm_util
from common import (
    FLAG_DEBUG,
    always_false,
    do_nothing,
    ensure_dir,
    ensures,
    fix_timezone_offset,
    locks_for,
    logging,
)
from redundancy import call_safe
from urllib3.exceptions import InsecureRequestWarning

# So HTTPS transfers work properly
ssl._create_default_https_context = ssl._create_unverified_context

# Suppress only the single warning from urllib3 needed.
requests.packages.urllib3.disable_warnings(category=InsecureRequestWarning)

VERIFY = False
# VERIFY = True

# pretend to be something else so servers don't block requests
HEADERS = {
    "User-Agent": " ".join(
        [
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64)",
            "AppleWebKit/537.36 (KHTML, like Gecko)",
            "Chrome/106.0.0.0 Safari/537.36 Edg/106.0.1370.34",
        ]
    ),
}
# HEADERS = {'User-Agent': 'WeatherSHIELD/0.93'}
RETRY_MAX_ATTEMPTS = 0
RETRY_DELAY = 2

# HACK: list of url parameters to not mask
#   just to make sure we mask everything unless we know it's safe in logs
SAFE_PARAMS = [
    "model",
    "lat",
    "lon",
    "ens_val",
    "where",
    "outFields",
    "f",
    "outStatistics",
]
MASK_PARAM = "#######"
WAS_MASKED = set()

CACHE_DOWNLOADED = {}
CACHE_LOCK_FILE = "firestarr_cache"


def _save_http_uncached(
    url,
    save_as,
    fct_is_invalid=always_false,
    **kwargs,
):
    modlocal = None
    logging.debug("Opening %s", url)
    response = requests.get(
        url,
        stream=True,
        verify=VERIFY,
        headers=HEADERS,
        **kwargs,
    )
    if 200 != response.status_code or fct_is_invalid(response):
        url_masked = mask_url(url)
        error = f"Error saving {save_as} from {url_masked}"
        logging.error(error)
        raise HTTPError(
            url_masked,
            response.status_code,
            error,
            response.headers,
            StringIO(response.text),
        )
    if os.path.isfile(save_as) and "last-modified" in response.headers.keys():
        mod = response.headers["last-modified"]
        modtime = dateutil.parser.parse(mod)
        modlocal = fix_timezone_offset(modtime)
        filetime = os.path.getmtime(save_as)
        filedatetime = datetime.datetime.fromtimestamp(filetime)
        if modlocal == filedatetime:
            return save_as
    ensure_dir(os.path.dirname(save_as))
    save_as = tqdm_util.wrap_write(
        response.iter_content(chunk_size=4096),
        save_as,
        "wb",
        desc=url.split("?")[0] if "?" in url else url,
        total=int(response.headers.get("content-length", 0)),
    )
    if modlocal is not None:
        tt = modlocal.timetuple()
        usetime = time.mktime(tt)
        os.utime(save_as, (usetime, usetime))
    return save_as


@cache
def _save_http_cached(
    url,
    save_as,
    fct_is_invalid=always_false,
    **kwargs,
):
    return call_safe(_save_http_uncached, url, save_as, fct_is_invalid, **kwargs)


def check_downloaded(path):
    # logging.debug("check_downloaded(%s) - waiting", path)
    with locks_for(CACHE_LOCK_FILE):
        # FIX: should return False if file no longer exists
        # logging.debug("check_downloaded(%s) - checking", path)
        result = CACHE_DOWNLOADED.get(path, None)
        # logging.debug("check_downloaded(%s) - returning %s", path, result)
        return result


def mark_downloaded(path, flag=True):
    # logging.debug("mark_downloaded(%s, %s)", path, flag)
    if not (flag and path in CACHE_DOWNLOADED):
        # logging.debug("mark_downloaded(%s, %s) - waiting", path, flag)
        with locks_for(CACHE_LOCK_FILE):
            # logging.debug("mark_downloaded(%s, %s) - marking", path, flag)
            if flag:
                # logging.debug("mark_downloaded(%s, %s) - adding", path, flag)
                CACHE_DOWNLOADED[path] = path
            elif path in CACHE_DOWNLOADED:
                # logging.debug("mark_downloaded(%s, %s) - removing", path, flag)
                del CACHE_DOWNLOADED[path]
    # else:
    #     logging.debug("mark_downloaded(%s, %s) - do nothing", path, flag)
    # logging.debug("mark_downloaded(%s, %s) - returning %s", path, flag, path)
    return path


def save_http(
    url,
    save_as,
    keep_existing,
    fct_pre_save,
    fct_post_save,
    fct_is_invalid=always_false,
    **kwargs,
):
    logging.debug("save_http(%s, %s)", url, save_as)

    @ensures(
        paths=save_as,
        remove_on_exception=True,
        replace=not keep_existing,
        msg_error=f"Failed getting {url}",
    )
    def do_save(_):
        # if another thread downloaded then don't do it again
        # @ensures already checked if file exists but we want to replace
        # logging.debug("do_save(%s)", _)
        r = check_downloaded(_)
        if r:
            # logging.debug("%s was downloaded already", _)
            return r
        # HACK: put in one last lock so it doesn't download twice
        with locks_for(_ + ".tmp"):
            # HACK: one last check for file
            if not (keep_existing and os.path.isfile(_)):
                r = _save_http_cached((fct_pre_save or do_nothing)(url), _, fct_is_invalid, **kwargs)
        # logging.debug("do_save(%s) - returning %s", _, r)
        # mark_downloaded(_)
        return _

    try:
        # if already downloaded then use existing file
        # if not downloaded then try to save but check cache before downloading
        # either way, call fct_post_save on the file
        file_existed = os.path.isfile(save_as)
        r = check_downloaded(save_as)
        if not r:
            # logging.debug("save_http(%s, %s) - calling do_save(%s)", url, save_as, save_as)
            r = do_save(save_as)
            # might have already existed, so marking in do_save() might not happen
            r = mark_downloaded(r)
        # else:
        #     logging.debug("save_http(%s, %s) - %s was downloaded", url, save_as, save_as)
        try:
            # HACK: if parsing fails then delete file
            r = (fct_post_save or do_nothing)(r)
        except KeyboardInterrupt as ex:
            raise ex
        except Exception as ex:
            logging.debug(ex)
            # @ensures should have taken care of deleting file
            mark_downloaded(save_as, False)
            if file_existed:
                logging.error("Unable to parse existing file %s" % save_as)
                os.remove(save_as)
                return save_http(
                    url,
                    save_as,
                    keep_existing,
                    fct_pre_save,
                    fct_post_save,
                    fct_is_invalid,
                    **kwargs,
                )
            else:
                raise ex

        # logging.debug("save_http(%s, %s) - returning %s", url, save_as, r)
        if not check_downloaded(save_as):
            raise RuntimeError(f"Expected {save_as} to be marked as downloaded")
        return r
    except KeyboardInterrupt as ex:
        raise ex
    except Exception as ex:
        logging.debug(ex)
        # @ensures should have taken care of deleting file
        mark_downloaded(save_as, False)
        raise ex


def mask_url(url):
    global WAS_MASKED
    r = urllib.parse.urlparse(url)
    if not FLAG_DEBUG and r.query:
        args = urllib.parse.parse_qs(r.query)
        for k in args.keys():
            if k not in SAFE_PARAMS:
                WAS_MASKED.add(k)
                args[k] = [MASK_PARAM]
        r = r._replace(query="&".join(f"{k}={','.join(v)}" for k, v in args.items()))
    return urllib.parse.urlunparse(r)


def try_save_http(
    url,
    save_as,
    keep_existing,
    fct_pre_save,
    fct_post_save,
    max_save_retries=RETRY_MAX_ATTEMPTS,
    check_code=False,
    fct_is_invalid=always_false,
    **kwargs,
):
    save_tries = 0
    while True:
        try:
            return save_http(url, save_as, keep_existing, fct_pre_save, fct_post_save, fct_is_invalid, **kwargs)
        except KeyboardInterrupt as ex:
            raise ex
        except Exception as ex:
            logging.info("Caught %s in %s", ex, __name__)
            if isinstance(ex, KeyboardInterrupt):
                raise ex
            m = mask_url(url)
            # no point in retrying if URL doesn't exist, is forbidden, or timed out
            if check_code and isinstance(ex, HTTPError) and ex.code in [403, 404, 504]:
                # if we're checking for code then return None since file can't exist
                with locks_for(CACHE_LOCK_FILE):
                    CACHE_DOWNLOADED[save_as] = None
                return None
            if FLAG_DEBUG or save_tries >= max_save_retries:
                logging.error(
                    "Downloading %s to %s - Failed after %s attempts",
                    m,
                    save_as,
                    save_tries,
                )
                raise ex
            logging.warning(
                "Downloading %s to %s - Retrying after:\n\t%s",
                m,
                save_as,
                ex,
            )
            time.sleep(RETRY_DELAY)
            save_tries += 1
