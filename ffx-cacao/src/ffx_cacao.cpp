// Modifications Copyright © 2020. Advanced Micro Devices, Inc. All Rights Reserved.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016, Intel Corporation
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
// the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File changes (yyyy-mm-dd)
// 2016-09-07: filip.strugar@intel.com: first commit
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ffx_cacao.h"
#include "ffx_cacao_defines.h"

#include <assert.h>
#include <math.h>

#if FFX_CACAO_ENABLE_D3D12
#include <d3dx12.h>
#endif

#define FFX_CACAO_ENABLE_CAULDRON_DEBUG 1

#define FFX_CACAO_ASSERT(exp) assert(exp)
#define FFX_CACAO_ARRAY_SIZE(xs) (sizeof(xs)/sizeof(xs[0]))
#define FFX_CACAO_COS(x) cosf(x)
#define FFX_CACAO_SIN(x) sinf(x)

#if FFX_CACAO_ENABLE_D3D12
#include "PrecompiledShaders/CACAOPrepareDownsampledDepthsHalf.h"
#include "PrecompiledShaders/CACAOPrepareNativeDepthsHalf.h"

#include "PrecompiledShaders/CACAOPrepareDownsampledDepthsAndMips.h"
#include "PrecompiledShaders/CACAOPrepareNativeDepthsAndMips.h"

#include "PrecompiledShaders/CACAOPrepareDownsampledNormals.h"
#include "PrecompiledShaders/CACAOPrepareNativeNormals.h"

#include "PrecompiledShaders/CACAOPrepareDownsampledNormalsFromInputNormals.h"
#include "PrecompiledShaders/CACAOPrepareNativeNormalsFromInputNormals.h"

#include "PrecompiledShaders/CACAOPrepareDownsampledDepths.h"
#include "PrecompiledShaders/CACAOPrepareNativeDepths.h"

#include "PrecompiledShaders/CACAOGenerateQ0.h"
#include "PrecompiledShaders/CACAOGenerateQ1.h"
#include "PrecompiledShaders/CACAOGenerateQ2.h"
#include "PrecompiledShaders/CACAOGenerateQ3.h"
#include "PrecompiledShaders/CACAOGenerateQ3Base.h"

#include "PrecompiledShaders/CACAOGenerateImportanceMap.h"
#include "PrecompiledShaders/CACAOPostprocessImportanceMapA.h"
#include "PrecompiledShaders/CACAOPostprocessImportanceMapB.h"

#include "PrecompiledShaders/CACAOEdgeSensitiveBlur1.h"
#include "PrecompiledShaders/CACAOEdgeSensitiveBlur2.h"
#include "PrecompiledShaders/CACAOEdgeSensitiveBlur3.h"
#include "PrecompiledShaders/CACAOEdgeSensitiveBlur4.h"
#include "PrecompiledShaders/CACAOEdgeSensitiveBlur5.h"
#include "PrecompiledShaders/CACAOEdgeSensitiveBlur6.h"
#include "PrecompiledShaders/CACAOEdgeSensitiveBlur7.h"
#include "PrecompiledShaders/CACAOEdgeSensitiveBlur8.h"

#include "PrecompiledShaders/CACAOApply.h"
#include "PrecompiledShaders/CACAONonSmartApply.h"
#include "PrecompiledShaders/CACAONonSmartHalfApply.h"

#include "PrecompiledShaders/CACAOUpscaleBilateral5x5.h"
#include "PrecompiledShaders/CACAOUpscaleBilateral5x5Half.h"
#endif

#define MATRIX_ROW_MAJOR_ORDER 1
#define MAX_BLUR_PASSES 8

#if FFX_CACAO_ENABLE_CAULDRON_DEBUG
#include <base/UserMarkers.h>

#define USER_MARKER(name) CAULDRON_DX12::UserMarker __marker(commandList, name)
#else
#define USER_MARKER(name)
#endif

#define FFX_CACAO_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define FFX_CACAO_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define FFX_CACAO_CLAMP(value, lower, upper) FFX_CACAO_MIN(FFX_CACAO_MAX(value, lower), upper)

typedef struct FfxCacaoConstants {
	float                   DepthUnpackConsts[2];
	float                   CameraTanHalfFOV[2];

	float                   NDCToViewMul[2];
	float                   NDCToViewAdd[2];

	float                   DepthBufferUVToViewMul[2];
	float                   DepthBufferUVToViewAdd[2];

	float                   EffectRadius;                           // world (viewspace) maximum size of the shadow
	float                   EffectShadowStrength;                   // global strength of the effect (0 - 5)
	float                   EffectShadowPow;
	float                   EffectShadowClamp;

	float                   EffectFadeOutMul;                       // effect fade out from distance (ex. 25)
	float                   EffectFadeOutAdd;                       // effect fade out to distance   (ex. 100)
	float                   EffectHorizonAngleThreshold;            // limit errors on slopes and caused by insufficient geometry tessellation (0.05 to 0.5)
	float                   EffectSamplingRadiusNearLimitRec;          // if viewspace pixel closer than this, don't enlarge shadow sampling radius anymore (makes no sense to grow beyond some distance, not enough samples to cover everything, so just limit the shadow growth; could be SSAOSettingsFadeOutFrom * 0.1 or less)

	float                   DepthPrecisionOffsetMod;
	float                   NegRecEffectRadius;                     // -1.0 / EffectRadius
	float                   LoadCounterAvgDiv;                      // 1.0 / ( halfDepthMip[SSAO_DEPTH_MIP_LEVELS-1].sizeX * halfDepthMip[SSAO_DEPTH_MIP_LEVELS-1].sizeY )
	float                   AdaptiveSampleCountLimit;

	float                   InvSharpness;
	int                     PassIndex;
	float                   BilateralSigmaSquared;
	float                   BilateralSimilarityDistanceSigma;

	float                   PatternRotScaleMatrices[5][4];

	float                   NormalsUnpackMul;
	float                   NormalsUnpackAdd;
	float                   DetailAOStrength;
	float                   Dummy0;

	float                   SSAOBufferDimensions[2];
	float                   SSAOBufferInverseDimensions[2];

	float                   DepthBufferDimensions[2];
	float                   DepthBufferInverseDimensions[2];

	int                     DepthBufferOffset[2];
	float                   PerPassFullResUVOffset[2];

	float                   InputOutputBufferDimensions[2];
	float                   InputOutputBufferInverseDimensions[2];

	float                   ImportanceMapDimensions[2];
	float                   ImportanceMapInverseDimensions[2];

	float                   DeinterleavedDepthBufferDimensions[2];
	float                   DeinterleavedDepthBufferInverseDimensions[2];

	float                   DeinterleavedDepthBufferOffset[2];
	float                   DeinterleavedDepthBufferNormalisedOffset[2];

	FfxCacaoMatrix4x4       NormalsWorldToViewspaceMatrix;
} FfxCacaoConstants;

typedef struct ScreenSizeInfo {
	uint32_t width;
	uint32_t height;
	uint32_t halfWidth;
	uint32_t halfHeight;
	uint32_t quarterWidth;
	uint32_t quarterHeight;
	uint32_t eighthWidth;
	uint32_t eighthHeight;
	uint32_t depthBufferWidth;
	uint32_t depthBufferHeight;
	uint32_t depthBufferHalfWidth;
	uint32_t depthBufferHalfHeight;
	uint32_t depthBufferQuarterWidth;
	uint32_t depthBufferQuarterHeight;
	uint32_t depthBufferOffsetX;
	uint32_t depthBufferOffsetY;
	uint32_t depthBufferHalfOffsetX;
	uint32_t depthBufferHalfOffsetY;
} ScreenSizeInfo;

typedef struct BufferSizeInfo {
	uint32_t inputOutputBufferWidth;
	uint32_t inputOutputBufferHeight;

	uint32_t ssaoBufferWidth;
	uint32_t ssaoBufferHeight;

	uint32_t depthBufferXOffset;
	uint32_t depthBufferYOffset;

	uint32_t depthBufferWidth;
	uint32_t depthBufferHeight;

	uint32_t deinterleavedDepthBufferXOffset;
	uint32_t deinterleavedDepthBufferYOffset;

	uint32_t deinterleavedDepthBufferWidth;
	uint32_t deinterleavedDepthBufferHeight;

	uint32_t importanceMapWidth;
	uint32_t importanceMapHeight;
};

static const FfxCacaoMatrix4x4 FFX_CACAO_IDENTITY_MATRIX = {
	{ { 1.0f, 0.0f, 0.0f, 0.0f },
	  { 0.0f, 1.0f, 0.0f, 0.0f },
	  { 0.0f, 0.0f, 1.0f, 0.0f },
	  { 0.0f, 0.0f, 0.0f, 1.0f } }
};

inline static uint32_t dispatchSize(uint32_t tileSize, uint32_t totalSize)
{
	return (totalSize + tileSize - 1) / tileSize;
}

