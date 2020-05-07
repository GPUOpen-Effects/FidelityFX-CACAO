// AMD SampleDX12 sample code
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

static const int backBufferCount = 2;

#define USE_CACAO true
#define USE_VID_MEM true
#define USE_SHADOWMASK false
#define USE_EXPANDED_DEPTH_BUFFER false

using namespace CAULDRON_DX12;

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

        uint32_t  spotlightCount;
        Spotlight spotlight[4];
        bool  bDrawLightFrustum;

		bool bUseCACAO;
    };

    void OnCreate(Device* pDevice, SwapChain *pSwapChain);
    void OnDestroy();

    void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
    void OnDestroyWindowSizeDependentResources();

    int LoadScene(GLTFCommon *pGLTFCommon, int stage = 0);
    void UnloadScene();

    /* bool GetHasTAA() const { return m_HasTAA; }
    void SetHasTAA(bool hasTAA) { m_HasTAA = hasTAA; } */

    const std::vector<TimeStamp> &GetTimingValues() { return m_TimeStamps; }

    void OnRender(State *pState, FfxCacaoSettings *cacaoSettings, SwapChain *pSwapChain, bool displayCacaoDirectly, bool useDownsampledSsao);

#if USE_CACAO
#if FFX_CACAO_ENABLE_PROFILING
	void GetCacaoTimings(FfxCacaoDetailedTiming* timings, uint64_t* gpuTicksPerSecong);
#endif
#endif

private:
    Device                         *m_pDevice;

    uint32_t                        m_Width;
    uint32_t                        m_Height;
	uint32_t                        m_HalfWidth;
	uint32_t                        m_HalfHeight;
    D3D12_VIEWPORT                  m_viewPort;
    D3D12_RECT                      m_RectScissor;
    bool                            m_HasTAA = false;

    // Initialize helper classes
    ResourceViewHeaps               m_resourceViewHeaps;
    UploadHeap                      m_UploadHeap;
    DynamicBufferRing               m_ConstantBufferRing;
    StaticBufferPool                m_VidMemBufferPool;
    CommandListRing                 m_CommandListRing;
    GPUTimestamps                   m_GPUTimer;

    //gltf passes
    GltfPbrPass                    *m_gltfPBR;
    GltfBBoxPass                   *m_gltfBBox;
    GltfDepthPass                  *m_gltfDepth;
    GltfMotionVectorsPass          *m_gltfMotionVectors;
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
#if USE_CACAO
	CBV_SRV_UAV                     m_applyCACAOInputs;
	CBV_SRV_UAV                     m_applyCACAOOutputs;
	PostProcCS                      m_applyCACAO;
	CBV_SRV_UAV                     m_applyDirectInput;
	PostProcPS                      m_applyDirect;

#if USE_EXPANDED_DEPTH_BUFFER
	bool                            m_useExpandedDepthBuffer;

	int                             m_expandedDepthWidth;
	int                             m_expandedDepthHeight;
	DSV                             m_expandedDepthBufferDSV;
	Texture                         m_expandedDepthBuffer;

	Camera                          m_expandedDepthCamera;

	FfxCacaoD3D12Context           *m_pCacaoContextNativeExpanded;
	FfxCacaoD3D12Context           *m_pCacaoContextDownsampledExpanded;
#endif

	bool                            m_useDownsampledSsao;

	Texture                         m_CacaoOutput;

	FfxCacaoD3D12Context           *m_pCacaoContextNativeNonExpanded;
	FfxCacaoD3D12Context           *m_pCacaoContextDownsampledNonExpanded;
#endif

    // GUI
    ImGUI                           m_ImGUI;

    // Temporary render targets

    // depth buffer
    Texture                         m_depthBuffer;
    DSV                             m_depthBufferDSV;
    CBV_SRV_UAV                     m_depthBufferSRV;

	Texture                         m_cacaoDepthBuffer;
	DSV                             m_cacaoDepthBufferDSV;
	CBV_SRV_UAV                     m_cacaoDepthBufferSRV;

    // Motion Vectors resources
    Texture                         m_MotionVectorsDepthMap;
    DSV                             m_MotionVectorsDepthMapDSV;
    CBV_SRV_UAV                     m_MotionVectorsDepthMapSRV;
    Texture                         m_MotionVectors;
    RTV                             m_MotionVectorsRTV;
    CBV_SRV_UAV                     m_MotionVectorsSRV;
    CBV_SRV_UAV                     m_MotionVectorsInputsSRV;

    // TAA buffer
    Texture                         m_TAABuffer;
    CBV_SRV_UAV                     m_TAABufferSRV;
    CBV_SRV_UAV                     m_TAABufferUAV;
    CBV_SRV_UAV                     m_TAAInputsSRV;
    Texture                         m_HistoryBuffer;
    RTV                             m_HistoryBufferRTV;

    // Normal buffer
    Texture                         m_NormalBuffer;
    RTV                             m_NormalBufferRTV;
    CBV_SRV_UAV                     m_NormalBufferSRV;

#if USE_SHADOWMASK
    // shadow buffer
    Texture                         m_ShadowMask;
    CBV_SRV_UAV                     m_ShadowMaskUAV;
    CBV_SRV_UAV                     m_ShadowMaskSRV;
    ShadowResolvePass               m_shadowResolve;
#endif

    // shadowmaps
    Texture                         m_ShadowMap;
    DSV                             m_ShadowMapDSV;
    CBV_SRV_UAV                     m_ShadowMapSRV;

    // MSAA RT
    Texture                         m_HDRMSAA;
    RTV                             m_HDRRTVMSAA;

    // Resolved RT
    Texture                         m_HDR;
    CBV_SRV_UAV                     m_HDRSRV;
    RTV                             m_HDRRTV;

    // motion blur
    Texture                         m_MotionBlurOutput;
    CBV_SRV_UAV                     m_MotionBlurOutputSRV;
    CBV_SRV_UAV                     m_MotionBlurOutputUAV;

    // widgets
    Wireframe                       m_wireframe;
    WireframeBox                    m_wireframeBox;

    std::vector<TimeStamp>          m_TimeStamps;
};
