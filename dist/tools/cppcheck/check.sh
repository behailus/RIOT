#!/bin/sh

BRANCH=${1}
FILEREGEX='\.([sScHh]|cpp)$'

# If no branch but an option is given, unset BRANCH.
# Otherwise, consume this parameter.
if echo "${BRANCH}" | grep -q '^-'; then
    BRANCH=""
else
    shift 1
fi

# If the --diff-filter option is given, consume this parameter.
# Set the default DIFFFILTER option otherwise.
DIFFFILTER="${1}"
if echo "${DIFFFILTER}" | grep -q '^--diff-filter='; then
    shift 1
else
    DIFFFILTER="--diff-filter=ACMR"
fi

# select either all or only touched-in-branch files, filter through FILEREGEX
if [ -z "${BRANCH}" ]; then
    FILES="$(git ls-tree -r --full-tree --name-only HEAD | grep -E ${FILEREGEX})"
else
    FILES="$(git diff ${DIFFFILTER} --name-only ${BRANCH} | grep -E ${FILEREGEX})"
fi

if [ -z "${FILES}" ]; then
    exit
fi

cppcheck --std=c99 --enable=style --force \
    --error-exitcode=2 --inline-suppr --quiet -j 8 \
    --template "{file}:{line}: {severity} ({id}): {message}" \
    ${@} ${FILES}
