This is a simplified Dalvik virtual machine implementation written
from scratch used for education purpose.

The simple-dvm is designed to be able to run Dhrystone like java benchmark

# How to Build
```shell
make
```
(Optional) verify the implementation:
```shell
make check
```

# How to Run
* Get DEX file of Dhrystone benchmark: [dhry.dex](https://github.com/cycheng/simple-dvm-hw3)
```shell
./simple-dvm dhry.dex
```
or in verbose mode
```shell
./simple-dvm dhry.dex 5
```

Dhrystone java source code is available here: [dhry_src.jar](http://www.okayan.jp/DhrystoneApplet/dhry_src.jar)

