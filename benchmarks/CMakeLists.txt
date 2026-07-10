add_executable(chess_benchmarks
    main.cpp
)

target_link_libraries(chess_benchmarks PRIVATE
    chess_core
)
