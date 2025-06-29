#!/bin/bash

git clone https://github.com/KarlsruheMIS/DataReductions.git
cd DataReductions

git checkout 4637b9e3c19037383ce9da2a0a16aa8990d34aed

make libmwis_reductions.a

mv -f libmwis_reductions.a ../bin/
cp -f include/mwis_reductions.h ../include/

cd ..
rm -rf DataReductions