static void updateConstants(FfxCacaoConstants* consts, FfxCacaoSettings* settings, BufferSizeInfo* bufferSizeInfo, const FfxCacaoMatrix4x4* proj, const FfxCacaoMatrix4x4* normalsToView)
{
	consts->BilateralSigmaSquared = settings->bilateralSigmaSquared;
	consts->BilateralSimilarityDistanceSigma = settings->bilateralSimilarityDistanceSigma;

	if (settings->generateNormals)
	{
		consts->NormalsWorldToViewspaceMatrix = FFX_CACAO_IDENTITY_MATRIX;
	}
	else
	{
		consts->NormalsWorldToViewspaceMatrix = *normalsToView;
	}

	// used to get average load per pixel; 9.0 is there to compensate for only doing every 9th InterlockedAdd in PSPostprocessImportanceMapB for performance reasons
	consts->LoadCounterAvgDiv = 9.0f / (float)(bufferSizeInfo->importanceMapWidth * bufferSizeInfo->importanceMapHeight * 255.0);

	float depthLinearizeMul = (MATRIX_ROW_MAJOR_ORDER) ? (-proj->elements[3][2]) : (-proj->elements[2][3]);           // float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
	float depthLinearizeAdd = (MATRIX_ROW_MAJOR_ORDER) ? (proj->elements[2][2]) : (proj->elements[2][2]);           // float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
	// correct the handedness issue. need to make sure this below is correct, but I think it is.
	if (depthLinearizeMul * depthLinearizeAdd < 0)
		depthLinearizeAdd = -depthLinearizeAdd;
	consts->DepthUnpackConsts[0] = depthLinearizeMul;
	consts->DepthUnpackConsts[1] = depthLinearizeAdd;

	float tanHalfFOVY = 1.0f / proj->elements[1][1];    // = tanf( drawContext.Camera.GetYFOV( ) * 0.5f );
	float tanHalfFOVX = 1.0F / proj->elements[0][0];    // = tanHalfFOVY * drawContext.Camera.GetAspect( );
	consts->CameraTanHalfFOV[0] = tanHalfFOVX;
	consts->CameraTanHalfFOV[1] = tanHalfFOVY;

	consts->NDCToViewMul[0] = consts->CameraTanHalfFOV[0] * 2.0f;
	consts->NDCToViewMul[1] = consts->CameraTanHalfFOV[1] * -2.0f;
	consts->NDCToViewAdd[0] = consts->CameraTanHalfFOV[0] * -1.0f;
	consts->NDCToViewAdd[1] = consts->CameraTanHalfFOV[1] * 1.0f;

	float ratio = ((float)bufferSizeInfo->inputOutputBufferWidth) / ((float)bufferSizeInfo->depthBufferWidth);
	float border = (1.0f - ratio) / 2.0f;
	for (int i = 0; i < 2; ++i)
	{
		consts->DepthBufferUVToViewMul[i] = consts->NDCToViewMul[i] / ratio;
		consts->DepthBufferUVToViewAdd[i] = consts->NDCToViewAdd[i] - consts->NDCToViewMul[i] * border / ratio;
	}

	consts->EffectRadius = FFX_CACAO_CLAMP(settings->radius, 0.0f, 100000.0f);
	consts->EffectShadowStrength = FFX_CACAO_CLAMP(settings->shadowMultiplier * 4.3f, 0.0f, 10.0f);
	consts->EffectShadowPow = FFX_CACAO_CLAMP(settings->shadowPower, 0.0f, 10.0f);
	consts->EffectShadowClamp = FFX_CACAO_CLAMP(settings->shadowClamp, 0.0f, 1.0f);
	consts->EffectFadeOutMul = -1.0f / (settings->fadeOutTo - settings->fadeOutFrom);
	consts->EffectFadeOutAdd = settings->fadeOutFrom / (settings->fadeOutTo - settings->fadeOutFrom) + 1.0f;
	consts->EffectHorizonAngleThreshold = FFX_CACAO_CLAMP(settings->horizonAngleThreshold, 0.0f, 1.0f);

	// 1.2 seems to be around the best trade off - 1.0 means on-screen radius will stop/slow growing when the camera is at 1.0 distance, so, depending on FOV, basically filling up most of the screen
	// This setting is viewspace-dependent and not screen size dependent intentionally, so that when you change FOV the effect stays (relatively) similar.
	float effectSamplingRadiusNearLimit = (settings->radius * 1.2f);

	// if the depth precision is switched to 32bit float, this can be set to something closer to 1 (0.9999 is fine)
	consts->DepthPrecisionOffsetMod = 0.9992f;

	// consts->RadiusDistanceScalingFunctionPow     = 1.0f - CLAMP( m_settings.RadiusDistanceScalingFunction, 0.0f, 1.0f );


	// Special settings for lowest quality level - just nerf the effect a tiny bit
	if (settings->qualityLevel <= FFX_CACAO_QUALITY_LOW)
	{
		//consts.EffectShadowStrength     *= 0.9f;
		effectSamplingRadiusNearLimit *= 1.50f;

		if (settings->qualityLevel == FFX_CACAO_QUALITY_LOWEST)
			consts->EffectRadius *= 0.8f;
	}

	effectSamplingRadiusNearLimit /= tanHalfFOVY; // to keep the effect same regardless of FOV

	consts->EffectSamplingRadiusNearLimitRec = 1.0f / effectSamplingRadiusNearLimit;

	consts->AdaptiveSampleCountLimit = settings->adaptiveQualityLimit;

	consts->NegRecEffectRadius = -1.0f / consts->EffectRadius;

	consts->InvSharpness = FFX_CACAO_CLAMP(1.0f - settings->sharpness, 0.0f, 1.0f);

	consts->DetailAOStrength = settings->detailShadowStrength;

	// set buffer size constants.
	consts->SSAOBufferDimensions[0] = (float)bufferSizeInfo->ssaoBufferWidth;
	consts->SSAOBufferDimensions[1] = (float)bufferSizeInfo->ssaoBufferHeight;
	consts->SSAOBufferInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->ssaoBufferWidth);
	consts->SSAOBufferInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->ssaoBufferHeight);

	consts->DepthBufferDimensions[0] = (float)bufferSizeInfo->depthBufferWidth;
	consts->DepthBufferDimensions[1] = (float)bufferSizeInfo->depthBufferHeight;
	consts->DepthBufferInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->depthBufferWidth);
	consts->DepthBufferInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->depthBufferHeight);

	consts->DepthBufferOffset[0] = bufferSizeInfo->depthBufferXOffset;
	consts->DepthBufferOffset[1] = bufferSizeInfo->depthBufferYOffset;

	consts->InputOutputBufferDimensions[0] = (float)bufferSizeInfo->inputOutputBufferWidth;
	consts->InputOutputBufferDimensions[1] = (float)bufferSizeInfo->inputOutputBufferHeight;
	consts->InputOutputBufferInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->inputOutputBufferWidth);
	consts->InputOutputBufferInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->inputOutputBufferHeight);

	consts->ImportanceMapDimensions[0] = (float)bufferSizeInfo->importanceMapWidth;
	consts->ImportanceMapDimensions[1] = (float)bufferSizeInfo->importanceMapHeight;
	consts->ImportanceMapInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->importanceMapWidth);
	consts->ImportanceMapInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->importanceMapHeight);

	consts->DeinterleavedDepthBufferDimensions[0] = (float)bufferSizeInfo->deinterleavedDepthBufferWidth;
	consts->DeinterleavedDepthBufferDimensions[1] = (float)bufferSizeInfo->deinterleavedDepthBufferHeight;
	consts->DeinterleavedDepthBufferInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->deinterleavedDepthBufferWidth);
	consts->DeinterleavedDepthBufferInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->deinterleavedDepthBufferHeight);

	consts->DeinterleavedDepthBufferOffset[0] = (float)bufferSizeInfo->deinterleavedDepthBufferXOffset;
	consts->DeinterleavedDepthBufferOffset[1] = (float)bufferSizeInfo->deinterleavedDepthBufferYOffset;
	consts->DeinterleavedDepthBufferNormalisedOffset[0] = ((float)bufferSizeInfo->deinterleavedDepthBufferXOffset) / ((float)bufferSizeInfo->deinterleavedDepthBufferWidth);
	consts->DeinterleavedDepthBufferNormalisedOffset[1] = ((float)bufferSizeInfo->deinterleavedDepthBufferYOffset) / ((float)bufferSizeInfo->deinterleavedDepthBufferHeight);

	if (!settings->generateNormals)
	{
		consts->NormalsUnpackMul = 2.0f;  // inputs->NormalsUnpackMul;
		consts->NormalsUnpackAdd = -1.0f; // inputs->NormalsUnpackAdd;
	}
	else
	{
		consts->NormalsUnpackMul = 2.0f;
		consts->NormalsUnpackAdd = -1.0f;
	}
}

static void updatePerPassConstants(FfxCacaoConstants* consts, FfxCacaoSettings* settings, BufferSizeInfo* bufferSizeInfo, int pass)
{
	consts->PerPassFullResUVOffset[0] = ((float)(pass % 2)) / (float)bufferSizeInfo->ssaoBufferWidth;
	consts->PerPassFullResUVOffset[1] = ((float)(pass / 2)) / (float)bufferSizeInfo->ssaoBufferHeight;

	consts->PassIndex = pass;

	float additionalAngleOffset = settings->temporalSupersamplingAngleOffset;  // if using temporal supersampling approach (like "Progressive Rendering Using Multi-frame Sampling" from GPU Pro 7, etc.)
	float additionalRadiusScale = settings->temporalSupersamplingRadiusOffset; // if using temporal supersampling approach (like "Progressive Rendering Using Multi-frame Sampling" from GPU Pro 7, etc.)
	const int subPassCount = 5;
	for (int subPass = 0; subPass < subPassCount; subPass++)
	{
		int a = pass;
		int b = subPass;

		int spmap[5]{ 0, 1, 4, 3, 2 };
		b = spmap[subPass];

		float ca, sa;
		float angle0 = ((float)a + (float)b / (float)subPassCount) * (3.1415926535897932384626433832795f) * 0.5f;
		// angle0 += additionalAngleOffset;

		ca = FFX_CACAO_COS(angle0);
		sa = FFX_CACAO_SIN(angle0);

		float scale = 1.0f + (a - 1.5f + (b - (subPassCount - 1.0f) * 0.5f) / (float)subPassCount) * 0.07f;
		// scale *= additionalRadiusScale;

		consts->PatternRotScaleMatrices[subPass][0] = scale * ca;
		consts->PatternRotScaleMatrices[subPass][1] = scale * -sa;
		consts->PatternRotScaleMatrices[subPass][2] = -scale * sa;
		consts->PatternRotScaleMatrices[subPass][3] = -scale * ca;
	}
}


// =================================================================================
// DirectX 12
// =================================================================================

#if FFX_CACAO_ENABLE_D3D12

static inline FfxCacaoStatus hresultToFfxCacaoStatus(HRESULT hr)
{
	switch (hr)
	{
	case E_FAIL: return FFX_CACAO_STATUS_FAILED;
	case E_INVALIDARG: return FFX_CACAO_STATUS_INVALID_ARGUMENT;
	case E_OUTOFMEMORY: return FFX_CACAO_STATUS_OUT_OF_MEMORY;
	case E_NOTIMPL: return FFX_CACAO_STATUS_INVALID_ARGUMENT;
	case S_FALSE: return FFX_CACAO_STATUS_OK;
	case S_OK: return FFX_CACAO_STATUS_OK;
	default: return FFX_CACAO_STATUS_FAILED;
	}
}

static inline void SetName(ID3D12Object* obj, const char* name)
{
	if (name == NULL)
	{
		return;
	}

	FFX_CACAO_ASSERT(obj != NULL);
	wchar_t buffer[1024];
	swprintf(buffer, FFX_CACAO_ARRAY_SIZE(buffer), L"%S", name);
	obj->SetName(buffer);
}

static inline size_t AlignOffset(size_t uOffset, size_t uAlign)
{
	return ((uOffset + (uAlign - 1)) & ~(uAlign - 1));
}

static size_t GetPixelByteSize(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case(DXGI_FORMAT_R10G10B10A2_TYPELESS):
	case(DXGI_FORMAT_R10G10B10A2_UNORM):
	case(DXGI_FORMAT_R10G10B10A2_UINT):
	case(DXGI_FORMAT_R11G11B10_FLOAT):
	case(DXGI_FORMAT_R8G8B8A8_TYPELESS):
	case(DXGI_FORMAT_R8G8B8A8_UNORM):
	case(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
	case(DXGI_FORMAT_R8G8B8A8_UINT):
	case(DXGI_FORMAT_R8G8B8A8_SNORM):
	case(DXGI_FORMAT_R8G8B8A8_SINT):
	case(DXGI_FORMAT_B8G8R8A8_UNORM):
	case(DXGI_FORMAT_B8G8R8X8_UNORM):
	case(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM):
	case(DXGI_FORMAT_B8G8R8A8_TYPELESS):
	case(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB):
	case(DXGI_FORMAT_B8G8R8X8_TYPELESS):
	case(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB):
	case(DXGI_FORMAT_R16G16_TYPELESS):
	case(DXGI_FORMAT_R16G16_FLOAT):
	case(DXGI_FORMAT_R16G16_UNORM):
	case(DXGI_FORMAT_R16G16_UINT):
	case(DXGI_FORMAT_R16G16_SNORM):
	case(DXGI_FORMAT_R16G16_SINT):
	case(DXGI_FORMAT_R32_TYPELESS):
	case(DXGI_FORMAT_D32_FLOAT):
	case(DXGI_FORMAT_R32_FLOAT):
	case(DXGI_FORMAT_R32_UINT):
	case(DXGI_FORMAT_R32_SINT):
		return 4;

	case(DXGI_FORMAT_BC1_TYPELESS):
	case(DXGI_FORMAT_BC1_UNORM):
	case(DXGI_FORMAT_BC1_UNORM_SRGB):
	case(DXGI_FORMAT_BC4_TYPELESS):
	case(DXGI_FORMAT_BC4_UNORM):
	case(DXGI_FORMAT_BC4_SNORM):
	case(DXGI_FORMAT_R16G16B16A16_FLOAT):
	case(DXGI_FORMAT_R16G16B16A16_TYPELESS):
		return 8;

	case(DXGI_FORMAT_BC2_TYPELESS):
	case(DXGI_FORMAT_BC2_UNORM):
	case(DXGI_FORMAT_BC2_UNORM_SRGB):
	case(DXGI_FORMAT_BC3_TYPELESS):
	case(DXGI_FORMAT_BC3_UNORM):
	case(DXGI_FORMAT_BC3_UNORM_SRGB):
	case(DXGI_FORMAT_BC5_TYPELESS):
	case(DXGI_FORMAT_BC5_UNORM):
	case(DXGI_FORMAT_BC5_SNORM):
	case(DXGI_FORMAT_BC6H_TYPELESS):
	case(DXGI_FORMAT_BC6H_UF16):
	case(DXGI_FORMAT_BC6H_SF16):
	case(DXGI_FORMAT_BC7_TYPELESS):
	case(DXGI_FORMAT_BC7_UNORM):
	case(DXGI_FORMAT_BC7_UNORM_SRGB):
	case(DXGI_FORMAT_R32G32B32A32_FLOAT):
	case(DXGI_FORMAT_R32G32B32A32_TYPELESS):
		return 16;

	default:
		FFX_CACAO_ASSERT(0);
		break;
	}
	return 0;
}

// =================================================================================================
// GpuTimer implementation
// =================================================================================================

#if FFX_CACAO_ENABLE_PROFILING
#define GPU_TIMER_MAX_VALUES_PER_FRAME (FFX_CACAO_ARRAY_SIZE(((FfxCacaoDetailedTiming*)0)->timestamps))

