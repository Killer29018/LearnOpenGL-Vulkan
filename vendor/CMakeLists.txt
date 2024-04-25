set(GLM_QUIET
    OFF
    CACHE BOOL "" FORCE)
set(GLM_TEST_ENABLE
    OFF
    CACHE BOOL "" FORCE)
add_subdirectory(glm)

if(NOT UNIX)
  set(GLFW_BUILD_DOCS
      OFF
      CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES
      OFF
      CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(GLFW_INSTALL
      OFF
      CACHE BOOL "" FORCE)
  add_subdirectory(glfw)
  set_target_properties(glfw PROPERTIES FOLDER vendor)
endif()

add_subdirectory(vk-bootstrap)

add_subdirectory(VMA)
target_compile_options(
  VulkanMemoryAllocator BEFORE
  INTERFACE -Wno-nullability-extension -Wno-nullability-completeness
            -Wno-missing-field-initializers -Wno-unused-function)

add_subdirectory(VulkanUtilityLibraries)

# add_subdirectory(tinyobjloader)
#
project(STB)
add_library(STB STATIC "stb/stb_image.h")
target_include_directories(STB PUBLIC "stb")
set_target_properties(STB PROPERTIES LINKER_LANGUAGE CXX)
#
# project(Imgui) file( GLOB IMGUI_FILES CONFIGURE_DEPENDS "imgui/*.h"
# "imgui/*.cpp" "imgui/backends/imgui_impl_glfw.*"
# "imgui/backends/imgui_impl_vulkan.*") add_library(Imgui STATIC ${IMGUI_FILES})
# add_compile_definitions(GLFW_INCLUDE_NONE) target_include_directories(Imgui
# PUBLIC "imgui/" "imgui/backends") set_target_properties(Imgui PROPERTIES
# LINKER_LANGUAGE CXX) target_link_libraries(Imgui PUBLIC glfw Vulkan::Vulkan)
#
# add_subdirectory(fastgltf) target_compile_options(fastgltf BEFORE INTERFACE
# -Wno-sign-compare)
