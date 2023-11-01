#!/bin/sh
set -e

export CFLAGS='-O3 -Wall -Wextra'
export CPPFLAGS=''

make -C TFTPclient
make -C TFTPserver