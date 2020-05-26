set(SOURCES
  app.cpp
  app.hpp
  camera.cpp
  camera.hpp
  main.cpp
  shader.cpp
  shader.hpp
  stb_image.cpp
  stb_image.h
  volume.cpp
  volume.hpp
  )

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
  glfw
  glm
  ${GLAD_LIBRARIES}
  ${IMGUI_LIBRARIES}
  )

include_directories(
  ${GLAD_INCLUDE_DIR}
  ${IMGUI_INCLUDE_DIR}
  )

set (source "${CMAKE_SOURCE_DIR}/res")
set (destination "${CMAKE_BINARY_DIR}/res")
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
  DEPENDS ${destination}
  COMMENT "symbolic link resources folder from ${source} => ${destination}"
  )
