project(userver-samples-http_chat CXX)

add_executable(${PROJECT_NAME} "http_chat.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-postgresql)

userver_sample_testsuite_add()
