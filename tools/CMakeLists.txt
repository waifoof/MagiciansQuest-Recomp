add_executable(rominfo rominfo/main.cpp)
target_link_libraries(rominfo PRIVATE ntr_common)
add_executable(romdump romdump/main.cpp)
target_link_libraries(romdump PRIVATE ntr_recomp ntr_common)
# Reference runner on melonDS's interpreter (development only, Linux).
if(NOT WIN32)
  add_executable(mqref mqref/main.cpp)
  target_link_libraries(mqref PRIVATE "$<LINK_GROUP:RESCAN,melonds_core,melonds_net>")
endif()
