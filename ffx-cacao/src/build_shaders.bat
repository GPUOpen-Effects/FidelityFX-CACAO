%echo off

pushd %~dp0

if not exist "PrecompiledShaders" mkdir "PrecompiledShaders"

fxc -Fh PrecompiledShaders/CACAOPrepareDownsampledDepths.h                  -Vn CSPrepareDownsampledDepthsDXIL                  -T cs_5_0 -E CSPrepareDownsampledDepths                  ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOPrepareNativeDepths.h                       -Vn CSPrepareNativeDepthsDXIL                       -T cs_5_0 -E CSPrepareNativeDepths                       ffx_cacao.hlsl

fxc -Fh PrecompiledShaders/CACAOPrepareDownsampledDepthsAndMips.h           -Vn CSPrepareDownsampledDepthsAndMipsDXIL           -T cs_5_0 -E CSPrepareDownsampledDepthsAndMips           ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOPrepareNativeDepthsAndMips.h                -Vn CSPrepareNativeDepthsAndMipsDXIL                -T cs_5_0 -E CSPrepareNativeDepthsAndMips                ffx_cacao.hlsl

fxc -Fh PrecompiledShaders/CACAOPrepareDownsampledNormals.h                 -Vn CSPrepareDownsampledNormalsDXIL                 -T cs_5_0 -E CSPrepareDownsampledNormals                 ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOPrepareNativeNormals.h                      -Vn CSPrepareNativeNormalsDXIL                      -T cs_5_0 -E CSPrepareNativeNormals                      ffx_cacao.hlsl

fxc -Fh PrecompiledShaders/CACAOPrepareDownsampledNormalsFromInputNormals.h -Vn CSPrepareDownsampledNormalsFromInputNormalsDXIL -T cs_5_0 -E CSPrepareDownsampledNormalsFromInputNormals ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOPrepareNativeNormalsFromInputNormals.h      -Vn CSPrepareNativeNormalsFromInputNormalsDXIL      -T cs_5_0 -E CSPrepareNativeNormalsFromInputNormals      ffx_cacao.hlsl

fxc -Fh PrecompiledShaders/CACAOPrepareDownsampledDepthsHalf.h              -Vn CSPrepareDownsampledDepthsHalfDXIL              -T cs_5_0 -E CSPrepareDownsampledDepthsHalf              ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOPrepareNativeDepthsHalf.h                   -Vn CSPrepareNativeDepthsHalfDXIL                   -T cs_5_0 -E CSPrepareNativeDepthsHalf                   ffx_cacao.hlsl


fxc -Fh PrecompiledShaders/CACAOGenerateQ0.h     -Vn CSGenerateQ0DXIL     -T cs_5_0 -E CSGenerateQ0     ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOGenerateQ1.h     -Vn CSGenerateQ1DXIL     -T cs_5_0 -E CSGenerateQ1     ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOGenerateQ2.h     -Vn CSGenerateQ2DXIL     -T cs_5_0 -E CSGenerateQ2     ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOGenerateQ3.h     -Vn CSGenerateQ3DXIL     -T cs_5_0 -E CSGenerateQ3     ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOGenerateQ3Base.h -Vn CSGenerateQ3BaseDXIL -T cs_5_0 -E CSGenerateQ3Base ffx_cacao.hlsl

fxc -Fh PrecompiledShaders/CACAOGenerateImportanceMap.h     -Vn CSGenerateImportanceMapDXIL     -T cs_5_0 -E CSGenerateImportanceMap     ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOPostprocessImportanceMapA.h -Vn CSPostprocessImportanceMapADXIL -T cs_5_0 -E CSPostprocessImportanceMapA ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOPostprocessImportanceMapB.h -Vn CSPostprocessImportanceMapBDXIL -T cs_5_0 -E CSPostprocessImportanceMapB ffx_cacao.hlsl

fxc -Fh PrecompiledShaders/CACAOEdgeSensitiveBlur1.h -Vn CSEdgeSensitiveBlur1DXIL -T cs_5_0 -E CSEdgeSensitiveBlur1 ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOEdgeSensitiveBlur2.h -Vn CSEdgeSensitiveBlur2DXIL -T cs_5_0 -E CSEdgeSensitiveBlur2 ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOEdgeSensitiveBlur3.h -Vn CSEdgeSensitiveBlur3DXIL -T cs_5_0 -E CSEdgeSensitiveBlur3 ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOEdgeSensitiveBlur4.h -Vn CSEdgeSensitiveBlur4DXIL -T cs_5_0 -E CSEdgeSensitiveBlur4 ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOEdgeSensitiveBlur5.h -Vn CSEdgeSensitiveBlur5DXIL -T cs_5_0 -E CSEdgeSensitiveBlur5 ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOEdgeSensitiveBlur6.h -Vn CSEdgeSensitiveBlur6DXIL -T cs_5_0 -E CSEdgeSensitiveBlur6 ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOEdgeSensitiveBlur7.h -Vn CSEdgeSensitiveBlur7DXIL -T cs_5_0 -E CSEdgeSensitiveBlur7 ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOEdgeSensitiveBlur8.h -Vn CSEdgeSensitiveBlur8DXIL -T cs_5_0 -E CSEdgeSensitiveBlur8 ffx_cacao.hlsl

fxc -Fh PrecompiledShaders/CACAOApply.h             -Vn CSApplyDXIL             -T cs_5_0 -E CSApply             ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAONonSmartApply.h     -Vn CSNonSmartApplyDXIL     -T cs_5_0 -E CSNonSmartApply     ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAONonSmartHalfApply.h -Vn CSNonSmartHalfApplyDXIL -T cs_5_0 -E CSNonSmartHalfApply ffx_cacao.hlsl

fxc -Fh PrecompiledShaders/CACAOUpscaleBilateral5x5.h        -Vn CSUpscaleBilateral5x5DXIL        -T cs_5_0 -E CSUpscaleBilateral5x5        ffx_cacao.hlsl
fxc -Fh PrecompiledShaders/CACAOUpscaleBilateral5x5Half.h    -Vn CSUpscaleBilateral5x5HalfDXIL    -T cs_5_0 -E CSUpscaleBilateral5x5Half    ffx_cacao.hlsl

popd