typedef struct GpuTimer {
	ID3D12Resource  *buffer;
	ID3D12QueryHeap *queryHeap;
	uint32_t         numberOfBackBuffers;
	uint32_t         currentFrame;
	uint32_t         currentMeasurement;
	const char      *labels[GPU_TIMER_MAX_VALUES_PER_FRAME];
} GpuTimer;

static FfxCacaoStatus gpuTimerInit(GpuTimer* gpuTimer, ID3D12Device* device, uint32_t numberOfBackBuffers)
{
	gpuTimer->numberOfBackBuffers = numberOfBackBuffers;

	D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	queryHeapDesc.Count = GPU_TIMER_MAX_VALUES_PER_FRAME * numberOfBackBuffers;
	queryHeapDesc.NodeMask = 0;
	HRESULT hr = device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&gpuTimer->queryHeap));
	if (FAILED(hr))
	{
		return hresultToFfxCacaoStatus(hr);
	}

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint64_t) * numberOfBackBuffers * GPU_TIMER_MAX_VALUES_PER_FRAME),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&gpuTimer->buffer));
	if (FAILED(hr))
	{
		FFX_CACAO_ASSERT(gpuTimer->queryHeap);
		gpuTimer->queryHeap->Release();
		return hresultToFfxCacaoStatus(hr);
	}

	SetName(gpuTimer->buffer, "CACAO::GPUTimer::buffer");

	return FFX_CACAO_STATUS_OK;
}

static void gpuTimerDestroy(GpuTimer* gpuTimer)
{
	FFX_CACAO_ASSERT(gpuTimer->buffer);
	FFX_CACAO_ASSERT(gpuTimer->queryHeap);
	gpuTimer->buffer->Release();
	gpuTimer->queryHeap->Release();
}

static void gpuTimerStartFrame(GpuTimer* gpuTimer)
{
	gpuTimer->currentMeasurement = 0;
	gpuTimer->currentFrame = (gpuTimer->currentFrame + 1) % gpuTimer->numberOfBackBuffers;
}

static void gpuTimerGetTimestamp(GpuTimer* gpuTimer, ID3D12GraphicsCommandList* commandList, const char* label)
{
	FFX_CACAO_ASSERT(gpuTimer->currentMeasurement < GPU_TIMER_MAX_VALUES_PER_FRAME);
	commandList->EndQuery(gpuTimer->queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, gpuTimer->currentFrame * GPU_TIMER_MAX_VALUES_PER_FRAME + gpuTimer->currentMeasurement);
	gpuTimer->labels[gpuTimer->currentMeasurement++] = label;
}

static void gpuTimerEndFrame(GpuTimer* gpuTimer, ID3D12GraphicsCommandList* commandList)
{
	commandList->ResolveQueryData(
		gpuTimer->queryHeap,
		D3D12_QUERY_TYPE_TIMESTAMP,
		gpuTimer->currentFrame * GPU_TIMER_MAX_VALUES_PER_FRAME,
		gpuTimer->currentMeasurement, gpuTimer->buffer,
		gpuTimer->currentFrame * GPU_TIMER_MAX_VALUES_PER_FRAME * sizeof(uint64_t));
}

static void gpuTimerCollectTimings(GpuTimer* gpuTimer, FfxCacaoDetailedTiming* timings)
{
	uint32_t numMeasurements = gpuTimer->currentMeasurement;
	uint32_t frame = gpuTimer->currentFrame;
	uint32_t start = GPU_TIMER_MAX_VALUES_PER_FRAME * frame;
	uint32_t end = GPU_TIMER_MAX_VALUES_PER_FRAME * (frame + 1);

	D3D12_RANGE readRange;
	readRange.Begin = start * sizeof(uint64_t);
	readRange.End = end * sizeof(uint64_t);
	uint64_t *timingsInTicks = NULL;
	gpuTimer->buffer->Map(0, &readRange, reinterpret_cast<void**>(&timingsInTicks));

	timings->numTimestamps = numMeasurements;

	uint64_t prevTimeTicks = timingsInTicks[start];
	for (uint32_t i = 1; i < numMeasurements; i++)
	{
		uint64_t curTimeTicks = timingsInTicks[start + i];
		FfxCacaoTimestamp *t = &timings->timestamps[i];
		t->label = gpuTimer->labels[i];
		t->ticks = curTimeTicks - prevTimeTicks;
		prevTimeTicks = curTimeTicks;
	}

	timings->timestamps[0].label = "total";
	timings->timestamps[0].ticks = prevTimeTicks - timingsInTicks[start];
}
#endif

// =================================================================================================
// CbvSrvUav implementation
// =================================================================================================

typedef struct CbvSrvUav {
	uint32_t                    size;
	uint32_t                    descriptorSize;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuVisibleCpuDescriptor;
} CbvSrvUav;

static D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvUavGetCpu(CbvSrvUav* cbvSrvUav, uint32_t i)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = cbvSrvUav->cpuDescriptor;
	cpuDescriptor.ptr += i * cbvSrvUav->descriptorSize;
	return cpuDescriptor;
}

static D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvUavGetCpuVisibleCpu(CbvSrvUav* cbvSrvUav, uint32_t i)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = cbvSrvUav->cpuVisibleCpuDescriptor;
	cpuDescriptor.ptr += i * cbvSrvUav->descriptorSize;
	return cpuDescriptor;
}

static D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvUavGetGpu(CbvSrvUav* cbvSrvUav, uint32_t i)
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor = cbvSrvUav->gpuDescriptor;
	gpuDescriptor.ptr += i * cbvSrvUav->descriptorSize;
	return gpuDescriptor;
}

// =================================================================================================
// CbvSrvUavHeap implementation
// =================================================================================================

typedef struct CbvSrvUavHeap {
	uint32_t              index;
	uint32_t              descriptorCount;
	uint32_t              descriptorElementSize;
	ID3D12DescriptorHeap *heap;
	ID3D12DescriptorHeap *cpuVisibleHeap;
} ResourceViewHeap;

static FfxCacaoStatus cbvSrvUavHeapInit(CbvSrvUavHeap* cbvSrvUavHeap, ID3D12Device* device, uint32_t descriptorCount)
{
	FFX_CACAO_ASSERT(cbvSrvUavHeap);
	FFX_CACAO_ASSERT(device);

	cbvSrvUavHeap->descriptorCount = descriptorCount;
	cbvSrvUavHeap->index = 0;

	cbvSrvUavHeap->descriptorElementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC descHeap;
	descHeap.NumDescriptors = descriptorCount;
	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeap.NodeMask = 0;

	HRESULT hr = device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&cbvSrvUavHeap->heap));
	if (FAILED(hr))
	{
		return hresultToFfxCacaoStatus(hr);
	}

	SetName(cbvSrvUavHeap->heap, "FfxCacaoCbvSrvUavHeap");

	D3D12_DESCRIPTOR_HEAP_DESC cpuVisibleDescHeap;
	cpuVisibleDescHeap.NumDescriptors = descriptorCount;
	cpuVisibleDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cpuVisibleDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	cpuVisibleDescHeap.NodeMask = 0;

	hr = device->CreateDescriptorHeap(&cpuVisibleDescHeap, IID_PPV_ARGS(&cbvSrvUavHeap->cpuVisibleHeap));
	if (FAILED(hr))
	{
		FFX_CACAO_ASSERT(cbvSrvUavHeap->heap);
		cbvSrvUavHeap->heap->Release();
		return hresultToFfxCacaoStatus(hr);
	}

	SetName(cbvSrvUavHeap->cpuVisibleHeap, "FfxCacaoCbvSrvUavCpuVisibleHeap");
	return FFX_CACAO_STATUS_OK;
}

static void cbvSrvUavHeapDestroy(CbvSrvUavHeap* cbvSrvUavHeap)
{
	FFX_CACAO_ASSERT(cbvSrvUavHeap);
	FFX_CACAO_ASSERT(cbvSrvUavHeap->heap);
	FFX_CACAO_ASSERT(cbvSrvUavHeap->cpuVisibleHeap);
	cbvSrvUavHeap->heap->Release();
	cbvSrvUavHeap->cpuVisibleHeap->Release();
}

static void cbvSrvUavHeapAllocDescriptor(CbvSrvUavHeap* cbvSrvUavHeap, CbvSrvUav* cbvSrvUav, uint32_t size)
{
	FFX_CACAO_ASSERT(cbvSrvUavHeap);
	FFX_CACAO_ASSERT(cbvSrvUav);
	FFX_CACAO_ASSERT(cbvSrvUavHeap->index + size <= cbvSrvUavHeap->descriptorCount);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuView = cbvSrvUavHeap->heap->GetCPUDescriptorHandleForHeapStart();
	cpuView.ptr += cbvSrvUavHeap->index * cbvSrvUavHeap->descriptorElementSize;

	D3D12_GPU_DESCRIPTOR_HANDLE gpuView = cbvSrvUavHeap->heap->GetGPUDescriptorHandleForHeapStart();
	gpuView.ptr += cbvSrvUavHeap->index * cbvSrvUavHeap->descriptorElementSize;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuVisibleCpuView = cbvSrvUavHeap->cpuVisibleHeap->GetCPUDescriptorHandleForHeapStart();
	cpuVisibleCpuView.ptr += cbvSrvUavHeap->index * cbvSrvUavHeap->descriptorElementSize;

	cbvSrvUavHeap->index += size;

	cbvSrvUav->size = size;
	cbvSrvUav->descriptorSize = cbvSrvUavHeap->descriptorElementSize;
	cbvSrvUav->cpuDescriptor = cpuView;
	cbvSrvUav->gpuDescriptor = gpuView;
	cbvSrvUav->cpuVisibleCpuDescriptor = cpuVisibleCpuView;
}

// =================================================================================================
// ConstantBufferRing implementation
// =================================================================================================

typedef struct ConstantBufferRing {
	size_t             pageSize;
	size_t             totalSize;
	size_t             currentOffset;
	uint32_t           currentPage;
	uint32_t           numPages;
	char              *data;
	ID3D12Resource    *buffer;
} ConstantBufferRing;

static FfxCacaoStatus constantBufferRingInit(ConstantBufferRing* constantBufferRing, ID3D12Device* device, uint32_t numPages, size_t pageSize)
{
	FFX_CACAO_ASSERT(constantBufferRing);
	FFX_CACAO_ASSERT(device);

	pageSize = AlignOffset(pageSize, 256);
	size_t totalSize = numPages * pageSize;
	char *data = NULL;
	ID3D12Resource *buffer = NULL;

	HRESULT hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(totalSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer));
	if (FAILED(hr))
	{
		return hresultToFfxCacaoStatus(hr);
	}

	SetName(buffer, "DynamicBufferRing::m_pBuffer");

	buffer->Map(0, NULL, (void**)&data);

	constantBufferRing->pageSize = pageSize;
	constantBufferRing->totalSize = totalSize;
	constantBufferRing->currentOffset = 0;
	constantBufferRing->currentPage = 0;
	constantBufferRing->numPages = numPages;
	constantBufferRing->data = data;
	constantBufferRing->buffer = buffer;

	return FFX_CACAO_STATUS_OK;
}

static void constantBufferRingDestroy(ConstantBufferRing* constantBufferRing)
{
	FFX_CACAO_ASSERT(constantBufferRing);
	FFX_CACAO_ASSERT(constantBufferRing->buffer);
	constantBufferRing->buffer->Release();
}

static void constantBufferRingStartFrame(ConstantBufferRing* constantBufferRing)
{
	FFX_CACAO_ASSERT(constantBufferRing);
	constantBufferRing->currentPage = (constantBufferRing->currentPage + 1) % constantBufferRing->numPages;
	constantBufferRing->currentOffset = 0;
}

static void constantBufferRingAlloc(ConstantBufferRing* constantBufferRing, size_t size, void **data, D3D12_GPU_VIRTUAL_ADDRESS *bufferViewDesc)
{
	FFX_CACAO_ASSERT(constantBufferRing);
	size = AlignOffset(size, 256);
	FFX_CACAO_ASSERT(constantBufferRing->currentOffset + size <= constantBufferRing->pageSize);

	size_t memOffset = constantBufferRing->pageSize * constantBufferRing->currentPage + constantBufferRing->currentOffset;
	*data = constantBufferRing->data + memOffset;
	constantBufferRing->currentOffset += size;

	*bufferViewDesc = constantBufferRing->buffer->GetGPUVirtualAddress() + memOffset;
}

// =================================================================================================
// ComputeShader implementation
// =================================================================================================

