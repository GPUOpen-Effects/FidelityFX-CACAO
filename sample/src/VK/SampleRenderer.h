// AMD SampleVK sample code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
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
        Camera light;
        XMVECTOR color;
        float intensity;
    };

    struct State
    {
        float time;
        Camera camera;

        float exposure;
        float iblFactor;
        float emmisiveFactor;

        int   toneMapper;
        int   skyDomeType;
        bool  bDrawBoundingBoxes;

        bool  m_useTAA;

        bool  bDrawLightFrustum;

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
		bool             m_useDownsampledSsao;
#endif
		FfxCacaoSettings m_cacaoSettings;
		bool             m_useCacao;
		bool             m_dispalyCacaoDirectly;

	};

    void OnCreate(Device *pDevice, SwapChain *pSwapChain);
    void OnDestroy();

    void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
    void OnDestroyWindowSizeDependentResources();

    int LoadScene(GLTFCommon *pGLTFCommon, int stage = 0);
    void UnloadScene();

#ifdef FFX_CACAO_ENABLE_PROFILING
	void GetCacaoTimingValues(State* pState, FfxCacaoDetailedTiming* timings);
#endif
    const std::vector<TimeStamp> &GetTimingValues() { return m_TimeStamps; }

    void OnRender(State *pState, SwapChain *pSwapChain);

private:
    Device *m_pDevice;

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	FfxCacaoVkContext              *m_cacaoContextNative;
#endif
	FfxCacaoVkContext              *m_cacaoContextDownsampled;

    uint32_t                        m_Width;
    uint32_t                        m_Height;

    VkRect2D                        m_rectScissor;
    VkViewport                      m_viewport;

    // Initialize helper classes
    ResourceViewHeaps               m_resourceViewHeaps;
    UploadHeap                      m_UploadHeap;
    DynamicBufferRing               m_ConstantBufferRing;
    StaticBufferPool                m_VidMemBufferPool;
    StaticBufferPool                m_SysMemBufferPool;
    CommandListRing                 m_CommandListRing;
    GPUTimestamps                   m_GPUTimer;

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
    ImGUI                           m_ImGUI;

    // Temporary render targets

    // depth buffer
    Texture                         m_depthBuffer;
    VkImageView                     m_depthBufferDSV;
	
    // shadowmaps
    Texture                         m_shadowMap;
    VkImageView                     m_shadowMapDSV;
    VkImageView                     m_shadowMapSRV;

    // MSAA RT
    Texture                         m_HDRMSAA;
    VkImageView                     m_HDRMSAASRV;

	// Non MSAA
	Texture     m_NormalBufferNonMsaa;
	Texture     m_DepthBufferNonMsaa;
	VkImageView m_NormalBufferNonMsaaView;
	VkImageView m_DepthBufferNonMsaaView;

    // Resolved RT
    Texture                         m_HDR;
    VkImageView                     m_HDRSRV;
    VkImageView                     m_HDRUAV;

	// CACAO
	Texture                         m_cacaoOutput;
	VkImageView                     m_cacaoOutputSRV;

	Texture                         m_finalOutput;
	VkImageView                     m_finalOutputView;

	uint32_t                        m_curBackBuffer;

	VkSampler                       m_directOutputSampler;
	VkDescriptorSet                 m_directOutputDescriptorSets[backBufferCount];
	VkDescriptorSetLayout           m_directOutputDescriptorSetLayout;
	PostProcPS                      m_directOutputPS;

    // widgets
    Wireframe                       m_wireframe;
    WireframeBox                    m_wireframeBox;

    VkRenderPass                    m_render_pass_shadow;
    VkRenderPass                    m_render_pass_HDR_MSAA;
    VkRenderPass                    m_render_pass_PBR_HDR;
	VkRenderPass                    m_render_pass_non_msaa;

    VkFramebuffer                   m_pFrameBuffer_shadow;
    VkFramebuffer                   m_pFrameBuffer_HDR_MSAA;
    VkFramebuffer                   m_pFrameBuffer_PBR_HDR;
	VkFramebuffer                   m_pFrameBuffer_non_msaa;

    std::vector<TimeStamp>          m_TimeStamps;
};

