#!/bin/bash

git clone git@github.com:KarlsruheMIS/DataReductions.git
cd DataReductions

make libmwis_reductions.a

mv -f libmwis_reductions.a ../bin/
cp -f include/mwis_reductions.h ../include/

cd ..
rm -rf DataReductions