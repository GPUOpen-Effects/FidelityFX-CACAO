project(${PROJECT_NAME})
include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)

add_compile_options(/MP)

set(sources
    FFX_CACAO_Sample.cpp
    FFX_CACAO_Sample.h
    SampleRenderer.cpp
    SampleRenderer.h
    ../../../ffx-cacao/src/ffx_cacao_defines.h
    ../../../ffx-cacao/src/ffx_cacao.cpp
    ../../../ffx-cacao/inc/ffx_cacao.h
    ../Common/FFX_CACAO_Common.h
    stdafx.cpp
    stdafx.h)

set(shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/ffx_cacao.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/ffx_cacao_defines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Apply_CACAO_Direct.glsl)

set(config
	${CMAKE_CURRENT_SOURCE_DIR}/../Common/FFX_CACAO_Sample.json
)

copyCommand("${shaders}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibVK)
copyCommand("${config}" ${CMAKE_HOME_DIRECTORY}/bin)

source_group("Sources" FILES ${sources})
source_group("Shaders" FILES ${shaders})
source_group("Config" FILES ${config})

add_executable(${PROJECT_NAME} WIN32 ${sources} ${shaders} ${config})

add_custom_command(
    TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/build_shaders_spirv.bat)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Cauldron_VK ImGUI Vulkan::Vulkan)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Common)
add_definitions(-DFFX_CACAO_ENABLE_PROFILING -DFFX_CACAO_ENABLE_VULKAN)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")
set_source_files_properties(${shaders} PROPERTIES VS_TOOL_OVERRIDE "Text")

addManifest(${PROJECT_NAME})
