#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

sudo rmmod dune
sudo insmod ${DIR}/kern/dune.ko
sudo chmod 777 /dev/dune
