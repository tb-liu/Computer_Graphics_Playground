
# Add source to this project's executable.
add_executable(Playground
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_types.h
    vk_initializers.cpp
    vk_initializers.h
    vk_pipeline.cpp
    vk_pipeline.h
    Defines.h
    RingBuffer.cpp
    RingBuffer.h
    DeletionQueue.cpp
    DeletionQueue.h
    Mesh.cpp
    Mesh.h
    )


set_property(TARGET Playground PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Playground>")

target_include_directories(Playground PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(Playground vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(Playground Vulkan::Vulkan sdl2)

add_dependencies(Playground Shaders)
