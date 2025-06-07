#!/bin/sh

[ -d m4 ] || mkdir m4
[ -d config ] || mkdir config

autoreconf -iv -W all
