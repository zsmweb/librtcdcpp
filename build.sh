cmake -DLIBNICE_LIBRARY=./libnice.so -DUSRSCTP_LIBRARY=./libusrsctp.so -DLIBNICE_INCLUDE_DIR="./libnice/nice/;./libnice/agent/;./libnice/;" -DUSRSCTP_INCLUDE_DIR=./usrsctp/usrsctplib -DSPDLOG_INCLUDE_DIR="./spdlog/include/"
make
