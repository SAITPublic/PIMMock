
add_executable(pim_test 
                pim_elt_add.cpp
                pim_elt_mul.cpp
                pim_bn.cpp
                pim_copy.cpp
                pim_gemv.cpp
                pim_memory_test.cpp
                pim_relu.cpp
                pim_rect_copy.cpp
                )

target_compile_definitions(pim_test PRIVATE 
                            EMULATOR 
                            TEST_VECTORS_DATA="${CMAKE_SOURCE_DIR}/external/PIMLibrary/test_vectors/")

target_include_directories(pim_test 
                            PRIVATE 
                            ${CMAKE_SOURCE_DIR}/external/half-float)

target_link_libraries(pim_test gtest_main PIMMock)

include(GoogleTest)
gtest_discover_tests(pim_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
