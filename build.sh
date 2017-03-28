cmake -DLIBNICE_LIBRARY=./libnice.so -DUSRSCTP_LIBRARY=./libusrsctp.so -DLIBNICE_INCLUDE_DIR="./libnice/nice/;./libnice/agent/;./libnice/;" -DUSRSCTP_INCLUDE_DIR=./usrsctp/usrsctplib -DDISABLE_SPD_LOG=True
make
