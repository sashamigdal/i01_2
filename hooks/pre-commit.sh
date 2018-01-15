#!/bin/bash

export GIT="${GIT:-git}"

failed=()

echo ">> Running pre-commit hooks..."
echo ">> To bypass this step, use 'git commit --no-verify' ..."

$GIT stash save -q --keep-index
trap "$GIT stash pop -q" EXIT

for i in hooks/pre-commit/*; do
    $i || failed+=("$i")
done

[ ${#failed[@]} -ne 0 ] && {
    echo -e "\033[31m>> Failed pre-commit hooks:"
    for i in ${failed[@]}; do 
        echo "** $i"
    done
    echo -e "\033[31m>> Some pre-commit hooks reported failure.\033[0m"

    if [[ "$($GIT config --bool hooks.i01-allow-failures)" == "true" && "$($GIT symbolic-ref HEAD 2>/dev/null)" != "refs/heads/master" ]]
    then
        echo -e "\033[33m>> Allowing pre-commit hook failures on non-master branches.\033[0m"
        exit 0
    fi

    exit 1
} >&2

echo -e "\033[32m>> All pre-commit hooks reported success!  Continuing...\033[0m"
exit 0