typedef struct ComputeShader {
	ID3D12RootSignature  *rootSignature;
	ID3D12PipelineState  *pipelineState;
} ComputeShader;

static FfxCacaoStatus computeShaderInit(ComputeShader* computeShader, ID3D12Device* device, const char* name, const void* bytecode, size_t bytecodeLength, uint32_t uavTableSize, uint32_t srvTableSize, D3D12_STATIC_SAMPLER_DESC* staticSamplers, uint32_t numStaticSamplers)
{
	FFX_CACAO_ASSERT(computeShader);
	FFX_CACAO_ASSERT(device);
	FFX_CACAO_ASSERT(name);
	FFX_CACAO_ASSERT(bytecode);
	FFX_CACAO_ASSERT(staticSamplers);

	D3D12_SHADER_BYTECODE shaderByteCode = {};
	shaderByteCode.pShaderBytecode = bytecode;
	shaderByteCode.BytecodeLength = bytecodeLength;

	// Create root signature
	{
		CD3DX12_DESCRIPTOR_RANGE DescRange[4];
		CD3DX12_ROOT_PARAMETER RTSlot[4];

		// we'll always have a constant buffer
		int parameterCount = 0;
		DescRange[parameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		RTSlot[parameterCount++].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

		// if we have a UAV table
		if (uavTableSize > 0)
		{
			DescRange[parameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, uavTableSize, 0);
			RTSlot[parameterCount].InitAsDescriptorTable(1, &DescRange[parameterCount], D3D12_SHADER_VISIBILITY_ALL);
			++parameterCount;
		}

		// if we have a SRV table
		if (srvTableSize > 0)
		{
			DescRange[parameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvTableSize, 0);
			RTSlot[parameterCount].InitAsDescriptorTable(1, &DescRange[parameterCount], D3D12_SHADER_VISIBILITY_ALL);
			++parameterCount;
		}

		// the root signature contains 3 slots to be used
		CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
		descRootSignature.NumParameters = parameterCount;
		descRootSignature.pParameters = RTSlot;
		descRootSignature.NumStaticSamplers = numStaticSamplers;
		descRootSignature.pStaticSamplers = staticSamplers;

		// deny uneccessary access to certain pipeline stages
		descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		ID3DBlob *outBlob, *errorBlob = NULL;

		HRESULT hr = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &outBlob, &errorBlob);
		if (FAILED(hr))
		{
			return hresultToFfxCacaoStatus(hr);
		}

		if (errorBlob)
		{
			errorBlob->Release();
			if (outBlob)
			{
				outBlob->Release();
			}
			return FFX_CACAO_STATUS_FAILED;
		}

		hr = device->CreateRootSignature(0, outBlob->GetBufferPointer(), outBlob->GetBufferSize(), IID_PPV_ARGS(&computeShader->rootSignature));
		if (FAILED(hr))
		{
			outBlob->Release();
			return hresultToFfxCacaoStatus(hr);
		}

		char nameBuffer[1024] = "PostProcCS::m_pRootSignature::";
		strncat_s(nameBuffer, name, FFX_CACAO_ARRAY_SIZE(nameBuffer));
		SetName(computeShader->rootSignature, nameBuffer);

		outBlob->Release();
	}

	// Create pipeline state
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC descPso = {};
		descPso.CS = shaderByteCode;
		descPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		descPso.pRootSignature = computeShader->rootSignature;
		descPso.NodeMask = 0;

		HRESULT hr = device->CreateComputePipelineState(&descPso, IID_PPV_ARGS(&computeShader->pipelineState));
		if (FAILED(hr))
		{
			computeShader->rootSignature->Release();
			return hresultToFfxCacaoStatus(hr);
		}

		char nameBuffer[1024] = "PostProcCS::m_pPipeline::";
		strncat_s(nameBuffer, name, FFX_CACAO_ARRAY_SIZE(nameBuffer));
		SetName(computeShader->rootSignature, nameBuffer);
	}

	return FFX_CACAO_STATUS_OK;
}

static void computeShaderDestroy(ComputeShader* computeShader)
{
	FFX_CACAO_ASSERT(computeShader);
	FFX_CACAO_ASSERT(computeShader->rootSignature);
	FFX_CACAO_ASSERT(computeShader->pipelineState);
	computeShader->rootSignature->Release();
	computeShader->pipelineState->Release();
}

static void computeShaderDraw(ComputeShader* computeShader, ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS constantBuffer, CbvSrvUav *uavTable, CbvSrvUav *srvTable, uint32_t width, uint32_t height, uint32_t depth)
{
	FFX_CACAO_ASSERT(computeShader);
	FFX_CACAO_ASSERT(commandList);
	FFX_CACAO_ASSERT(uavTable);
	FFX_CACAO_ASSERT(srvTable);
	FFX_CACAO_ASSERT(computeShader->pipelineState);
	FFX_CACAO_ASSERT(computeShader->rootSignature);

	commandList->SetComputeRootSignature(computeShader->rootSignature);

	int params = 0;
	commandList->SetComputeRootConstantBufferView(params++, constantBuffer);
	if (uavTable)
	{
		commandList->SetComputeRootDescriptorTable(params++, uavTable->gpuDescriptor);
	}
	if (srvTable)
	{
		commandList->SetComputeRootDescriptorTable(params++, srvTable->gpuDescriptor);
	}

	commandList->SetPipelineState(computeShader->pipelineState);
	commandList->Dispatch(width, height, depth);
}

// =================================================================================================
// Texture implementation
// =================================================================================================

typedef struct Texture {
	ID3D12Resource *resource;
	DXGI_FORMAT     format;
	uint32_t        width;
	uint32_t        height;
	uint32_t        arraySize;
	uint32_t        mipMapCount;
} Texture;

static FfxCacaoStatus textureInit(Texture* texture, ID3D12Device* device, const char* name, const CD3DX12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
	FFX_CACAO_ASSERT(texture);
	FFX_CACAO_ASSERT(device);
	FFX_CACAO_ASSERT(name);
	FFX_CACAO_ASSERT(desc);

	HRESULT hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		desc,
		initialState,
		clearValue,
		IID_PPV_ARGS(&texture->resource));
	if (FAILED(hr))
	{
		return hresultToFfxCacaoStatus(hr);
	}

	texture->format = desc->Format;
	texture->width = (uint32_t)desc->Width;
	texture->height = desc->Height;
	texture->arraySize = desc->DepthOrArraySize;
	texture->mipMapCount = desc->MipLevels;

	SetName(texture->resource, name);

	return FFX_CACAO_STATUS_OK;
}

static void textureDestroy(Texture* texture)
{
	FFX_CACAO_ASSERT(texture);
	FFX_CACAO_ASSERT(texture->resource);
	texture->resource->Release();
}

static void textureCreateSrvFromDesc(Texture* texture, uint32_t index, CbvSrvUav* srv, const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc)
{
	FFX_CACAO_ASSERT(texture);
	FFX_CACAO_ASSERT(srv);
	FFX_CACAO_ASSERT(srvDesc);

	ID3D12Device* device;
	texture->resource->GetDevice(__uuidof(*device), (void**)&device);

	device->CreateShaderResourceView(texture->resource, srvDesc, cbvSrvUavGetCpu(srv, index));
	device->CreateShaderResourceView(texture->resource, srvDesc, cbvSrvUavGetCpuVisibleCpu(srv, index));

	device->Release();
}

static void textureCreateSrv(Texture* texture, uint32_t index, CbvSrvUav* srv, int mipLevel, int arraySize, int firstArraySlice)
{
	FFX_CACAO_ASSERT(texture);
	FFX_CACAO_ASSERT(srv);

	D3D12_RESOURCE_DESC resourceDesc = texture->resource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = resourceDesc.Format == DXGI_FORMAT_D32_FLOAT ? DXGI_FORMAT_R32_FLOAT : resourceDesc.Format;
	if (resourceDesc.SampleDesc.Count == 1)
	{
		if (resourceDesc.DepthOrArraySize == 1)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = (mipLevel == -1) ? 0 : mipLevel;
			srvDesc.Texture2D.MipLevels = (mipLevel == -1) ? texture->mipMapCount : 1;
			FFX_CACAO_ASSERT(arraySize == -1);
			FFX_CACAO_ASSERT(firstArraySlice == -1);
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MostDetailedMip = (mipLevel == -1) ? 0 : mipLevel;
			srvDesc.Texture2DArray.MipLevels = (mipLevel == -1) ? texture->mipMapCount : 1;
			srvDesc.Texture2DArray.FirstArraySlice = (firstArraySlice == -1) ? 0 : firstArraySlice;
			srvDesc.Texture2DArray.ArraySize = (arraySize == -1) ? resourceDesc.DepthOrArraySize : arraySize;
		}
	}
	else
	{
		if (resourceDesc.DepthOrArraySize == 1)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			FFX_CACAO_ASSERT(mipLevel == -1);
			FFX_CACAO_ASSERT(arraySize == -1);
			FFX_CACAO_ASSERT(firstArraySlice == -1);
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
			srvDesc.Texture2DMSArray.FirstArraySlice = (firstArraySlice == -1) ? 0 : firstArraySlice;
			srvDesc.Texture2DMSArray.ArraySize = (arraySize == -1) ? resourceDesc.DepthOrArraySize : arraySize;
			FFX_CACAO_ASSERT(mipLevel == -1);
		}
	}
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	textureCreateSrvFromDesc(texture, index, srv, &srvDesc);
}

static void textureCreateUavFromDesc(Texture* texture, uint32_t index, CbvSrvUav* uav, const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc)
{
	FFX_CACAO_ASSERT(texture);
	FFX_CACAO_ASSERT(uav);
	FFX_CACAO_ASSERT(uavDesc);

	ID3D12Device* device;
	texture->resource->GetDevice(__uuidof(*device), (void**)&device);

	device->CreateUnorderedAccessView(texture->resource, NULL, uavDesc, cbvSrvUavGetCpu(uav, index));
	device->CreateUnorderedAccessView(texture->resource, NULL, uavDesc, cbvSrvUavGetCpuVisibleCpu(uav, index));

	device->Release();
}

static void textureCreateUav(Texture* texture, uint32_t index, CbvSrvUav* uav, int mipLevel, int arraySize, int firstArraySlice)
{
	FFX_CACAO_ASSERT(texture);
	FFX_CACAO_ASSERT(uav);

	D3D12_RESOURCE_DESC resourceDesc = texture->resource->GetDesc();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = resourceDesc.Format;
	if (arraySize == -1)
	{
		FFX_CACAO_ASSERT(firstArraySlice == -1);
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = (mipLevel == -1) ? 0 : mipLevel;
	}
	else
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = arraySize;
		uavDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		uavDesc.Texture2DArray.MipSlice = (mipLevel == -1) ? 0 : mipLevel;
	}

	textureCreateUavFromDesc(texture, index, uav, &uavDesc);
}

// =================================================================================================
// CACAO implementation
// =================================================================================================

struct FfxCacaoD3D12Context {
	FfxCacaoSettings   settings;
	FfxCacaoBool       useDownsampledSsao;

	ID3D12Device      *device;
	CbvSrvUavHeap      cbvSrvUavHeap;

#if FFX_CACAO_ENABLE_PROFILING
	GpuTimer           gpuTimer;
#endif

	ConstantBufferRing constantBufferRing;
	BufferSizeInfo     bufferSizeInfo;
	ID3D12Resource    *outputResource;

	// ==========================================
	// Prepare shaders/resources

	ComputeShader prepareDownsampledDepthsAndMips;
	ComputeShader prepareNativeDepthsAndMips;

	ComputeShader prepareDownsampledNormals;
	ComputeShader prepareNativeNormals;

	ComputeShader prepareDownsampledNormalsFromInputNormals;
	ComputeShader prepareNativeNormalsFromInputNormals;

	ComputeShader prepareDownsampledDepths;
	ComputeShader prepareNativeDepths;

	ComputeShader prepareDownsampledDepthsHalf;
	ComputeShader prepareNativeDepthsHalf;

	CbvSrvUav prepareDepthsNormalsAndMipsInputs; // <-- this is just the depth source
	CbvSrvUav prepareDepthsAndMipsOutputs;
	CbvSrvUav prepareDepthsOutputs;
	CbvSrvUav prepareNormalsOutput;
	CbvSrvUav prepareNormalsFromInputNormalsInput;
	CbvSrvUav prepareNormalsFromInputNormalsOutput;

