zephyr_include_directories(.)
target_sources(app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/system_health.c
    ${CMAKE_CURRENT_SOURCE_DIR}/vtt_model.c
)