#!/bin/bash
cd "$(dirname "$0")" || exit 1
apps=("init")

../apps/build.sh
mkdir -p ../distro-files/bin
for app in "${apps[@]}"; do
  cp "../apps/$app" "../distro-files/bin/"
done
