zephyr_include_directories(.)
target_sources(app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/network_listener.c
    ${CMAKE_CURRENT_SOURCE_DIR}/serial_bridge.c
    ${CMAKE_CURRENT_SOURCE_DIR}/node_manager.c
)