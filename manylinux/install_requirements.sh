#!/bin/bash
set -e

for d in /opt/python/cp{35,36,37,38,39}*; do
    $d/bin/pip install -r /tmp/spead2/requirements.txt
done
