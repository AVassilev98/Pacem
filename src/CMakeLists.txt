add_executable(pararay
    main.cpp
)

target_link_libraries(pararay
    vulkan
    glfw
)