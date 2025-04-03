from azure.storage.blob import BlobServiceClient
from azurebatch import _STORAGE_ACCOUNT_URL, _STORAGE_CONTAINER, _STORAGE_KEY


def undelete_all(run_id):
    # was used when accidentally deleted a bunch of stuff
    # not required normally
    blob_service_client = BlobServiceClient(account_url=_STORAGE_ACCOUNT_URL, credential=_STORAGE_KEY)
    container = blob_service_client.get_container_client(_STORAGE_CONTAINER)
    blob_list = [x for x in container.list_blobs(name_starts_with=f"sims/{run_id}", include="deleted") if x.deleted]
    blob_list = [x for x in blob_list if ".lock" not in x.name]

    def undelete(ext):
        undelete_list = [x for x in blob_list if ext in x.name]
        for x in undelete_list:
            blob_client = blob_service_client.get_blob_client(container=_STORAGE_CONTAINER, blob=x.name)
            blob_client.undelete_blob()

    for ext in [".csv", ".tif", ".geojson", ".log"]:
        undelete(ext)
