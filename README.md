This is NTU-Android 2014 (https://sites.google.com/site/ntuandroid2014/home)
homework 3, one of the mission is to modify the original simple-dvm to be able
to run dhrystone like java benchmark

Homework 3 requirements :
    https://ntu-android-2014.hackpad.com/Homework-3-q2ZJ0q61uPL

In order to run dhrystone, I implement :
1. simple class loader
2. simple object model (like c++ object layout)
3. required op code for java dhrystone

Please see the git log for details

# How to build
cd simple_dvm
make

# How to run
./simple-dvm dhry.dex

or in verbose mode
./simple-dvm dhry.dex 5

The test data (dhry.dex) can be get here : https://github.com/cycheng/simple-dvm-hw3

dhrystone java source code is available here : wget http://www.okayan.jp/DhrystoneApplet/dhry_src.jar

