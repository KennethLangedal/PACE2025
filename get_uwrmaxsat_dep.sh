#!/bin/bash

if [ ! -f scipoptsuite-9.2.1.tgz ]; then
    echo "Error: scipoptsuite-9.2.1.tgz not found in working directory. Download from https://scipopt.org/index.php#download."
    exit 1
fi

mkdir dep
cd dep

git clone https://github.com/marekpiotrow/UWrMaxSat uwrmaxsat

cd uwrmaxsat
git checkout df7a4890b74373aaeeb3cc8b99c424178cef5c66
cd ..

git clone https://github.com/marekpiotrow/cominisatps
cd cominisatps
rm core simp mtl utils && ln -s minisat/core minisat/simp minisat/mtl minisat/utils .
make lr
cp -f build/release/lib/libcominisatps.a ../../bin/
cd ..

git clone https://github.com/arminbiere/cadical
cd cadical
patch -p1 <../uwrmaxsat/cadical.patch
./configure --no-contracts --no-tracing
make cadical
cp -f build/libcadical.a ../../bin/
cd ../uwrmaxsat
cp config.cadical config.mk
cd ..

git clone https://github.com/Laakeri/maxpre
cd maxpre
sed -i 's/-g/-D NDEBUG/' src/Makefile
make lib
cp -f src/lib/libmaxpre.a ../../bin/
cd ..

cp -r ../scipoptsuite-9.2.1.tgz .
tar zxvf scipoptsuite-9.2.1.tgz
cd scipoptsuite-9.2.1
mkdir build && cd build  
cmake -DSYM=nauty -DSHARED=off -DNO_EXTERNAL_CODE=on -DSOPLEX=on -DTPI=tny ..
cmake --build . --config Release --target libscip libsoplex-pic
cp -f lib/libscip.a ../../../bin/
cp -f lib/libsoplex.a ../../../bin/
cd ../..

cd uwrmaxsat
make clean
#USESCIP= make r
make r
cp -f build/release/lib/libuwrmaxsat.a ../../bin/
cp -f ipamir.h ../../include/

cd ../..
rm -rf dep
rm -rf scipoptsuite-9.2.1.tgz