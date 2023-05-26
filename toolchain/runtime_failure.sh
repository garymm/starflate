#!/usr/bin/env bash

set -euo pipefail

bin=$1
err=$2

ret=0
$bin &> log || ret=$?
[ $ret -ne 0 ]

grep -q $err log
