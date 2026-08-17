#!/usr/bin/env bash
# Publish the distributable addon (project/addons/fluxxishaderlang) to the
# `addon-dist` branch, rooted at the addon directory itself.
#
# Consumers vendor that branch with `git subtree`, so they get the .gdextension,
# the C# bindings and the prebuilt binaries without the C++ source or the
# godot-cpp checkout.
#
# Usage: ./publish-addon.sh [--dry-run]
set -euo pipefail

PREFIX="project/addons/fluxxishaderlang"
BRANCH="addon-dist"
REMOTE="${REMOTE:-origin}"

cd "$(dirname "$0")"

if ! git diff --quiet -- "$PREFIX" || ! git diff --cached --quiet -- "$PREFIX"; then
    echo "error: $PREFIX has uncommitted changes." >&2
    echo "       The split only sees committed history, so publishing now would" >&2
    echo "       ship a stale addon. Commit them first:" >&2
    echo >&2
    echo "         git add $PREFIX && git commit" >&2
    exit 1
fi

echo "==> Splitting $PREFIX into $BRANCH"
git subtree split --prefix="$PREFIX" -b "$BRANCH" --rejoin 2>/dev/null \
    || git subtree split --prefix="$PREFIX" -b "$BRANCH"

if [[ "${1:-}" == "--dry-run" ]]; then
    echo "==> Dry run; branch $BRANCH updated locally, not pushed."
    git log --oneline -3 "$BRANCH"
    exit 0
fi

echo "==> Pushing $BRANCH to $REMOTE"
git push -f "$REMOTE" "$BRANCH"

echo
echo "Done. Consumers update with:"
echo "  git subtree pull --prefix=addons/fluxxishaderlang \\"
echo "    $(git remote get-url "$REMOTE") $BRANCH --squash"
