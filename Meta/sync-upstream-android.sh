#!/usr/bin/env bash
set -euo pipefail

UPSTREAM_URL="${UPSTREAM_URL:-https://github.com/LadybirdBrowser/ladybird.git}"
UPSTREAM_BRANCH="${UPSTREAM_BRANCH:-master}"

if ! git remote get-url upstream >/dev/null 2>&1; then
    echo "Adding upstream remote: ${UPSTREAM_URL}"
    git remote add upstream "${UPSTREAM_URL}"
fi

echo "Fetching upstream/${UPSTREAM_BRANCH}..."
git fetch upstream "${UPSTREAM_BRANCH}"

CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
echo "Merging upstream/${UPSTREAM_BRANCH} into ${CURRENT_BRANCH}..."
git merge --no-edit "upstream/${UPSTREAM_BRANCH}"

echo "Sync complete! Current HEAD: $(git rev-parse --short HEAD)"
