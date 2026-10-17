add_executable(rominfo rominfo/main.cpp)
target_link_libraries(rominfo PRIVATE ntr_common)
add_executable(romdump romdump/main.cpp)
target_link_libraries(romdump PRIVATE ntr_recomp ntr_common)
