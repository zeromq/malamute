#!/usr/bin/env bash
set -e

docker run -v "$REPO_DIR":/gsl zeromqorg/zproject project.xml

if [[ $(git diff) ]]; then
    exit 1
fi
if [[ $(git status -s) ]]; then
    exit 1
fi
