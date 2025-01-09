###########################################################
## TARGETS
###########################################################

add_executable(taic-bi taic-bi.c uintr.S)
add_executable(taic-uni taic-uni.c uintr.S)
add_executable(taic-bi-uint taic-bi-uint.c uintr.S)
add_executable(taic-uni-uint taic-uni-uint.c uintr.S)

###########################################################
## COMMON
###########################################################

target_link_libraries(taic-bi ipc-bench-common pthread)
target_link_libraries(taic-uni ipc-bench-common pthread)
target_link_libraries(taic-bi-uint ipc-bench-common pthread)
target_link_libraries(taic-uni-uint ipc-bench-common pthread)
