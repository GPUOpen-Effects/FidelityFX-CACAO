// AMD SampleVK sample code
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

#include "ffx_cacao_impl.h"

// We are queuing (backBufferCount + 0.5) frames, so we need to triple buffer the resources that get modified each frame
static const int backBufferCount = 3;

#define USE_VID_MEM true

using namespace CAULDRON_VK;

//
// This class deals with the GPU side of the sample.
//

class SampleRenderer
{
public:
	struct Spotlight
	{
		Camera   light;
		XMVECTOR color;
		float    intensity;
	};

	struct State
	{
		float              time;
		Camera             camera;

		float              exposure;
		float              iblFactor;
		float              emmisiveFactor;

		int                toneMapper;
		int                skyDomeType;
		bool               drawBoundingBoxes;

		bool               useTAA;

		bool               drawLightFrustum;

		bool               useDownsampledSsao;
		FFX_CACAO_Settings cacaoSettings;
		bool               useCacao;
		bool               dispalyCacaoDirectly;

	};

	void OnCreate(Device *pDevice, SwapChain *pSwapChain);
	void OnDestroy();

	void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
	void OnDestroyWindowSizeDependentResources();

	int LoadScene(GLTFCommon *pGLTFCommon, int stage = 0);
	void UnloadScene();

#ifdef FFX_CACAO_ENABLE_PROFILING
	void GetCacaoTimingValues(State* pState, FFX_CACAO_DetailedTiming* timings);
#endif
	const std::vector<TimeStamp> &GetTimingValues() { return m_timeStamps; }

	void OnRender(State *pState, SwapChain *pSwapChain);

private:
	Device *m_pDevice;

	FFX_CACAO_VkContext            *m_cacaoContextNative;
	FFX_CACAO_VkContext            *m_cacaoContextDownsampled;

	uint32_t                        m_width;
	uint32_t                        m_height;

	VkRect2D                        m_rectScissor;
	VkViewport                      m_viewport;

	// Initialize helper classes
	ResourceViewHeaps               m_resourceViewHeaps;
	UploadHeap                      m_uploadHeap;
	DynamicBufferRing               m_constantBufferRing;
	StaticBufferPool                m_vidMemBufferPool;
	StaticBufferPool                m_sysMemBufferPool;
	CommandListRing                 m_commandListRing;
	GPUTimestamps                   m_gpuTimer;

	//gltf passes
	GltfPbrPass                    *m_gltfPBR;
	GltfPbrPass                    *m_gltfPbrNonMsaa;
	GltfBBoxPass                   *m_gltfBBox;
	GltfDepthPass                  *m_gltfDepth;
	GLTFTexturesAndBuffers         *m_pGLTFTexturesAndBuffers;

	// effects
	Bloom                           m_bloom;
	SkyDome                         m_skyDome;
	DownSamplePS                    m_downSample;
	SkyDomeProc                     m_skyDomeProc;
	ToneMapping                     m_toneMappingPS;
	ToneMappingCS                   m_toneMappingCS;
	ColorConversionPS               m_colorConversionPS;

	// GUI
	ImGUI                           m_imGUI;

	// Temporary render targets

	// depth buffer
	Texture                         m_depthBuffer;
	VkImageView                     m_depthBufferDSV;

	// shadowmaps
	Texture                         m_shadowMap;
	VkImageView                     m_shadowMapDSV;
	VkImageView                     m_shadowMapSRV;

	// MSAA RT
	Texture                         m_hdrMSAA;
	VkImageView                     m_hdrMSAASRV;

	// Non MSAA
	Texture                         m_normalBufferNonMsaa;
	Texture                         m_depthBufferNonMsaa;
	VkImageView                     m_normalBufferNonMsaaView;
	VkImageView                     m_depthBufferNonMsaaView;

	// Resolved RT
	Texture                         m_hdr;
	VkImageView                     m_hdrSRV;
	VkImageView                     m_hdrUAV;

	// CACAO
	Texture                         m_cacaoOutput;
	VkImageView                     m_cacaoOutputSRV;

	Texture                         m_finalOutput;
	VkImageView                     m_finalOutputView;

	uint32_t                        m_curBackBuffer;

	VkSampler                       m_cacaoApplyDirectSampler;
	VkDescriptorSet                 m_cacaoApplyDirectDescriptorSets[backBufferCount];
	VkDescriptorSetLayout           m_cacaoApplyDirectDescriptorSetLayout;
	PostProcPS                      m_cacaoApplyDirectPS;

	// widgets
	Wireframe                       m_wireframe;
	WireframeBox                    m_wireframeBox;

	VkRenderPass                    m_renderPassShadow;
	VkRenderPass                    m_renderPassHDRMSAA;
	VkRenderPass                    m_renderPassPBRHDR;
	VkRenderPass                    m_renderPassNonMSAA;

	VkFramebuffer                   m_pFrameBufferShadow;
	VkFramebuffer                   m_pFrameBufferHDRMSAA;
	VkFramebuffer                   m_pFrameBufferPBRHDR;
	VkFramebuffer                   m_pFrameBufferNonMSAA;

	std::vector<TimeStamp>          m_timeStamps;
};

