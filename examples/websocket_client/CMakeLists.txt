#add_executable(testclient
      #cpslib/pslib.h
      #json/json.h
      #json/json-forwards.h
      #easywsclient.cpp
      #easywsclient.hpp
      #jsoncpp.cpp
      #testclient.cpp
      #WebSocketWrapper.cpp
      #WebSocketWrapper.hpp)

#set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cpslib/")
#find_library(CPS_LIB NAMES pslib)
#target_link_libraries(testclient pslib "${CPS_LIB}" rtcdcpp)

#add_executable(c_offer
            #c_offer.c
            #"${CMAKE_CURRENT_SOURCE_DIR}/../../include/rtcdcpp/librtcdcpp.h"
            #)
#target_link_libraries(c_offer rtcdcpp)
#set_target_properties(c_offer
  #PROPERTIES LINKER_LANGUAGE C
  #)

add_executable(2in1
            2in1.c
            "${CMAKE_CURRENT_SOURCE_DIR}/../../include/rtcdcpp/librtcdcpp.h"
            )
target_link_libraries(2in1 rtcdcpp)
set_target_properties(2in1
  PROPERTIES LINKER_LANGUAGE C
  )

#add_executable(c_answer
            #c_answer.c
            #"${CMAKE_CURRENT_SOURCE_DIR}/../../include/rtcdcpp/librtcdcpp.h"
            #)
#target_link_libraries(c_answer rtcdcpp)
#set_target_properties(c_answer
  #PROPERTIES LINKER_LANGUAGE C
  #)

#add_executable(testclientoffer
      #json/json.h
      #json/json-forwards.h
      #easywsclient.cpp
      #easywsclient.hpp
      #jsoncpp.cpp
      #testclient_offer.cpp
      #WebSocketWrapper.cpp
      #WebSocketWrapper.hpp)
#target_link_libraries(testclientoffer rtcdcpp)

#add_executable(testclientanswer
      #json/json.h
      #json/json-forwards.h
      #easywsclient.cpp
      #easywsclient.hpp
      #jsoncpp.cpp
      #testclient_answer.cpp
      #WebSocketWrapper.cpp
      #WebSocketWrapper.hpp)
#target_link_libraries(testclientanswer rtcdcpp)
