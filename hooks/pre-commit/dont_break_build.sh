#!/bin/bash

./build.sh >/dev/null
RESULT=$?

[ $RESULT -ne 0 ] && {
    cat >&2 <<EOF
Build-breaking commit attempted, you have been reported to the build police!
EOF
    exit 1
}

exit 0
