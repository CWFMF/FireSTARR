#!/bin/bash
set -e
VERSION=`sed -n "/^VERSION/{s/.*=\(.*\)/\1/g;p;}" .env`
SRCS=(firestarr-app)
# REPOS=(ghcr.io/jordan-evens/ registrycwfisdev.azurecr.io/firestarr/)
# REPOS=(ghcr.io/jordan-evens/)
REPOS=(registrycwfisdev.azurecr.io/firestarr/)
# BRANCHES=(latest dev)
# BRANCHES=(latest)
BRANCHES=(dev)

# HACK: ensure correct submodule versions
git submodule update --init

# echo ${GHCR_TOKEN}  | docker login ghcr.io -u jordan-evens --password-stdin
az acr login --name registrycwfisdev || (az login && az acr login --name registrycwfisdev)
build_tag_and_push() {
    echo $1
    echo $2
    src=$1
    img="${src}:${VERSION}"
    repo=$2
    docker compose build --no-cache ${src}-hotfix
    set +e
    docker rmi ${repo}${img}
    for branch in ${BRANCHES[*]}; do
        docker rmi ${repo}${src}:${branch}
    done
    set -e
    docker tag ${img} ${repo}${img}
    for branch in ${BRANCHES[*]}; do
        docker tag ${img} ${repo}${src}:${branch}
    done
    docker push ${repo}${src}
    docker push ${repo}${img}
    for branch in ${BRANCHES[*]}; do
        docker push ${repo}${src}:${branch}
    done
}

for src in ${SRCS[*]}; do
    for repo in ${REPOS[*]}; do
        build_tag_and_push ${src} ${repo}
    done
done
