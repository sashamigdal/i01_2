#!/bin/bash

set -e -u

ROOTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "$ROOTDIR"

failure=0
for i in hooks/*.sh; do
    h=`basename "$i" .sh`
    ln -s ../../"$i" .git/hooks/`basename "$i" .sh` || {
        failure=1
        echo "Could not symlink ${h} -> ../${i}: File exists." >&2
    }
done

[ $failure -ne 0 ] && exit 1

exit 0
