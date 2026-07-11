add_executable(vanth_compare
    main_compare.cpp
)

target_include_directories(vanth_compare PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(vanth_compare PRIVATE
    chess_core
)