	// ==========================================
	// Generate SSAO shaders/resources

	ComputeShader generateSSAO[5];

	CbvSrvUav generateSSAOInputs[4];
	CbvSrvUav generateAdaptiveSSAOInputs[4];
	CbvSrvUav generateSSAOOutputsFinal[4];
	CbvSrvUav generateSSAOOutputsPing[4];

	// ==========================================
	// Importance map generate/post process shaders/resources

	ComputeShader generateImportanceMap;
	ComputeShader postprocessImportanceMapA;
	ComputeShader postprocessImportanceMapB;

	CbvSrvUav generateImportanceMapInputs;
	CbvSrvUav generateImportanceMapOutputs;
	CbvSrvUav generateImportanceMapAInputs;
	CbvSrvUav generateImportanceMapAOutputs;
	CbvSrvUav generateImportanceMapBInputs;
	CbvSrvUav generateImportanceMapBOutputs;

	// ==========================================
	// De-interleave Blur shaders/resources

	ComputeShader edgeSensitiveBlur[8];

	CbvSrvUav pingPongHalfResultASRV[4];
	CbvSrvUav finalResultsArrayUAV[4];

	// ==========================================
	// Apply shaders/resources

	ComputeShader smartApply;
	ComputeShader nonSmartApply;
	ComputeShader nonSmartHalfApply;

	CbvSrvUav createOutputInputs;
	CbvSrvUav createOutputOutputs;

	// ==========================================
	// upscale shaders/resources

	ComputeShader upscaleBilateral5x5;
	ComputeShader upscaleBilateral5x5Half;

	CbvSrvUav bilateralUpscaleInputs;
	CbvSrvUav bilateralUpscaleOutputs;

	// ==========================================
	// Intermediate buffers

	Texture halfDepthsArray;
	Texture pingPongHalfResultA[4];
	Texture finalResults;
	Texture deinterlacedNormals;
	Texture importanceMap;
	Texture importanceMapPong;
	Texture loadCounter;

	CbvSrvUav loadCounterUav; // required for LoadCounter clear
};

static inline FfxCacaoD3D12Context* getAlignedD3D12ContextPointer(FfxCacaoD3D12Context* ptr)
{
	uintptr_t tmp = (uintptr_t)ptr;
	tmp = (tmp + alignof(FfxCacaoD3D12Context) - 1) & (~(alignof(FfxCacaoD3D12Context) - 1));
	return (FfxCacaoD3D12Context*)tmp;
}
#endif

// =================================================================================
// Interface
// =================================================================================

#ifdef __cplusplus
extern "C"
{
#endif

#if FFX_CACAO_ENABLE_D3D12
size_t ffxCacaoD3D12GetContextSize()
{
	return sizeof(FfxCacaoD3D12Context) + alignof(FfxCacaoD3D12Context) - 1;
}

FfxCacaoStatus ffxCacaoD3D12InitContext(FfxCacaoD3D12Context* context, ID3D12Device* device)
{
	if (context == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	if (device == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	context = getAlignedD3D12ContextPointer(context);

#define COMPUTE_SHADER_INIT(name, entryPoint, uavSize, srvSize) \
	errorStatus = computeShaderInit(&context->name, device, #entryPoint, entryPoint ## DXIL, sizeof(entryPoint ## DXIL), uavSize, srvSize, samplers, FFX_CACAO_ARRAY_SIZE(samplers)); \
	if (errorStatus) \
	{ \
		goto error_create_ ## entryPoint; \
	}
#define ERROR_COMPUTE_SHADER_DESTROY(name, entryPoint) \
	computeShaderDestroy(&context->name); \
error_create_ ## entryPoint:

	FfxCacaoStatus errorStatus = FFX_CACAO_STATUS_FAILED;

	context->device = device;
	CbvSrvUavHeap *cbvSrvUavHeap = &context->cbvSrvUavHeap;
	errorStatus = cbvSrvUavHeapInit(cbvSrvUavHeap, device, 256);
	if (errorStatus)
	{
		goto error_create_cbv_srv_uav_heap;
	}
	errorStatus = constantBufferRingInit(&context->constantBufferRing, device, 5, 1024 * 5);
	if (errorStatus)
	{
		goto error_create_constant_buffer_ring;
	}
#if FFX_CACAO_ENABLE_PROFILING
	errorStatus = gpuTimerInit(&context->gpuTimer, device, 5);
	if (errorStatus)
	{
		goto error_create_gpu_timer;
	}
#endif

	D3D12_STATIC_SAMPLER_DESC samplers[5] = { };

	samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[0].MinLOD = 0.0f;
	samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[0].MipLODBias = 0;
	samplers[0].MaxAnisotropy = 1;
	samplers[0].ShaderRegister = 0;
	samplers[0].RegisterSpace = 0;
	samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[1].MinLOD = 0.0f;
	samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[1].MipLODBias = 0;
	samplers[1].MaxAnisotropy = 1;
	samplers[1].ShaderRegister = 1;
	samplers[1].RegisterSpace = 0;
	samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	samplers[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplers[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[2].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplers[2].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[2].MinLOD = 0.0f;
	samplers[2].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[2].MipLODBias = 0;
	samplers[2].MaxAnisotropy = 1;
	samplers[2].ShaderRegister = 2;
	samplers[2].RegisterSpace = 0;
	samplers[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	samplers[3].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplers[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[3].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplers[3].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[3].MinLOD = 0.0f;
	samplers[3].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[3].MipLODBias = 0;
	samplers[3].MaxAnisotropy = 1;
	samplers[3].ShaderRegister = 3;
	samplers[3].RegisterSpace = 0;
	samplers[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	samplers[4].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplers[4].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[4].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[4].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[4].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplers[4].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[4].MinLOD = 0.0f;
	samplers[4].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[4].MipLODBias = 0;
	samplers[4].MaxAnisotropy = 1;
	samplers[4].ShaderRegister = 4;
	samplers[4].RegisterSpace = 0;
	samplers[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// =====================================
	// Prepare shaders/resources

	COMPUTE_SHADER_INIT(prepareDownsampledDepthsHalf, CSPrepareDownsampledDepthsHalf, 1, 1);
	COMPUTE_SHADER_INIT(prepareNativeDepthsHalf, CSPrepareNativeDepthsHalf, 1, 1);

	COMPUTE_SHADER_INIT(prepareDownsampledDepthsAndMips, CSPrepareDownsampledDepthsAndMips, 4, 1);
	COMPUTE_SHADER_INIT(prepareNativeDepthsAndMips, CSPrepareNativeDepthsAndMips, 4, 1);

	COMPUTE_SHADER_INIT(prepareDownsampledNormals, CSPrepareDownsampledNormals, 1, 1);
	COMPUTE_SHADER_INIT(prepareNativeNormals, CSPrepareNativeNormals, 1, 1);

	COMPUTE_SHADER_INIT(prepareDownsampledNormalsFromInputNormals, CSPrepareDownsampledNormalsFromInputNormals, 1, 1);
	COMPUTE_SHADER_INIT(prepareNativeNormalsFromInputNormals, CSPrepareNativeNormalsFromInputNormals, 1, 1);

	COMPUTE_SHADER_INIT(prepareDownsampledDepths, CSPrepareDownsampledDepths, 1, 1);
	COMPUTE_SHADER_INIT(prepareNativeDepths, CSPrepareNativeDepths, 1, 1);

	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->prepareDepthsAndMipsOutputs, 4);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->prepareDepthsOutputs, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->prepareDepthsNormalsAndMipsInputs, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->prepareNormalsOutput, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->prepareNormalsFromInputNormalsInput, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->prepareNormalsFromInputNormalsOutput, 1);

	// =====================================
	// Generate SSAO shaders/resources

	COMPUTE_SHADER_INIT(generateSSAO[0], CSGenerateQ0, 1, 7);
	COMPUTE_SHADER_INIT(generateSSAO[1], CSGenerateQ1, 1, 7);
	COMPUTE_SHADER_INIT(generateSSAO[2], CSGenerateQ2, 1, 7);
	COMPUTE_SHADER_INIT(generateSSAO[3], CSGenerateQ3, 1, 7);
	COMPUTE_SHADER_INIT(generateSSAO[4], CSGenerateQ3Base, 2, 7);

	for (int i = 0; i < 4; ++i)
	{
		cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateSSAOInputs[i], 7);
		cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateAdaptiveSSAOInputs[i], 7);

		cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateSSAOOutputsFinal[i], 1);
		cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateSSAOOutputsPing[i], 1);
	}

	// =====================================
	// Importance map shaders/resources

	COMPUTE_SHADER_INIT(generateImportanceMap, CSGenerateImportanceMap, 1, 1);
	COMPUTE_SHADER_INIT(postprocessImportanceMapA, CSPostprocessImportanceMapA, 1, 1);
	COMPUTE_SHADER_INIT(postprocessImportanceMapB, CSPostprocessImportanceMapB, 2, 1);

	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateImportanceMapInputs, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateImportanceMapOutputs, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateImportanceMapAInputs, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateImportanceMapAOutputs, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateImportanceMapBInputs, 1);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->generateImportanceMapBOutputs, 2);

	// =====================================
	// De-interleave Blur shaders/resources

	COMPUTE_SHADER_INIT(edgeSensitiveBlur[0], CSEdgeSensitiveBlur1, 1, 1);
	COMPUTE_SHADER_INIT(edgeSensitiveBlur[1], CSEdgeSensitiveBlur2, 1, 1);
	COMPUTE_SHADER_INIT(edgeSensitiveBlur[2], CSEdgeSensitiveBlur3, 1, 1);
	COMPUTE_SHADER_INIT(edgeSensitiveBlur[3], CSEdgeSensitiveBlur4, 1, 1);
	COMPUTE_SHADER_INIT(edgeSensitiveBlur[4], CSEdgeSensitiveBlur5, 1, 1);
	COMPUTE_SHADER_INIT(edgeSensitiveBlur[5], CSEdgeSensitiveBlur6, 1, 1);
	COMPUTE_SHADER_INIT(edgeSensitiveBlur[6], CSEdgeSensitiveBlur7, 1, 1);
	COMPUTE_SHADER_INIT(edgeSensitiveBlur[7], CSEdgeSensitiveBlur8, 1, 1);

	for (int i = 0; i < 4; ++i)
	{
		cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->finalResultsArrayUAV[i], 1);
		cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->pingPongHalfResultASRV[i], 1);
	}

	// =====================================
	// Apply shaders/resources

	COMPUTE_SHADER_INIT(smartApply, CSApply, 1, 1);
	COMPUTE_SHADER_INIT(nonSmartApply, CSNonSmartApply, 1, 1);
	COMPUTE_SHADER_INIT(nonSmartHalfApply, CSNonSmartHalfApply, 1, 1);

	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->createOutputInputs, 4);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->createOutputOutputs, 1);

	// =====================================
	// Upacale shaders/resources

	COMPUTE_SHADER_INIT(upscaleBilateral5x5, CSUpscaleBilateral5x5, 1, 4);
	COMPUTE_SHADER_INIT(upscaleBilateral5x5Half, CSUpscaleBilateral5x5Half, 1, 4);

	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->bilateralUpscaleInputs, 4);
	cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->bilateralUpscaleOutputs, 1);

	// =====================================
	// Misc

	errorStatus = textureInit(&context->loadCounter, device, "CACAO::m_loadCounter", &CD3DX12_RESOURCE_DESC::Tex1D(DXGI_FORMAT_R32_UINT, 1, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL);
	if (errorStatus)
	{
		goto error_create_load_counter_texture;
	}

	// create uav for load counter
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_UINT;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uavDesc.Texture1D.MipSlice = 0;

		cbvSrvUavHeapAllocDescriptor(cbvSrvUavHeap, &context->loadCounterUav, 1); // required for clearing the load counter
		textureCreateUavFromDesc(&context->loadCounter, 0, &context->loadCounterUav, &uavDesc);
	}

	return FFX_CACAO_STATUS_OK;

