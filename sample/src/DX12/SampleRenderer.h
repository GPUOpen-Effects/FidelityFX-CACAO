// AMD SampleDX12 sample code
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

static const int backBufferCount = 2;

#define USE_VID_MEM true

using namespace CAULDRON_DX12;

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

		uint32_t           spotlightCount;
		Spotlight          spotlight[4];
		bool               drawLightFrustum;

		bool               useDownsampledSSAO;
		bool               displayCacaoDirectly;
		bool               useCACAO;
		FFX_CACAO_Settings cacaoSettings;
	};

	void OnCreate(Device* pDevice, SwapChain *pSwapChain);
	void OnDestroy();

	void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
	void OnDestroyWindowSizeDependentResources();

	int LoadScene(GLTFCommon *pGLTFCommon, int stage = 0);
	void UnloadScene();

	const std::vector<TimeStamp> &GetTimingValues() { return m_timeStamps; }

	void OnRender(State *pState, SwapChain *pSwapChain);

#ifdef FFX_CACAO_ENABLE_PROFILING
	void GetCacaoTimings(State *pState, FFX_CACAO_DetailedTiming* timings, uint64_t* gpuTicksPerSecong);
#endif

private:
	Device                         *m_pDevice;

	uint32_t                        m_width;
	uint32_t                        m_height;
	uint32_t                        m_halfWidth;
	uint32_t                        m_halfHeight;
	D3D12_VIEWPORT                  m_viewPort;
	D3D12_RECT                      m_rectScissor;
	bool                            m_hasTAA = false;

	// Initialize helper classes
	ResourceViewHeaps               m_resourceViewHeaps;
	UploadHeap                      m_uploadHeap;
	DynamicBufferRing               m_constantBufferRing;
	StaticBufferPool                m_vidMemBufferPool;
	CommandListRing                 m_commandListRing;
	GPUTimestamps                   m_gpuTimer;

	//gltf passes
	GltfPbrPass                    *m_gltfPBRNonMsaa;
	GltfPbrPass                    *m_gltfPBR;
	GltfBBoxPass                   *m_gltfBBox;
	GltfDepthPass                  *m_gltfDepth;
	GLTFTexturesAndBuffers         *m_pGLTFTexturesAndBuffers;

	// effects
	Bloom                           m_bloom;
	SkyDome                         m_skyDome;
	DownSamplePS                    m_downSample;
	SkyDomeProc                     m_skyDomeProc;
	ToneMapping                     m_toneMapping;
	PostProcCS                      m_motionBlur;
	Sharpen                         m_sharpen;
	TAA                             m_taa;

	// ================================
	// CACAO stuff
	CBV_SRV_UAV                     m_cacaoApplyDirectInput;
	PostProcPS                      m_cacaoApplyDirect;
	PostProcCS                      m_cacaoUAVClear;

	Texture                         m_cacaoOutput;
	CBV_SRV_UAV                     m_cacaoOutputUAV;
	CBV_SRV_UAV                     m_cacaoOutputSRV;

	FFX_CACAO_D3D12Context         *m_pCACAOContextNative;
	FFX_CACAO_D3D12Context         *m_pCACAOContextDownsampled;

	// GUI
	ImGUI                           m_imGUI;

	// Deferred pass buffers
	Texture                         m_depthBufferNonMsaa;
	DSV                             m_depthBufferNonMsaaDSV;
	CBV_SRV_UAV                     m_depthBufferNonMsaaSRV;

	Texture                         m_normalBufferNonMsaa;
	RTV                             m_normalBufferNonMsaaRTV;
	CBV_SRV_UAV                     m_normalBufferNonMsaaSRV;

	// depth buffer
	Texture                         m_depthBuffer;
	DSV                             m_depthBufferDSV;
	CBV_SRV_UAV                     m_depthBufferSRV;

	// TAA buffer
	Texture                         m_taaBuffer;
	CBV_SRV_UAV                     m_taaBufferSRV;
	CBV_SRV_UAV                     m_taaBufferUAV;
	CBV_SRV_UAV                     m_taaInputsSRV;
	Texture                         m_historyBuffer;
	RTV                             m_historyBufferRTV;

	// shadowmaps
	Texture                         m_shadowMap;
	DSV                             m_shadowMapDSV;
	CBV_SRV_UAV                     m_shadowMapSRV;

	// MSAA RT
	Texture                         m_hdrMSAA;
	RTV                             m_hdrRTVMSAA;

	// Resolved RT
	Texture                         m_hdr;
	CBV_SRV_UAV                     m_hdrSRV;
	RTV                             m_hdrRTV;

	// widgets
	Wireframe                       m_wireframe;
	WireframeBox                    m_wireframeBox;

	std::vector<TimeStamp>          m_timeStamps;
};
