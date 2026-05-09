# 服务端启动
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
./im_server
