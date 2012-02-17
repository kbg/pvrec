Steps to compile a 64bit release version of PvRec:

  mkdir build
  cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release -DPROSILICA_CPU=x64
  make

This assumes that the PvApi.h is installed in /usr/local/include and the libPvAPI.so file is installed in /usr/local/lib. You can change these paths by modifying the (advanced) PROSILICA_* entries using e.g. "cmake-gui .." or passing additional

      -DPROSILICA_LIBRARY_PVAPI=/path/to/libPvAPI.so
  and -DPROSILICA_INCLUDE_DIR=/path/to/include

to the cmake call. To make an statically linked version of PvRec you can also use the libPvAPI.a file instead of the libPvAPI.so file. This only works if you have a suitable libPvAPI.a version for your compiler (e.g. 4.4/libPvAPI.a for gcc 4.4).


As an example, the following steps will compile a statically linked, 64bit release version of PvRec with an installation of the PvApi at a custom location:

  mkdir build
  cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release -DPROSILICA_CPU=x64 \
   -DPROSILICA_LIBRARY_PVAPI="../../AVT GigE SDK/lib-pc/x64/4.4/libPvAPI.a" \
   -DPROSILICA_INCLUDE_DIR="../../AVT GigE SDK/inc-pc"
  make

