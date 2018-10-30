#!/usr/bin/env bash

if [[ $# < 2 ]]
then
  echo "Syntax: run_stress <jdk-image-dir> <0-15|all> [runs]"
  exit 1
fi

WD=$(pwd)
RUNS=1000000000
if [[ $# == 3 ]]
then
  RUNS=$3
fi
cd $(dirname "$0")
SCRIPT_PATH=$(pwd)
cd -
cd "$WD"

expect "$SCRIPT_PATH"/stress_socket.tcl "$1" InfLoop "$2" "$RUNS"