error_create_load_counter_texture:

	ERROR_COMPUTE_SHADER_DESTROY(upscaleBilateral5x5Half, CSUpscaleBilateral5x5Half);
	ERROR_COMPUTE_SHADER_DESTROY(upscaleBilateral5x5, CSUpscaleBilateral5x5);

	ERROR_COMPUTE_SHADER_DESTROY(nonSmartHalfApply, CSNonSmartHalfApply);
	ERROR_COMPUTE_SHADER_DESTROY(nonSmartApply, CSNonSmartApply);
	ERROR_COMPUTE_SHADER_DESTROY(smartApply, CSApply);

	ERROR_COMPUTE_SHADER_DESTROY(edgeSensitiveBlur[7], CSEdgeSensitiveBlur8);
	ERROR_COMPUTE_SHADER_DESTROY(edgeSensitiveBlur[6], CSEdgeSensitiveBlur7);
	ERROR_COMPUTE_SHADER_DESTROY(edgeSensitiveBlur[5], CSEdgeSensitiveBlur6);
	ERROR_COMPUTE_SHADER_DESTROY(edgeSensitiveBlur[4], CSEdgeSensitiveBlur5);
	ERROR_COMPUTE_SHADER_DESTROY(edgeSensitiveBlur[3], CSEdgeSensitiveBlur4);
	ERROR_COMPUTE_SHADER_DESTROY(edgeSensitiveBlur[2], CSEdgeSensitiveBlur3);
	ERROR_COMPUTE_SHADER_DESTROY(edgeSensitiveBlur[1], CSEdgeSensitiveBlur2);
	ERROR_COMPUTE_SHADER_DESTROY(edgeSensitiveBlur[0], CSEdgeSensitiveBlur1);

	ERROR_COMPUTE_SHADER_DESTROY(postprocessImportanceMapB, CSPostprocessImportanceMapB);
	ERROR_COMPUTE_SHADER_DESTROY(postprocessImportanceMapA, CSPostprocessImportanceMapA);
	ERROR_COMPUTE_SHADER_DESTROY(generateImportanceMap, CSGenerateImportanceMap);

	ERROR_COMPUTE_SHADER_DESTROY(generateSSAO[4], CSGenerateQ3Base);
	ERROR_COMPUTE_SHADER_DESTROY(generateSSAO[3], CSGenerateQ3);
	ERROR_COMPUTE_SHADER_DESTROY(generateSSAO[2], CSGenerateQ2);
	ERROR_COMPUTE_SHADER_DESTROY(generateSSAO[1], CSGenerateQ1);
	ERROR_COMPUTE_SHADER_DESTROY(generateSSAO[0], CSGenerateQ0);

	ERROR_COMPUTE_SHADER_DESTROY(prepareNativeDepths, CSPrepareNativeDepths);
	ERROR_COMPUTE_SHADER_DESTROY(prepareDownsampledDepths, CSPrepareDownsampledDepths);

	ERROR_COMPUTE_SHADER_DESTROY(prepareNativeNormalsFromInputNormals, CSPrepareNativeNormalsFromInputNormals);
	ERROR_COMPUTE_SHADER_DESTROY(prepareDownsampledNormalsFromInputNormals, CSPrepareDownsampledNormalsFromInputNormals);

	ERROR_COMPUTE_SHADER_DESTROY(prepareNativeNormals, CSPrepareNativeNormals);
	ERROR_COMPUTE_SHADER_DESTROY(prepareDownsampledNormals, CSPrepareDownsampledNormals);

	ERROR_COMPUTE_SHADER_DESTROY(prepareNativeDepthsAndMips, CSPrepareNativeDepthsAndMips);
	ERROR_COMPUTE_SHADER_DESTROY(prepareDownsampledDepthsAndMips, CSPrepareDownsampledDepthsAndMips);

	ERROR_COMPUTE_SHADER_DESTROY(prepareNativeDepthsHalf, CSPrepareNativeDepthsHalf);
	ERROR_COMPUTE_SHADER_DESTROY(prepareDownsampledDepthsHalf, CSPrepareDownsampledDepthsHalf);

#if FFX_CACAO_ENABLE_PROFILING
	gpuTimerDestroy(&context->gpuTimer);
error_create_gpu_timer:
#endif
	constantBufferRingDestroy(&context->constantBufferRing);
error_create_constant_buffer_ring:
	cbvSrvUavHeapDestroy(&context->cbvSrvUavHeap);
error_create_cbv_srv_uav_heap:

	return errorStatus;
	
#undef COMPUTE_SHADER_INIT
#undef ERROR_COMPUTE_SHADER_DESTROY
}

