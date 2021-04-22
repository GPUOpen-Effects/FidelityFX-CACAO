project(FFX_CACAO_Sample_DX12)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)

add_compile_options(/MP)

set(sources
    Sample.cpp
    Sample.h
    SampleRenderer.cpp
    SampleRenderer.h
    ../../../ffx-cacao/src/ffx_cacao_defines.h
    ../../../ffx-cacao/src/ffx_cacao.cpp
    ../../../ffx-cacao/inc/ffx_cacao.h
    ../../../ffx-cacao/src/ffx_cacao_impl.cpp
    ../../../ffx-cacao/inc/ffx_cacao_impl.h
    ../Common/Common.h
    stdafx.cpp
    stdafx.h)

set(shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/ffx_cacao.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/ffx_cacao_defines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Apply_CACAO.hlsl)

set(config
	${CMAKE_CURRENT_SOURCE_DIR}/../Common/SampleSettings.json
)

source_group("Sources" FILES ${sources})
source_group("Shaders" FILES ${shaders})
source_group("Config" FILES ${config})

copyCommand("${shaders}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibDX)
copyCommand("${config}" ${CMAKE_HOME_DIRECTORY}/bin)

add_executable(${PROJECT_NAME} WIN32 ${sources} ${shaders} ${config})

add_custom_command(
    TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src/build_shaders_dxil.bat)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Cauldron_DX12 ImGUI amd_ags DXC d3dcompiler D3D12)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Common)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../ffx-cacao/src)
add_definitions(-DFFX_CACAO_ENABLE_PROFILING -DFFX_CACAO_ENABLE_D3D12)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")
set_source_files_properties(${shaders} PROPERTIES VS_TOOL_OVERRIDE "Text")

addManifest(${PROJECT_NAME})
