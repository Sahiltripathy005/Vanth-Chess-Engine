add_executable(chess_tests
    main.cpp
    test_pieces.cpp
    test_moves.cpp
    test_board.cpp
    test_move_generator.cpp
    test_search.cpp
    test_milestone6.cpp
    test_milestone7.cpp
    test_milestone8.cpp
    test_explainable_ai.cpp
)

target_link_libraries(chess_tests PRIVATE
    chess_core
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(chess_tests)
