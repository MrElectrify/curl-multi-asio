add_executable(Example1 Example1.cpp)

target_link_libraries(Example1
	PUBLIC curl-multi-asio)