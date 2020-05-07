project(FFX_CACAO_Sample_DX12)

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
    stdafx.cpp
    stdafx.h)

set(shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/ffx_cacao.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/ffx_cacao_defines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Apply_CACAO.hlsl)

source_group("Sources" FILES ${sources})
source_group("Shaders" FILES ${shaders})

copyCommand("${shaders}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibDX)

add_executable(${PROJECT_NAME} WIN32 ${sources} ${shaders})

add_custom_command(
    TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/build_shaders.bat)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Cauldron_DX12 ImGUI amd_ags DXC d3dcompiler D3D12)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")
set_source_files_properties(${shaders} PROPERTIES VS_TOOL_OVERRIDE "Text")

addManifest(${PROJECT_NAME})
