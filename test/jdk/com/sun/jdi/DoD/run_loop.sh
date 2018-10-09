#!/usr/bin/env bash

if [[ $# != 1 ]]
then
  echo "Syntax: run_loop <jdk-image-dir>"
  exit 1
fi

WD=$(pwd)
cd "$1"
JH=$(PWD)
cd "$WD"
cd $(dirname "$0")
"$JH"/bin/javac -g -d . InfLoop.java FakeJdb.java
mv -f *.class "$WD"/
cd "$WD"

"$JH"/bin/java -agentlib:jdwp=transport=dt_socket,ondemand=y -cp . InfLoop
