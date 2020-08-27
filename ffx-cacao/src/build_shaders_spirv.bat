%echo off

pushd %~dp0

set cauldron_dxc_16=..\..\sample\libs\cauldron\libs\DXC\bin\dxc.exe -Wno-conversion -spirv -T cs_6_2 -enable-16bit-types -fspv-target-env=vulkan1.1 -fvk-s-shift 0 0 -fvk-b-shift 10 0 -fvk-t-shift 20 0 -fvk-u-shift 30 0
set cauldron_dxc_32=..\..\sample\libs\cauldron\libs\DXC\bin\dxc.exe -Wno-conversion -spirv -T cs_6_2 -fspv-target-env=vulkan1.1 -fvk-s-shift 0 0 -fvk-b-shift 10 0 -fvk-t-shift 20 0 -fvk-u-shift 30 0

if not exist "PrecompiledShadersSPIRV" mkdir "PrecompiledShadersSPIRV"

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOClearLoadCounter_16.h -Vn CSClearLoadCounterSPIRV16 -E CSClearLoadCounter ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledDepths_16.h                  -Vn CSPrepareDownsampledDepthsSPIRV16                  -E CSPrepareDownsampledDepths                  ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeDepths_16.h                       -Vn CSPrepareNativeDepthsSPIRV16                       -E CSPrepareNativeDepths                       ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledDepthsAndMips_16.h           -Vn CSPrepareDownsampledDepthsAndMipsSPIRV16            -E CSPrepareDownsampledDepthsAndMips           ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeDepthsAndMips_16.h                -Vn CSPrepareNativeDepthsAndMipsSPIRV16                 -E CSPrepareNativeDepthsAndMips                ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledNormals_16.h                 -Vn CSPrepareDownsampledNormalsSPIRV16                  -E CSPrepareDownsampledNormals                 ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeNormals_16.h                      -Vn CSPrepareNativeNormalsSPIRV16                       -E CSPrepareNativeNormals                      ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledNormalsFromInputNormals_16.h -Vn CSPrepareDownsampledNormalsFromInputNormalsSPIRV16  -E CSPrepareDownsampledNormalsFromInputNormals ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeNormalsFromInputNormals_16.h      -Vn CSPrepareNativeNormalsFromInputNormalsSPIRV16       -E CSPrepareNativeNormalsFromInputNormals      ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledDepthsHalf_16.h              -Vn CSPrepareDownsampledDepthsHalfSPIRV16               -E CSPrepareDownsampledDepthsHalf              ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeDepthsHalf_16.h                   -Vn CSPrepareNativeDepthsHalfSPIRV16                    -E CSPrepareNativeDepthsHalf                   ffx_cacao.hlsl


%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ0_16.h     -Vn CSGenerateQ0SPIRV16      -E CSGenerateQ0     ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ1_16.h     -Vn CSGenerateQ1SPIRV16      -E CSGenerateQ1     ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ2_16.h     -Vn CSGenerateQ2SPIRV16      -E CSGenerateQ2     ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ3_16.h     -Vn CSGenerateQ3SPIRV16      -E CSGenerateQ3     ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ3Base_16.h -Vn CSGenerateQ3BaseSPIRV16  -E CSGenerateQ3Base ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOGenerateImportanceMap_16.h     -Vn CSGenerateImportanceMapSPIRV16      -E CSGenerateImportanceMap     ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPostprocessImportanceMapA_16.h -Vn CSPostprocessImportanceMapASPIRV16  -E CSPostprocessImportanceMapA ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOPostprocessImportanceMapB_16.h -Vn CSPostprocessImportanceMapBSPIRV16  -E CSPostprocessImportanceMapB ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur1_16.h -Vn CSEdgeSensitiveBlur1SPIRV16  -E CSEdgeSensitiveBlur1 ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur2_16.h -Vn CSEdgeSensitiveBlur2SPIRV16  -E CSEdgeSensitiveBlur2 ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur3_16.h -Vn CSEdgeSensitiveBlur3SPIRV16  -E CSEdgeSensitiveBlur3 ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur4_16.h -Vn CSEdgeSensitiveBlur4SPIRV16  -E CSEdgeSensitiveBlur4 ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur5_16.h -Vn CSEdgeSensitiveBlur5SPIRV16  -E CSEdgeSensitiveBlur5 ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur6_16.h -Vn CSEdgeSensitiveBlur6SPIRV16  -E CSEdgeSensitiveBlur6 ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur7_16.h -Vn CSEdgeSensitiveBlur7SPIRV16  -E CSEdgeSensitiveBlur7 ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur8_16.h -Vn CSEdgeSensitiveBlur8SPIRV16  -E CSEdgeSensitiveBlur8 ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOApply_16.h             -Vn CSApplySPIRV16              -E CSApply             ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAONonSmartApply_16.h     -Vn CSNonSmartApplySPIRV16      -E CSNonSmartApply     ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAONonSmartHalfApply_16.h -Vn CSNonSmartHalfApplySPIRV16  -E CSNonSmartHalfApply ffx_cacao.hlsl

