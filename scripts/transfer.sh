#!/bin/bash
# Auto-transfer script for syspro2023 bare-metal part
# Written by PENG AO

# Usage: outside the current workspace
# run `bash xxx/scripts/transfer.sh src-dir tar-dir`

# check arguments
[[ $# -ne 2 ]] && echo "Both source and target directory are required." && exit 1
[[ ! -d $1 ]] && echo "$1 is not a valid directory." && exit 1
[[ ! -d $2 ]] && echo "$2 is not a valid directory." && exit 1

# check git status
trap "popd > /dev/null 2>&1" 1
for dir in $@; do
    pushd $dir > /dev/null 2>&1
    [[ `git status --porcelain | wc -l` -ne 0 ]] && echo "$dir should be clean." && exit 1
    popd > /dev/null 2>&1
done
trap - 1

# start copying
pushd $1 > /dev/null 2>&1
for file in $(git ls-files); do
    [[ -f ../$2/$file ]] && continue
    cp --parents $file ../$2 > /dev/null 2>&1
done
popd > /dev/null 2>&1

# auto save
pushd $2 > /dev/null 2>&1
git add . > /dev/null 2>&1
git commit -m "auto: transferred from $1" > /dev/null 2>&1
popd > /dev/null 2>&1
