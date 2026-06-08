#!/bin/bash
set -e
pushd firestarr
VERSION=`sed -n "/^VERSION/{s/.*=\(.*\)/\1/g;p;}" .env`
SRCS=(firestarr)
# REPOS=(ghcr.io/jordan-evens/ registrycwfisdev.azurecr.io/firestarr/)
REPOS=(ghcr.io/jordan-evens/)

echo ${GHCR_TOKEN}  | docker login ghcr.io -u jordan-evens --password-stdin
# az acr login --name registrycwfisdev || (az login && az acr login --name registrycwfisdev)
build_tag_and_push() {
    echo $1
    echo $2
    src=$1
    img="${src}:${VERSION}"
    repo=$2
    docker compose build ${src}
    set +e
    docker rmi ${repo}${img}
    docker rmi ${repo}${src}:latest
    set -e
    docker tag ${img} ${repo}${img}
    docker tag ${img} ${repo}${src}:latest
    docker push ${repo}${src}
    docker push ${repo}${img}
    docker push ${repo}${src}:latest
}

for src in ${SRCS[*]}; do
    for repo in ${REPOS[*]}; do
        build_tag_and_push ${src} ${repo}
    done
done

popd
