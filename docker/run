#!/bin/bash
#
# Script to build and run Docker container for ScarletDME development.
#
set -ue
cd "$(dirname "$0")"
docker build -f ./Dockerfile -t scarletdme ..
docker run -it scarletdme /bin/bash