%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOUpscaleBilateral5x5_16.h        -Vn CSUpscaleBilateral5x5SPIRV16         -E CSUpscaleBilateral5x5        ffx_cacao.hlsl
%cauldron_dxc_16% -Fh PrecompiledShadersSPIRV/CACAOUpscaleBilateral5x5Half_16.h    -Vn CSUpscaleBilateral5x5HalfSPIRV16     -E CSUpscaleBilateral5x5Half    ffx_cacao.hlsl


%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOClearLoadCounter_32.h -Vn CSClearLoadCounterSPIRV32 -E CSClearLoadCounter ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledDepths_32.h                  -Vn CSPrepareDownsampledDepthsSPIRV32                  -E CSPrepareDownsampledDepths                  ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeDepths_32.h                       -Vn CSPrepareNativeDepthsSPIRV32                       -E CSPrepareNativeDepths                       ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledDepthsAndMips_32.h           -Vn CSPrepareDownsampledDepthsAndMipsSPIRV32            -E CSPrepareDownsampledDepthsAndMips           ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeDepthsAndMips_32.h                -Vn CSPrepareNativeDepthsAndMipsSPIRV32                 -E CSPrepareNativeDepthsAndMips                ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledNormals_32.h                 -Vn CSPrepareDownsampledNormalsSPIRV32                  -E CSPrepareDownsampledNormals                 ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeNormals_32.h                      -Vn CSPrepareNativeNormalsSPIRV32                       -E CSPrepareNativeNormals                      ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledNormalsFromInputNormals_32.h -Vn CSPrepareDownsampledNormalsFromInputNormalsSPIRV32  -E CSPrepareDownsampledNormalsFromInputNormals ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeNormalsFromInputNormals_32.h      -Vn CSPrepareNativeNormalsFromInputNormalsSPIRV32       -E CSPrepareNativeNormalsFromInputNormals      ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareDownsampledDepthsHalf_32.h              -Vn CSPrepareDownsampledDepthsHalfSPIRV32               -E CSPrepareDownsampledDepthsHalf              ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPrepareNativeDepthsHalf_32.h                   -Vn CSPrepareNativeDepthsHalfSPIRV32                    -E CSPrepareNativeDepthsHalf                   ffx_cacao.hlsl


%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ0_32.h     -Vn CSGenerateQ0SPIRV32      -E CSGenerateQ0     ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ1_32.h     -Vn CSGenerateQ1SPIRV32      -E CSGenerateQ1     ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ2_32.h     -Vn CSGenerateQ2SPIRV32      -E CSGenerateQ2     ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ3_32.h     -Vn CSGenerateQ3SPIRV32      -E CSGenerateQ3     ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOGenerateQ3Base_32.h -Vn CSGenerateQ3BaseSPIRV32  -E CSGenerateQ3Base ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOGenerateImportanceMap_32.h     -Vn CSGenerateImportanceMapSPIRV32      -E CSGenerateImportanceMap     ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPostprocessImportanceMapA_32.h -Vn CSPostprocessImportanceMapASPIRV32  -E CSPostprocessImportanceMapA ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOPostprocessImportanceMapB_32.h -Vn CSPostprocessImportanceMapBSPIRV32  -E CSPostprocessImportanceMapB ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur1_32.h -Vn CSEdgeSensitiveBlur1SPIRV32  -E CSEdgeSensitiveBlur1 ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur2_32.h -Vn CSEdgeSensitiveBlur2SPIRV32  -E CSEdgeSensitiveBlur2 ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur3_32.h -Vn CSEdgeSensitiveBlur3SPIRV32  -E CSEdgeSensitiveBlur3 ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur4_32.h -Vn CSEdgeSensitiveBlur4SPIRV32  -E CSEdgeSensitiveBlur4 ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur5_32.h -Vn CSEdgeSensitiveBlur5SPIRV32  -E CSEdgeSensitiveBlur5 ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur6_32.h -Vn CSEdgeSensitiveBlur6SPIRV32  -E CSEdgeSensitiveBlur6 ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur7_32.h -Vn CSEdgeSensitiveBlur7SPIRV32  -E CSEdgeSensitiveBlur7 ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOEdgeSensitiveBlur8_32.h -Vn CSEdgeSensitiveBlur8SPIRV32  -E CSEdgeSensitiveBlur8 ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOApply_32.h             -Vn CSApplySPIRV32              -E CSApply             ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAONonSmartApply_32.h     -Vn CSNonSmartApplySPIRV32      -E CSNonSmartApply     ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAONonSmartHalfApply_32.h -Vn CSNonSmartHalfApplySPIRV32  -E CSNonSmartHalfApply ffx_cacao.hlsl

%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOUpscaleBilateral5x5_32.h        -Vn CSUpscaleBilateral5x5SPIRV32         -E CSUpscaleBilateral5x5        ffx_cacao.hlsl
%cauldron_dxc_32% -Fh PrecompiledShadersSPIRV/CACAOUpscaleBilateral5x5Half_32.h    -Vn CSUpscaleBilateral5x5HalfSPIRV32     -E CSUpscaleBilateral5x5Half    ffx_cacao.hlsl

popd
