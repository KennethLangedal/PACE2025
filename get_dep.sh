#!/bin/bash

git clone git@github.com:KarlsruheMIS/DataReductions.git
cd DataReductions

make mwis_reductions.a

mv -f mwis_reductions.a ../
cp -f include/mwis_reductions.h ../include/

cd ..
rm -rf DataReductions