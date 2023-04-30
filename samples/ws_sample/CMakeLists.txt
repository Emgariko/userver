project(userver-samples-ws_sample CXX)

add_executable(${PROJECT_NAME} "sample.cpp" http_chat.cpp)
target_link_libraries(${PROJECT_NAME} userver-core)

userver_testsuite_add(
        SERVICE_TARGET ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
        PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
        PYTEST_ARGS
        --service-config=${CMAKE_CURRENT_SOURCE_DIR}/config_dev.yaml
        --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
        --config-fallback=${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json
#        --service-config-vars=${CMAKE_CURRENT_SOURCE_DIR}/config_vars.yaml
        --service-port=8080
)