export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:../dylib"
LD_PRELOAD=../dylib/libruntime.so ./lab1pa3  "$@"
