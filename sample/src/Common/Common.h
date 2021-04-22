// AMD Sample sample code
//
// Copyright(c) 2021 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#pragma once

#include "ffx_cacao.h"

struct Preset
{
	bool useDownsampledSsao;
	FFX_CACAO_Settings settings;
};


static const char *FFX_CACAO_PRESET_NAMES[] = {
	"Native - Adaptive Quality",
	"Native - High Quality",
	"Native - Medium Quality",
	"Native - Low Quality",
	"Native - Lowest Quality",
	"Downsampled - Adaptive Quality",
	"Downsampled - High Quality",
	"Downsampled - Medium Quality",
	"Downsampled - Low Quality",
	"Downsampled - Lowest Quality",
	"Custom"
};

static const Preset FFX_CACAO_PRESETS[] = {
	// Native - Adaptive Quality
	{
		/* useDownsampledSsao */ false,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_HIGHEST,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 2,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.1f,
		}
	},
	// Native - High Quality
	{
		/* useDownsampledSsao */ false,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_HIGH,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 2,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.1f,
		}
	},
	// Native - Medium Quality
	{
		/* useDownsampledSsao */ false,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_MEDIUM,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 2,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.1f,
		}
	},
	// Native - Low Quality
	{
		/* useDownsampledSsao */ false,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_LOW,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 6,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.1f,
		}
	},
	// Native - Lowest Quality
	{
		/* useDownsampledSsao */ false,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_LOWEST,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 6,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.1f,
		}
	},
	// Downsampled - Highest Quality
	{
		/* useDownsampledSsao */ true,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_HIGHEST,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 2,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.1f,
		}
	},
	// Downsampled - High Quality
	{
		/* useDownsampledSsao */ true,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_HIGH,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 2,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.1f,
		}
	},
	// Downsampled - Medium Quality
	{
		/* useDownsampledSsao */ true,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_MEDIUM,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 3,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.2f,
		}
	},
	// Downsampled - Low Quality
	{
		/* useDownsampledSsao */ true,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_LOW,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 6,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 8.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.8f,
		}
	},
	// Downsampled - Lowest Quality
	{
		/* useDownsampledSsao */ true,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_LOWEST,
			/* adaptiveQualityLimit              */ 0.75f,
			/* blurPassCount                     */ 6,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 8.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.8f,
		}
	}
};
