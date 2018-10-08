#!/usr/bin/env bash

if [[ $# != 2 ]]
then
  echo "Syntax: run_stress <jdk-image-dir> [0-10|all]"
  exit 1
fi

cd $(dirname "$0")
SCRIPT_PATH=$(pwd)
cd -

expect "$SCRIPT_PATH"/stress.tcl "$1" InfLoop "$2"