FfxCacaoStatus ffxCacaoD3D12DestroyContext(FfxCacaoD3D12Context* context)
{
	if (context == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	context = getAlignedD3D12ContextPointer(context);

	textureDestroy(&context->loadCounter);

	computeShaderDestroy(&context->upscaleBilateral5x5Half);
	computeShaderDestroy(&context->upscaleBilateral5x5);

	computeShaderDestroy(&context->nonSmartHalfApply);
	computeShaderDestroy(&context->nonSmartApply);
	computeShaderDestroy(&context->smartApply);

	computeShaderDestroy(&context->edgeSensitiveBlur[7]);
	computeShaderDestroy(&context->edgeSensitiveBlur[6]);
	computeShaderDestroy(&context->edgeSensitiveBlur[5]);
	computeShaderDestroy(&context->edgeSensitiveBlur[4]);
	computeShaderDestroy(&context->edgeSensitiveBlur[3]);
	computeShaderDestroy(&context->edgeSensitiveBlur[2]);
	computeShaderDestroy(&context->edgeSensitiveBlur[1]);
	computeShaderDestroy(&context->edgeSensitiveBlur[0]);

	computeShaderDestroy(&context->postprocessImportanceMapB);
	computeShaderDestroy(&context->postprocessImportanceMapA);
	computeShaderDestroy(&context->generateImportanceMap);

	computeShaderDestroy(&context->generateSSAO[4]);
	computeShaderDestroy(&context->generateSSAO[3]);
	computeShaderDestroy(&context->generateSSAO[2]);
	computeShaderDestroy(&context->generateSSAO[1]);
	computeShaderDestroy(&context->generateSSAO[0]);

	computeShaderDestroy(&context->prepareNativeDepths);
	computeShaderDestroy(&context->prepareDownsampledDepths);

	computeShaderDestroy(&context->prepareNativeNormalsFromInputNormals);
	computeShaderDestroy(&context->prepareDownsampledNormalsFromInputNormals);

	computeShaderDestroy(&context->prepareNativeNormals);
	computeShaderDestroy(&context->prepareDownsampledNormals);

	computeShaderDestroy(&context->prepareNativeDepthsAndMips);
	computeShaderDestroy(&context->prepareDownsampledDepthsAndMips);

	computeShaderDestroy(&context->prepareNativeDepthsHalf);
	computeShaderDestroy(&context->prepareDownsampledDepthsHalf);

#if FFX_CACAO_ENABLE_PROFILING
	gpuTimerDestroy(&context->gpuTimer);
#endif
	constantBufferRingDestroy(&context->constantBufferRing);
	cbvSrvUavHeapDestroy(&context->cbvSrvUavHeap);

	return FFX_CACAO_STATUS_OK;
}

FfxCacaoStatus ffxCacaoD3D12InitScreenSizeDependentResources(FfxCacaoD3D12Context* context, const FfxCacaoD3D12ScreenSizeInfo* info, FfxCacaoBool useDownsampledSsao)
{
	if (context == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	if (info == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	context = getAlignedD3D12ContextPointer(context);

	context->useDownsampledSsao = useDownsampledSsao;
	FfxCacaoStatus errorStatus;

#define TEXTURE_INIT(name, label, format, width, height, arraySize, mipLevels) \
	errorStatus = textureInit(&context->name, device, "CACAO::" #name, &CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, arraySize, mipLevels, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL); \
	if (errorStatus) \
	{ \
		goto error_create_texture_ ## label;\
	}
#define ERROR_TEXTURE_DESTROY(name, label) \
	textureDestroy(&context->name); \
error_create_texture_ ## label:


	ID3D12Device * device = context->device;

	uint32_t width = info->width;
	uint32_t height = info->height;
	uint32_t halfWidth = (width + 1) / 2;
	uint32_t halfHeight = (height + 1) / 2;
	uint32_t quarterWidth = (halfWidth + 1) / 2;
	uint32_t quarterHeight = (halfHeight + 1) / 2;
	uint32_t eighthWidth = (quarterWidth + 1) / 2;
	uint32_t eighthHeight = (quarterHeight + 1) / 2;

#if 1
	uint32_t depthBufferWidth = width;
	uint32_t depthBufferHeight = height;
	uint32_t depthBufferHalfWidth = halfWidth;
	uint32_t depthBufferHalfHeight = halfHeight;
	uint32_t depthBufferQuarterWidth = quarterWidth;
	uint32_t depthBufferQuarterHeight = quarterHeight;

	uint32_t depthBufferXOffset = 0;
	uint32_t depthBufferYOffset = 0;
	uint32_t depthBufferHalfXOffset = 0;
	uint32_t depthBufferHalfYOffset = 0;
	uint32_t depthBufferQuarterXOffset = 0;
	uint32_t depthBufferQuarterYOffset = 0;
#else
	uint32_t depthBufferWidth = info->depthBufferWidth;
	uint32_t depthBufferHeight = info->depthBufferHeight;
	uint32_t depthBufferHalfWidth = (depthBufferWidth + 1) / 2;
	uint32_t depthBufferHalfHeight = (depthBufferHeight + 1) / 2;
	uint32_t depthBufferQuarterWidth = (depthBufferHalfWidth + 1) / 2;
	uint32_t depthBufferQuarterHeight = (depthBufferHalfHeight + 1) / 2;

	uint32_t depthBufferXOffset = info->depthBufferXOffset;
	uint32_t depthBufferYOffset = info->depthBufferYOffset;
	uint32_t depthBufferHalfXOffset = (depthBufferXOffset + 1) / 2; // XXX - is this really right?
	uint32_t depthBufferHalfYOffset = (depthBufferYOffset + 1) / 2; // XXX - is this really right?
	uint32_t depthBufferQuarterXOffset = (depthBufferHalfXOffset + 1) / 2; // XXX - is this really right?
	uint32_t depthBufferQuarterYOffset = (depthBufferHalfYOffset + 1) / 2; // XXX - is this really right?
#endif

	BufferSizeInfo bsi = {};
	bsi.inputOutputBufferWidth = width;
	bsi.inputOutputBufferHeight = height;
	bsi.depthBufferXOffset = depthBufferXOffset;
	bsi.depthBufferYOffset = depthBufferYOffset;
	bsi.depthBufferWidth = depthBufferWidth;
	bsi.depthBufferHeight = depthBufferHeight;

	if (useDownsampledSsao)
	{
		bsi.ssaoBufferWidth = quarterWidth;
		bsi.ssaoBufferHeight = quarterHeight;
		bsi.deinterleavedDepthBufferXOffset = depthBufferQuarterXOffset;
		bsi.deinterleavedDepthBufferYOffset = depthBufferQuarterYOffset;
		bsi.deinterleavedDepthBufferWidth = depthBufferQuarterWidth;
		bsi.deinterleavedDepthBufferHeight = depthBufferQuarterHeight;
		bsi.importanceMapWidth = eighthWidth;
		bsi.importanceMapHeight = eighthHeight;
	}
	else
	{
		bsi.ssaoBufferWidth = halfWidth;
		bsi.ssaoBufferHeight = halfHeight;
		bsi.deinterleavedDepthBufferXOffset = depthBufferHalfXOffset;
		bsi.deinterleavedDepthBufferYOffset = depthBufferHalfYOffset;
		bsi.deinterleavedDepthBufferWidth = depthBufferHalfWidth;
		bsi.deinterleavedDepthBufferHeight = depthBufferHalfHeight;
		bsi.importanceMapWidth = quarterWidth;
		bsi.importanceMapHeight = quarterHeight;
	}
	
	context->bufferSizeInfo = bsi;

	// =======================================
	// allocate intermediate textures

	TEXTURE_INIT(halfDepthsArray, half_depths_array, DXGI_FORMAT_R16_FLOAT, bsi.deinterleavedDepthBufferWidth, bsi.deinterleavedDepthBufferHeight, 4, 4);
	TEXTURE_INIT(pingPongHalfResultA[0], ping_pong_half_result_0, DXGI_FORMAT_R8G8_UNORM, bsi.ssaoBufferWidth, bsi.ssaoBufferHeight, 1, 1);
	TEXTURE_INIT(pingPongHalfResultA[1], ping_pong_half_result_1, DXGI_FORMAT_R8G8_UNORM, bsi.ssaoBufferWidth, bsi.ssaoBufferHeight, 1, 1);
	TEXTURE_INIT(pingPongHalfResultA[2], ping_pong_half_result_2, DXGI_FORMAT_R8G8_UNORM, bsi.ssaoBufferWidth, bsi.ssaoBufferHeight, 1, 1);
	TEXTURE_INIT(pingPongHalfResultA[3], ping_pong_half_result_3, DXGI_FORMAT_R8G8_UNORM, bsi.ssaoBufferWidth, bsi.ssaoBufferHeight, 1, 1);
	TEXTURE_INIT(finalResults, final_results, DXGI_FORMAT_R8G8_UNORM, bsi.ssaoBufferWidth, bsi.ssaoBufferHeight, 4, 1);
	TEXTURE_INIT(deinterlacedNormals, deinterlaced_normals, DXGI_FORMAT_R8G8B8A8_SNORM, bsi.ssaoBufferWidth, bsi.ssaoBufferHeight, 4, 1);
	TEXTURE_INIT(importanceMap, importance_map, DXGI_FORMAT_R8_UNORM, bsi.importanceMapWidth, bsi.importanceMapHeight, 1, 1);
	TEXTURE_INIT(importanceMapPong, importance_map_pong, DXGI_FORMAT_R8_UNORM, bsi.importanceMapWidth, bsi.importanceMapHeight, 1, 1);

	// =======================================
	// Init Prepare SRVs/UAVs

	for (int i = 0; i < 4; ++i)
	{
		textureCreateUav(&context->halfDepthsArray, i, &context->prepareDepthsAndMipsOutputs, i, 4, 0);
	}
	textureCreateUav(&context->halfDepthsArray, 0, &context->prepareDepthsOutputs, 0, 4, 0);
	textureCreateUav(&context->deinterlacedNormals, 0, &context->prepareNormalsOutput, 0, 4, 0);

	device->CreateShaderResourceView(info->depthBufferResource, &info->depthBufferSrvDesc, context->prepareDepthsNormalsAndMipsInputs.cpuDescriptor);

	textureCreateUav(&context->deinterlacedNormals, 0, &context->prepareNormalsFromInputNormalsOutput, 0, 4, 0);
	device->CreateShaderResourceView(info->normalBufferResource, &info->normalBufferSrvDesc, context->prepareNormalsFromInputNormalsInput.cpuDescriptor);

	// =======================================
	// Init Generate SSAO SRVs/UAVs

	for (int i = 0; i < 4; ++i)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_UINT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture1D.MostDetailedMip = 0;
		srvDesc.Texture1D.MipLevels = 1;

		D3D12_SHADER_RESOURCE_VIEW_DESC zeroTextureSRVDesc = {};
		zeroTextureSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
		zeroTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		zeroTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		zeroTextureSRVDesc.Texture1D.MostDetailedMip = 0;
		zeroTextureSRVDesc.Texture1D.MipLevels = 1;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_UINT;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uavDesc.Texture1D.MipSlice = 0;

		textureCreateSrv(&context->halfDepthsArray, 0, &context->generateSSAOInputs[i], -1, 1, i);
		textureCreateSrv(&context->deinterlacedNormals, 6, &context->generateSSAOInputs[i], 0, 4, 0);


		textureCreateSrv(&context->halfDepthsArray, 0, &context->generateAdaptiveSSAOInputs[i], -1, 1, i);
		textureCreateSrvFromDesc(&context->loadCounter, 2, &context->generateAdaptiveSSAOInputs[i], &srvDesc);
		textureCreateSrv(&context->importanceMap, 3, &context->generateAdaptiveSSAOInputs[i], -1, -1, -1);
		textureCreateSrv(&context->finalResults, 4, &context->generateAdaptiveSSAOInputs[i], -1, -1, -1);
		textureCreateSrv(&context->deinterlacedNormals, 6, &context->generateAdaptiveSSAOInputs[i], 0, 4, 0);

		textureCreateUav(&context->finalResults, 0, &context->generateSSAOOutputsFinal[i], 0, 1, i);

		textureCreateUav(&context->pingPongHalfResultA[i], 0, &context->generateSSAOOutputsPing[i], -1, -1, -1);

	}

	// =======================================
	// Init Generate/Postprocess Importance map SRVs/UAVs

	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_UINT;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uavDesc.Texture1D.MipSlice = 0;

		textureCreateSrv(&context->finalResults, 0, &context->generateImportanceMapInputs, -1, -1, -1);
		textureCreateUav(&context->importanceMap, 0, &context->generateImportanceMapOutputs, -1, -1, -1);

		textureCreateSrv(&context->importanceMap, 0, &context->generateImportanceMapAInputs, -1, -1, -1);
		textureCreateUav(&context->importanceMapPong, 0, &context->generateImportanceMapAOutputs, -1, -1, -1);

		textureCreateSrv(&context->importanceMapPong, 0, &context->generateImportanceMapBInputs, -1, -1, -1);
		textureCreateUav(&context->importanceMap, 0, &context->generateImportanceMapBOutputs, -1, -1, -1);
		textureCreateUavFromDesc(&context->loadCounter, 1, &context->generateImportanceMapBOutputs, &uavDesc);
	}

	// =======================================
	// Init De-interleave Blur SRVs/UAVs

	for (int i = 0; i < 4; ++i)
	{
		textureCreateSrv(&context->pingPongHalfResultA[i], 0, &context->pingPongHalfResultASRV[i], -1, -1, -1);
		textureCreateUav(&context->finalResults, 0, &context->finalResultsArrayUAV[i], 0, 1, i);
	}

	// =======================================
	// Init apply SRVs/UAVs

	textureCreateSrv(&context->finalResults, 0, &context->createOutputInputs, -1, -1, -1);

	context->device->CreateUnorderedAccessView(info->outputResource, NULL, &info->outputUavDesc, context->createOutputOutputs.cpuDescriptor);
	context->device->CreateUnorderedAccessView(info->outputResource, NULL, &info->outputUavDesc, context->createOutputOutputs.cpuVisibleCpuDescriptor);

	// =======================================
	// Init upscale SRVs/UAVs

	textureCreateSrv(&context->finalResults, 0, &context->bilateralUpscaleInputs, -1, -1, -1);
	context->device->CreateShaderResourceView(info->depthBufferResource, &info->depthBufferSrvDesc, cbvSrvUavGetCpu(&context->bilateralUpscaleInputs, 1));
	textureCreateSrv(&context->halfDepthsArray, 3, &context->bilateralUpscaleInputs, 0, -1, -1);

	context->device->CreateUnorderedAccessView(info->outputResource, NULL, &info->outputUavDesc, context->bilateralUpscaleOutputs.cpuDescriptor);

	// =======================================
	// Init debug SRVs/UAVs

	context->outputResource = info->outputResource;

	return FFX_CACAO_STATUS_OK;

	ERROR_TEXTURE_DESTROY(importanceMapPong, importance_map_pong);
	ERROR_TEXTURE_DESTROY(importanceMap, importance_map);
	ERROR_TEXTURE_DESTROY(deinterlacedNormals, deinterlaced_normals);
	ERROR_TEXTURE_DESTROY(finalResults, final_results);

	ERROR_TEXTURE_DESTROY(pingPongHalfResultA[3], ping_pong_half_result_3);
	ERROR_TEXTURE_DESTROY(pingPongHalfResultA[2], ping_pong_half_result_2);
	ERROR_TEXTURE_DESTROY(pingPongHalfResultA[1], ping_pong_half_result_1);
	ERROR_TEXTURE_DESTROY(pingPongHalfResultA[0], ping_pong_half_result_0);

	ERROR_TEXTURE_DESTROY(halfDepthsArray, half_depths_array);

	return errorStatus;

#undef TEXTURE_INIT
#undef ERROR_TEXTURE_DESTROY
}

FfxCacaoStatus ffxCacaoD3D12DestroyScreenSizeDependentResources(FfxCacaoD3D12Context* context)
{
	if (context == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	context = getAlignedD3D12ContextPointer(context);

	textureDestroy(&context->importanceMapPong);
	textureDestroy(&context->importanceMap);
	textureDestroy(&context->finalResults);
	textureDestroy(&context->pingPongHalfResultA[3]);
	textureDestroy(&context->pingPongHalfResultA[2]);
	textureDestroy(&context->pingPongHalfResultA[1]);
	textureDestroy(&context->pingPongHalfResultA[0]);
	textureDestroy(&context->halfDepthsArray);

	return FFX_CACAO_STATUS_OK;
}

FfxCacaoStatus ffxCacaoD3D12UpdateSettings(FfxCacaoD3D12Context* context, const FfxCacaoSettings* settings)
{
	if (context == NULL || settings == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	context = getAlignedD3D12ContextPointer(context);

	memcpy(&context->settings, settings, sizeof(*settings));

	return FFX_CACAO_STATUS_OK;
}

FfxCacaoStatus ffxCacaoD3D12Draw(FfxCacaoD3D12Context* context, ID3D12GraphicsCommandList* commandList, const FfxCacaoMatrix4x4* proj, const FfxCacaoMatrix4x4* normalsToView)
{
	if (context == NULL || commandList == NULL || proj == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	context = getAlignedD3D12ContextPointer(context);

#if FFX_CACAO_ENABLE_PROFILING
#define GET_TIMESTAMP(name) gpuTimerGetTimestamp(&context->gpuTimer, commandList, name)
#else
#define GET_TIMESTAMP(name)
#endif
	BufferSizeInfo *bsi = &context->bufferSizeInfo;


	USER_MARKER("CACAO");

	constantBufferRingStartFrame(&context->constantBufferRing);

#if FFX_CACAO_ENABLE_PROFILING
	gpuTimerStartFrame(&context->gpuTimer);
#endif

	GET_TIMESTAMP("Begin CACAO");

	// set the descriptor heaps
	{
		ID3D12DescriptorHeap *descriptorHeaps[] = { context->cbvSrvUavHeap.heap };
		commandList->SetDescriptorHeaps(FFX_CACAO_ARRAY_SIZE(descriptorHeaps), descriptorHeaps);
	}

	// clear load counter
	{
		UINT clearValue[] = { 0, 0, 0, 0 };
		commandList->ClearUnorderedAccessViewUint(context->loadCounterUav.gpuDescriptor, context->loadCounterUav.cpuVisibleCpuDescriptor, context->loadCounter.resource, clearValue, 0, NULL);
	}

	// move this to initialisation
	D3D12_GPU_VIRTUAL_ADDRESS cbCACAOHandle;
	FfxCacaoConstants *pCACAOConsts;
	D3D12_GPU_VIRTUAL_ADDRESS cbCACAOPerPassHandle[4];
	FfxCacaoConstants *pPerPassConsts[4];

	// upload constant buffers
	{
		USER_MARKER("Upload Constant Buffers");
		constantBufferRingAlloc(&context->constantBufferRing, sizeof(*pCACAOConsts), (void**)&pCACAOConsts, &cbCACAOHandle);
		updateConstants(pCACAOConsts, &context->settings, bsi, proj, normalsToView);

		for (int i = 0; i < 4; ++i)
		{
			constantBufferRingAlloc(&context->constantBufferRing, sizeof(*pPerPassConsts[0]), (void**)&pPerPassConsts[i], &cbCACAOPerPassHandle[i]);
			updateConstants(pPerPassConsts[i], &context->settings, bsi, proj, normalsToView);
			updatePerPassConstants(pPerPassConsts[i], &context->settings, &context->bufferSizeInfo, i);
		}

		GET_TIMESTAMP("Upload Constant Buffers");
	}

	// prepare depths, normals and mips
	{
		USER_MARKER("Prepare downsampled depths, normals and mips");

		switch (context->settings.qualityLevel)
		{
		case FFX_CACAO_QUALITY_LOWEST: {
			uint32_t dispatchWidth = dispatchSize(PREPARE_DEPTHS_HALF_WIDTH, bsi->deinterleavedDepthBufferWidth);
			uint32_t dispatchHeight = dispatchSize(PREPARE_DEPTHS_HALF_HEIGHT, bsi->deinterleavedDepthBufferHeight);
			ComputeShader *prepareDepthsHalf = context->useDownsampledSsao ? &context->prepareDownsampledDepthsHalf : &context->prepareNativeDepthsHalf;
			computeShaderDraw(prepareDepthsHalf, commandList, cbCACAOHandle, &context->prepareDepthsOutputs, &context->prepareDepthsNormalsAndMipsInputs, dispatchWidth, dispatchHeight, 1);
			break;
		}
		case FFX_CACAO_QUALITY_LOW: {
			uint32_t dispatchWidth = dispatchSize(PREPARE_DEPTHS_WIDTH, bsi->deinterleavedDepthBufferWidth);
			uint32_t dispatchHeight = dispatchSize(PREPARE_DEPTHS_HEIGHT, bsi->deinterleavedDepthBufferHeight);
			ComputeShader *prepareDepths = context->useDownsampledSsao ? &context->prepareDownsampledDepths : &context->prepareNativeDepths;
			computeShaderDraw(prepareDepths, commandList, cbCACAOHandle, &context->prepareDepthsOutputs, &context->prepareDepthsNormalsAndMipsInputs, dispatchWidth, dispatchHeight, 1);
			break;
		}
		default: {
			uint32_t dispatchWidth = dispatchSize(PREPARE_DEPTHS_AND_MIPS_WIDTH, bsi->deinterleavedDepthBufferWidth);
			uint32_t dispatchHeight = dispatchSize(PREPARE_DEPTHS_AND_MIPS_HEIGHT, bsi->deinterleavedDepthBufferHeight);
			ComputeShader *prepareDepthsAndMips = context->useDownsampledSsao ? &context->prepareDownsampledDepthsAndMips : &context->prepareNativeDepthsAndMips;
			computeShaderDraw(prepareDepthsAndMips, commandList, cbCACAOHandle, &context->prepareDepthsAndMipsOutputs, &context->prepareDepthsNormalsAndMipsInputs, dispatchWidth, dispatchHeight, 1);
			break;
		}
		}

		CD3DX12_RESOURCE_BARRIER barriers[2];
		UINT numBarriers = 0;

		barriers[numBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(context->deinterlacedNormals.resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		barriers[numBarriers++] = CD3DX12_RESOURCE_BARRIER::UAV(context->halfDepthsArray.resource);
		commandList->ResourceBarrier(numBarriers, barriers);
		numBarriers = 0;

		if (context->settings.generateNormals)
		{
			uint32_t dispatchWidth = dispatchSize(PREPARE_NORMALS_WIDTH, bsi->ssaoBufferWidth);
			uint32_t dispatchHeight = dispatchSize(PREPARE_NORMALS_HEIGHT, bsi->ssaoBufferHeight);
			ComputeShader *prepareNormals = context->useDownsampledSsao ? &context->prepareDownsampledNormals : &context->prepareNativeNormals;
			computeShaderDraw(prepareNormals, commandList, cbCACAOHandle, &context->prepareNormalsOutput, &context->prepareDepthsNormalsAndMipsInputs, dispatchWidth, dispatchHeight, 1);
		}
		else
		{
			uint32_t dispatchWidth = dispatchSize(PREPARE_NORMALS_FROM_INPUT_NORMALS_WIDTH, bsi->ssaoBufferWidth);
			uint32_t dispatchHeight = dispatchSize(PREPARE_NORMALS_FROM_INPUT_NORMALS_HEIGHT, bsi->ssaoBufferHeight);
			ComputeShader *prepareNormalsFromInputNormals = context->useDownsampledSsao ? &context->prepareDownsampledNormalsFromInputNormals : &context->prepareNativeNormalsFromInputNormals;
			computeShaderDraw(prepareNormalsFromInputNormals, commandList, cbCACAOHandle, &context->prepareNormalsFromInputNormalsOutput, &context->prepareNormalsFromInputNormalsInput, dispatchWidth, dispatchHeight, 1);
		}

		barriers[numBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(context->deinterlacedNormals.resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(numBarriers, barriers);
		numBarriers = 0;

		GET_TIMESTAMP("Prepare downsampled depths, normals and mips");
	}


	// base pass for highest quality setting
	if (context->settings.qualityLevel == FFX_CACAO_QUALITY_HIGHEST)
	{
		USER_MARKER("Generate High Quality Base Pass");

		// SSAO
		{
			USER_MARKER("SSAO");

			for (int pass = 0; pass < 4; ++pass)
			{
				CbvSrvUav *inputs = &context->generateSSAOInputs[pass];
				uint32_t dispatchWidth = dispatchSize(GENERATE_WIDTH, bsi->ssaoBufferWidth);
				uint32_t dispatchHeight = dispatchSize(GENERATE_WIDTH, bsi->ssaoBufferHeight);
				computeShaderDraw(&context->generateSSAO[4], commandList, cbCACAOPerPassHandle[pass], &context->generateSSAOOutputsFinal[pass], inputs, dispatchWidth, dispatchHeight, 1);
			}
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(context->finalResults.resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			GET_TIMESTAMP("High Quality Base Pass SSAO");
		}


		// generate importance map
		{
			USER_MARKER("Importance Map");

			CD3DX12_RESOURCE_BARRIER barriers[2];
			UINT barrierCount;

			uint32_t dispatchWidth = dispatchSize(IMPORTANCE_MAP_WIDTH, bsi->importanceMapWidth);
			uint32_t dispatchHeight = dispatchSize(IMPORTANCE_MAP_HEIGHT, bsi->importanceMapHeight);

			computeShaderDraw(&context->generateImportanceMap, commandList, cbCACAOHandle, &context->generateImportanceMapOutputs, &context->generateImportanceMapInputs, dispatchWidth, dispatchHeight, 1);

			barrierCount = 0;
			barriers[barrierCount++] = CD3DX12_RESOURCE_BARRIER::Transition(context->importanceMap.resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(barrierCount, barriers);

			computeShaderDraw(&context->postprocessImportanceMapA, commandList, cbCACAOHandle, &context->generateImportanceMapAOutputs, &context->generateImportanceMapAInputs, dispatchWidth, dispatchHeight, 1);

			barrierCount = 0;
			barriers[barrierCount++] = CD3DX12_RESOURCE_BARRIER::Transition(context->importanceMap.resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			barriers[barrierCount++] = CD3DX12_RESOURCE_BARRIER::Transition(context->importanceMapPong.resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(barrierCount, barriers);

			computeShaderDraw(&context->postprocessImportanceMapB, commandList, cbCACAOHandle, &context->generateImportanceMapBOutputs, &context->generateImportanceMapBInputs, dispatchWidth, dispatchHeight, 1);

			barrierCount = 0;
			barriers[barrierCount++] = CD3DX12_RESOURCE_BARRIER::Transition(context->importanceMap.resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(barrierCount, barriers);
			GET_TIMESTAMP("Generate Importance Map");
		}
	}

	int blurPassCount = context->settings.blurPassCount;
	blurPassCount = FFX_CACAO_CLAMP(blurPassCount, 0, MAX_BLUR_PASSES);
	
	// main ssao generation
	{
		USER_MARKER("Generate SSAO");

		ComputeShader *generate = &context->generateSSAO[FFX_CACAO_MAX(0, context->settings.qualityLevel - 1)];
		for (int pass = 0; pass < 4; ++pass)
		{
			if (context->settings.qualityLevel == FFX_CACAO_QUALITY_LOWEST && (pass == 1 || pass == 2))
			{
				continue;
			}

			CbvSrvUav *input = context->settings.qualityLevel == FFX_CACAO_QUALITY_HIGHEST ? &context->generateAdaptiveSSAOInputs[pass] : &context->generateSSAOInputs[pass];
			CbvSrvUav *output = blurPassCount == 0 ? &context->generateSSAOOutputsFinal[pass] : &context->generateSSAOOutputsPing[pass];

			uint32_t dispatchWidth = dispatchSize(GENERATE_WIDTH, bsi->ssaoBufferWidth);
			uint32_t dispatchHeight = dispatchSize(GENERATE_WIDTH, bsi->ssaoBufferHeight);
			computeShaderDraw(generate, commandList, cbCACAOPerPassHandle[pass], output, input, dispatchWidth, dispatchHeight, 1);
		}

		GET_TIMESTAMP("Generate SSAO");
	}

	// de-interleaved blur
	if (blurPassCount)
	{
		CD3DX12_RESOURCE_BARRIER barriers[5];
		UINT barrierCount = 0;
		for (int pass = 0; pass < 4; ++pass)
		{
			if (context->settings.qualityLevel == FFX_CACAO_QUALITY_LOWEST && (pass == 1 || pass == 2))
			{
				continue;
			}
			barriers[barrierCount++] = CD3DX12_RESOURCE_BARRIER::Transition(context->pingPongHalfResultA[pass].resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
		barriers[barrierCount++] = CD3DX12_RESOURCE_BARRIER::Transition(context->finalResults.resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(barrierCount, barriers);

		USER_MARKER("Deinterleaved blur");

		for (int pass = 0; pass < 4; ++pass)
		{
			if (context->settings.qualityLevel == FFX_CACAO_QUALITY_LOWEST && (pass == 1 || pass == 2))
			{
				continue;
			}

			uint32_t w = 4 * BLUR_WIDTH - 2 * blurPassCount;
			uint32_t h = 3 * BLUR_HEIGHT - 2 * blurPassCount;
			uint32_t blurPassIndex = blurPassCount - 1;
			uint32_t dispatchWidth = dispatchSize(w, bsi->ssaoBufferWidth);
			uint32_t dispatchHeight = dispatchSize(h, bsi->ssaoBufferHeight);
			computeShaderDraw(&context->edgeSensitiveBlur[blurPassIndex], commandList, cbCACAOPerPassHandle[pass], &context->finalResultsArrayUAV[pass], &context->pingPongHalfResultASRV[pass], dispatchWidth, dispatchHeight, 1);
		}

		GET_TIMESTAMP("Deinterleaved blur");
	}

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(context->finalResults.resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	if (context->useDownsampledSsao)
	{
		USER_MARKER("Upscale");

		CbvSrvUav *inputs = &context->bilateralUpscaleInputs;
		ComputeShader *upscaler = context->settings.qualityLevel == FFX_CACAO_QUALITY_LOWEST ? &context->upscaleBilateral5x5Half : &context->upscaleBilateral5x5;
		uint32_t dispatchWidth = dispatchSize(2 * BILATERAL_UPSCALE_WIDTH, bsi->inputOutputBufferWidth);
		uint32_t dispatchHeight = dispatchSize(2 * BILATERAL_UPSCALE_HEIGHT, bsi->inputOutputBufferHeight);
		computeShaderDraw(upscaler, commandList, cbCACAOHandle, &context->bilateralUpscaleOutputs, inputs, dispatchWidth, dispatchHeight, 1);

		GET_TIMESTAMP("Upscale");
	}
	else
	{
		USER_MARKER("Create Output");
		uint32_t dispatchWidth = dispatchSize(APPLY_WIDTH, bsi->inputOutputBufferWidth);
		uint32_t dispatchHeight = dispatchSize(APPLY_HEIGHT, bsi->inputOutputBufferHeight);
		switch (context->settings.qualityLevel)
		{
		case FFX_CACAO_QUALITY_LOWEST:
			computeShaderDraw(&context->nonSmartHalfApply, commandList, cbCACAOHandle, &context->createOutputOutputs, &context->createOutputInputs, dispatchWidth, dispatchHeight, 1);
			break;
		case FFX_CACAO_QUALITY_LOW:
			computeShaderDraw(&context->nonSmartApply, commandList, cbCACAOHandle, &context->createOutputOutputs, &context->createOutputInputs, dispatchWidth, dispatchHeight, 1);
			break;
		default:
			computeShaderDraw(&context->smartApply, commandList, cbCACAOHandle, &context->createOutputOutputs, &context->createOutputInputs, dispatchWidth, dispatchHeight, 1);
			break;
		}
		GET_TIMESTAMP("Create Output (Reinterleave)");
	}

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(context->outputResource));

#if FFX_CACAO_ENABLE_PROFILING
	gpuTimerEndFrame(&context->gpuTimer, commandList);
#endif

	return FFX_CACAO_STATUS_OK;

#undef GET_TIMESTAMP
}

#if FFX_CACAO_ENABLE_PROFILING
FfxCacaoStatus ffxCacaoD3D12GetDetailedTimings(FfxCacaoD3D12Context* context, FfxCacaoDetailedTiming* timings)
{
	if (context == NULL || timings == NULL)
	{
		return FFX_CACAO_STATUS_INVALID_POINTER;
	}
	context = getAlignedD3D12ContextPointer(context);

	gpuTimerCollectTimings(&context->gpuTimer, timings);

	return FFX_CACAO_STATUS_OK;
}
#endif
#endif

#ifdef __cplusplus
}
#endif