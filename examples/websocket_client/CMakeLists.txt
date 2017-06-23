add_executable(testclient
        cpslib/pslib.h
        json/json.h
        json/json-forwards.h
        easywsclient.cpp
        easywsclient.hpp
        jsoncpp.cpp
        testclient.cpp
        WebSocketWrapper.cpp
        WebSocketWrapper.hpp)

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cpslib/")
find_library(CPS_LIB NAMES pslib)
target_link_libraries(testclient pslib "${CPS_LIB}" rtcdcpp)

add_executable(testclientc
            testclient.c
            "${CMAKE_CURRENT_SOURCE_DIR}/../../include/rtcdcpp/librtcdcpp.h"
            )

          #add_definitions(-std=c99)
target_link_libraries(testclientc rtcdcpp)
set_target_properties(testclientc
  PROPERTIES LINKER_LANGUAGE C
  )
#set(CMAKE_C_LINK_EXECUTABLE testclientc)

add_executable(testclientoffer
        json/json.h
        json/json-forwards.h
        easywsclient.cpp
        easywsclient.hpp
        jsoncpp.cpp
        testclient_offer.cpp
        WebSocketWrapper.cpp
        WebSocketWrapper.hpp)
target_link_libraries(testclientoffer rtcdcpp)

add_executable(testclientanswer
        json/json.h
        json/json-forwards.h
        easywsclient.cpp
        easywsclient.hpp
        jsoncpp.cpp
        testclient_answer.cpp
        WebSocketWrapper.cpp
        WebSocketWrapper.hpp)
target_link_libraries(testclientanswer rtcdcpp)
