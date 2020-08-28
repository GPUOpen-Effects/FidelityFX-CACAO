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

#include "stdafx.h"

#include "SampleRenderer.h"

//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void SampleRenderer::OnCreate(Device* pDevice, SwapChain *pSwapChain)
{
    m_pDevice = pDevice;

    // Initialize helpers

    // Create all the heaps for the resources views
    const uint32_t cbvDescriptorCount = 3000;
    const uint32_t srvDescriptorCount = 3000;
    const uint32_t uavDescriptorCount = 100;
    const uint32_t dsvDescriptorCount = 100;
    const uint32_t rtvDescriptorCount = 1000;
    const uint32_t samplerDescriptorCount = 50;
    m_resourceViewHeaps.OnCreate(pDevice, cbvDescriptorCount, srvDescriptorCount, uavDescriptorCount, dsvDescriptorCount, rtvDescriptorCount, samplerDescriptorCount);

    // Create a commandlist ring for the Direct queue
    // We are queuing (backBufferCount + 0.5) frames, so we need to triple buffer the command lists
    uint32_t commandListsPerBackBuffer = 8;
    m_CommandListRing.OnCreate(pDevice, backBufferCount + 1, commandListsPerBackBuffer, pDevice->GetGraphicsQueue()->GetDesc());

    // Create a 'dynamic' constant buffer
    const uint32_t constantBuffersMemSize = 20 * 1024 * 1024;
    m_ConstantBufferRing.OnCreate(pDevice, backBufferCount, constantBuffersMemSize, &m_resourceViewHeaps);

    // Create a 'static' pool for vertices, indices and constant buffers
    const uint32_t staticGeometryMemSize = 128 * 1024 * 1024;
    m_VidMemBufferPool.OnCreate(pDevice, staticGeometryMemSize, USE_VID_MEM, "StaticGeom");

    // initialize the GPU time stamps module
    m_GPUTimer.OnCreate(pDevice, backBufferCount);

    // Quick helper to upload resources, it has it's own commandList and uses suballocation.
    // for 4K textures we'll need 100Megs
    const uint32_t uploadHeapMemSize = 1000 * 1024 * 1024;
    m_UploadHeap.OnCreate(pDevice, uploadHeapMemSize);    // initialize an upload heap (uses suballocation for faster results)

    // Create the depth buffer views
    m_resourceViewHeaps.AllocDSVDescriptor(1, &m_depthBufferDSV);
    m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, &m_depthBufferSRV);

    // Create a Shadowmap atlas to hold 4 cascades/spotlights
	m_ShadowMap.InitDepthStencil(pDevice, "m_pShadowMap", &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, 2 * 1024, 2 * 1024, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
	m_resourceViewHeaps.AllocDSVDescriptor(1, &m_ShadowMapDSV);
    m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, &m_ShadowMapSRV);
    m_ShadowMap.CreateDSV(0, &m_ShadowMapDSV);
    m_ShadowMap.CreateSRV(0, &m_ShadowMapSRV);

    m_skyDome.OnCreate(pDevice, &m_UploadHeap, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, "..\\media\\envmaps\\papermill\\diffuse.dds", "..\\media\\envmaps\\papermill\\specular.dds", DXGI_FORMAT_R16G16B16A16_FLOAT, 4);
    m_skyDomeProc.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, DXGI_FORMAT_R16G16B16A16_FLOAT, 4);
    m_wireframe.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, DXGI_FORMAT_R16G16B16A16_FLOAT, 4);
    m_wireframeBox.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool);
    m_downSample.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_bloom.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_motionBlur.OnCreate(pDevice, &m_resourceViewHeaps, "motionBlur.hlsl", "main", 1, 2, 8, 8, 1);

	size_t cacaoSize = ffxCacaoD3D12GetContextSize();
	FfxCacaoStatus status;

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	m_pFfxCacaoContextNative = (FfxCacaoD3D12Context*)malloc(cacaoSize);
	status = ffxCacaoD3D12InitContext(m_pFfxCacaoContextNative, pDevice->GetDevice());
	assert(status == FFX_CACAO_STATUS_OK);
#endif

	m_pFfxCacaoContextDownsampled = (FfxCacaoD3D12Context*)malloc(cacaoSize);
	status = ffxCacaoD3D12InitContext(m_pFfxCacaoContextDownsampled, pDevice->GetDevice());
	assert(status == FFX_CACAO_STATUS_OK);

	D3D12_STATIC_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	SamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	SamplerDesc.MinLOD = 0.0f;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	SamplerDesc.MipLODBias = 0;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.ShaderRegister = 0;
	SamplerDesc.RegisterSpace = 0;
	SamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	m_applyDirect.OnCreate(pDevice, "Apply_CACAO.hlsl", &m_resourceViewHeaps, &m_VidMemBufferPool, 1, 1, &SamplerDesc, pSwapChain->GetFormat()); //  DXGI_FORMAT_R16G16B16A16_FLOAT);
																																				 // Create tonemapping pass
	m_cacaoUavClear.OnCreate(pDevice, &m_resourceViewHeaps, "Apply_CACAO.hlsl", "CSClear", 1, 0, 8, 8, 1);
    m_toneMapping.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, pSwapChain->GetFormat());

    // Initialize UI rendering resources
    m_ImGUI.OnCreate(pDevice, &m_UploadHeap, &m_resourceViewHeaps, &m_ConstantBufferRing, pSwapChain->GetFormat());

    m_resourceViewHeaps.AllocRTVDescriptor(1, &m_HDRRTV);
    m_resourceViewHeaps.AllocRTVDescriptor(1, &m_HDRRTVMSAA);

    m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, &m_HDRSRV);

	// CACAO stuff
	m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, &m_applyDirectInput);
	m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, &m_FfxCacaoOutputSRV);
	m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, &m_FfxCacaoOutputUAV);

	// Deferred non msaa pass
	m_resourceViewHeaps.AllocDSVDescriptor(1, &m_depthBufferNonMsaaDSV);
	m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, &m_depthBufferNonMsaaSRV);
	m_resourceViewHeaps.AllocRTVDescriptor(1, &m_normalBufferNonMsaaRTV);
	m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, &m_normalBufferNonMsaaSRV);

    // Make sure upload heap has finished uploading before continuing
#if (USE_VID_MEM==true)
    m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
    m_UploadHeap.FlushAndFinish();
#endif
}

//--------------------------------------------------------------------------------------
//
// OnDestroy
//
//--------------------------------------------------------------------------------------
void SampleRenderer::OnDestroy()
{
    m_ImGUI.OnDestroy();
    m_toneMapping.OnDestroy();
    m_taa.OnDestroy();
    m_motionBlur.OnDestroy();
    m_sharpen.OnDestroy();
    m_bloom.OnDestroy();

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	ffxCacaoD3D12DestroyContext(m_pFfxCacaoContextNative);
	free(m_pFfxCacaoContextNative);
#endif
	ffxCacaoD3D12DestroyContext(m_pFfxCacaoContextDownsampled);
	free(m_pFfxCacaoContextDownsampled);
	m_cacaoUavClear.OnDestroy();
	m_applyDirect.OnDestroy();

	m_downSample.OnDestroy();
    m_wireframeBox.OnDestroy();
    m_wireframe.OnDestroy();
    m_skyDomeProc.OnDestroy();
    m_skyDome.OnDestroy();
    m_ShadowMap.OnDestroy();
#if USE_SHADOWMASK
    m_shadowResolve.OnDestroy();
#endif

    m_UploadHeap.OnDestroy();
    m_GPUTimer.OnDestroy();
    m_VidMemBufferPool.OnDestroy();
    m_ConstantBufferRing.OnDestroy();
    m_CommandListRing.OnDestroy();
    m_resourceViewHeaps.OnDestroy();
}

//--------------------------------------------------------------------------------------
//
// OnCreateWindowSizeDependentResources
//
//--------------------------------------------------------------------------------------
void SampleRenderer::OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height)
{
    m_Width = Width;
    m_Height = Height;

    // Set the viewport
    //
    m_viewPort = { 0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height), 0.0f, 1.0f };

    // Create scissor rectangle
    //
    m_RectScissor = { 0, 0, (LONG)Width, (LONG)Height };

    // Create depth buffer
    //
	m_depthBuffer.InitDepthStencil(m_pDevice, "depthbuffer", &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, Width, Height, 1, 1, 4, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
	m_depthBuffer.CreateDSV(0, &m_depthBufferDSV);
    m_depthBuffer.CreateSRV(0, &m_depthBufferSRV);

	m_depthBufferNonMsaa.InitDepthStencil(m_pDevice, "depthBufferNonMSAA", &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, Width, Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
	m_depthBufferNonMsaa.CreateDSV(0, &m_depthBufferNonMsaaDSV);
	m_depthBufferNonMsaa.CreateSRV(0, &m_depthBufferNonMsaaSRV);

	m_normalBufferNonMsaa.InitRenderTarget(m_pDevice, "m_normalBufferNonMsaa", &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, Width, Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET));
	m_normalBufferNonMsaa.CreateRTV(0, &m_normalBufferNonMsaaRTV);
	m_normalBufferNonMsaa.CreateSRV(0, &m_normalBufferNonMsaaSRV);

    // Create Texture + RTV with x4 MSAA
    //
    CD3DX12_RESOURCE_DESC RDescMSAA = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, Width, Height, 1, 1, 4, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    m_HDRMSAA.InitRenderTarget(m_pDevice, "HDRMSAA", &RDescMSAA, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_HDRMSAA.CreateRTV(0, &m_HDRRTVMSAA);

    // Create Texture + RTV, to hold the resolved scene
    //
    CD3DX12_RESOURCE_DESC RDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, Width, Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_HDR.InitRenderTarget(m_pDevice, "HDR", &RDesc, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_HDR.CreateSRV(0, &m_HDRSRV);
    m_HDR.CreateRTV(0, &m_HDRRTV);

	m_FfxCacaoOutput.Init(m_pDevice, "cacaoOutput", &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, Width, Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_GENERIC_READ, NULL);
	m_FfxCacaoOutput.CreateSRV(0, &m_FfxCacaoOutputSRV);
	m_FfxCacaoOutput.CreateUAV(0, &m_FfxCacaoOutputUAV);

	if (m_gltfPBR)
	{
		m_gltfPBR->OnUpdateWindowSizeDependentResources(&m_FfxCacaoOutput);
	}

	FfxCacaoD3D12ScreenSizeInfo cacaoScreenSizeDependentInfo;

	cacaoScreenSizeDependentInfo.width = Width;
	cacaoScreenSizeDependentInfo.height = Height;

	cacaoScreenSizeDependentInfo.normalBufferResource = m_normalBufferNonMsaa.GetResource();
	cacaoScreenSizeDependentInfo.normalBufferSrvDesc.Format = m_normalBufferNonMsaa.GetFormat();
	cacaoScreenSizeDependentInfo.normalBufferSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	cacaoScreenSizeDependentInfo.normalBufferSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	cacaoScreenSizeDependentInfo.normalBufferSrvDesc.Texture2D.MostDetailedMip = 0;
	cacaoScreenSizeDependentInfo.normalBufferSrvDesc.Texture2D.MipLevels = 1;
	cacaoScreenSizeDependentInfo.normalBufferSrvDesc.Texture2D.PlaneSlice = 0;
	cacaoScreenSizeDependentInfo.normalBufferSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	cacaoScreenSizeDependentInfo.outputResource = m_FfxCacaoOutput.GetResource();
	cacaoScreenSizeDependentInfo.outputUavDesc.Format = m_FfxCacaoOutput.GetFormat();
	cacaoScreenSizeDependentInfo.outputUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	cacaoScreenSizeDependentInfo.outputUavDesc.Texture2D.MipSlice = 0;
	cacaoScreenSizeDependentInfo.outputUavDesc.Texture2D.PlaneSlice = 0;

	cacaoScreenSizeDependentInfo.depthBufferResource = m_depthBufferNonMsaa.GetResource();
	cacaoScreenSizeDependentInfo.depthBufferSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	cacaoScreenSizeDependentInfo.depthBufferSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	cacaoScreenSizeDependentInfo.depthBufferSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	cacaoScreenSizeDependentInfo.depthBufferSrvDesc.Texture2D.MipLevels = 1;
	cacaoScreenSizeDependentInfo.depthBufferSrvDesc.Texture2D.MostDetailedMip = 0;
	cacaoScreenSizeDependentInfo.depthBufferSrvDesc.Texture2D.PlaneSlice = 0;
	cacaoScreenSizeDependentInfo.depthBufferSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	cacaoScreenSizeDependentInfo.useDownsampledSsao = FFX_CACAO_FALSE;
	ffxCacaoD3D12InitScreenSizeDependentResources(m_pFfxCacaoContextNative, &cacaoScreenSizeDependentInfo);
	cacaoScreenSizeDependentInfo.useDownsampledSsao = FFX_CACAO_TRUE;
	ffxCacaoD3D12InitScreenSizeDependentResources(m_pFfxCacaoContextDownsampled, &cacaoScreenSizeDependentInfo);
#else
	ffxCacaoD3D12InitScreenSizeDependentResources(m_pFfxCacaoContextDownsampled, &cacaoScreenSizeDependentInfo);
#endif

	m_FfxCacaoOutput.CreateSRV(0, &m_applyDirectInput);

	m_applyDirect.UpdatePipeline(pSwapChain->GetFormat());

    // update bloom and downscaling effect
    //
    m_downSample.OnCreateWindowSizeDependentResources(m_Width, m_Height, &m_HDR, 5); //downsample the HDR texture 5 times
    m_bloom.OnCreateWindowSizeDependentResources(m_Width / 2, m_Height / 2, m_downSample.GetTexture(), 5, &m_HDR);
    m_toneMapping.UpdatePipelines(pSwapChain->GetFormat());
    m_ImGUI.UpdatePipeline(pSwapChain->GetFormat());
}

//--------------------------------------------------------------------------------------
//
// OnDestroyWindowSizeDependentResources
//
//--------------------------------------------------------------------------------------
void SampleRenderer::OnDestroyWindowSizeDependentResources()
{
    m_bloom.OnDestroyWindowSizeDependentResources();
    m_downSample.OnDestroyWindowSizeDependentResources();

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	ffxCacaoD3D12DestroyScreenSizeDependentResources(m_pFfxCacaoContextNative);
#endif
	ffxCacaoD3D12DestroyScreenSizeDependentResources(m_pFfxCacaoContextDownsampled);
	m_FfxCacaoOutput.OnDestroy();

    m_HDR.OnDestroy();
    m_HDRMSAA.OnDestroy();
    m_HistoryBuffer.OnDestroy();
    m_TAABuffer.OnDestroy();
#if USE_SHADOWMASK
    m_ShadowMask.OnDestroy();
#endif

	m_normalBufferNonMsaa.OnDestroy();
	m_depthBufferNonMsaa.OnDestroy();
    m_depthBuffer.OnDestroy();
}


//--------------------------------------------------------------------------------------
//
// LoadScene
//
//--------------------------------------------------------------------------------------
int SampleRenderer::LoadScene(GLTFCommon *pGLTFCommon, int stage)
{
    // show loading progress
    //
    ImGui::OpenPopup("Loading");
    if (ImGui::BeginPopupModal("Loading", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        float progress = (float)stage / 13.0f;
        ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), NULL);
        ImGui::EndPopup();
    }

    // Loading stages
    //
    if (stage == 0)
    {
    }
    else if (stage == 5)
    {
        Profile p("m_pGltfLoader->Load");

        m_pGLTFTexturesAndBuffers = new GLTFTexturesAndBuffers();
        m_pGLTFTexturesAndBuffers->OnCreate(m_pDevice, pGLTFCommon, &m_UploadHeap, &m_VidMemBufferPool, &m_ConstantBufferRing);
    }
    else if (stage == 6)
    {
        Profile p("LoadTextures");

        // here we are loading onto the GPU all the textures and the inverse matrices
        // this data will be used to create the PBR and Depth passes
        m_pGLTFTexturesAndBuffers->LoadTextures();
    }
    else if (stage == 7)
    {
        {
            Profile p("m_gltfDepth->OnCreate");

            //create the glTF's textures, VBs, IBs, shaders and descriptors for this particular pass
            m_gltfDepth = new GltfDepthPass();
            m_gltfDepth->OnCreate(
                m_pDevice,
                &m_UploadHeap,
                &m_resourceViewHeaps,
                &m_ConstantBufferRing,
                &m_VidMemBufferPool,
                m_pGLTFTexturesAndBuffers
            );
        }
    }
	else if (stage == 8)
	{
		Profile p("m_gltfPBR->OnCreate (Non MSAA)");

		// same thing as above but for the PBR pass
		m_gltfPBRNonMsaa = new GltfPbrPass();
		m_gltfPBRNonMsaa->OnCreate(
			m_pDevice,
			&m_UploadHeap,
			&m_resourceViewHeaps,
			&m_ConstantBufferRing,
			&m_VidMemBufferPool,
			m_pGLTFTexturesAndBuffers,
			&m_skyDome,
			false,
			false,
			DXGI_FORMAT_UNKNOWN, // Don't export forward pass
			DXGI_FORMAT_UNKNOWN, // Don't export specular roughless
			DXGI_FORMAT_UNKNOWN, // Don't export diffuse colour
			DXGI_FORMAT_R8G8B8A8_UNORM, // Export normals
			1
		);
	}
    else if (stage == 9)
    {
        Profile p("m_gltfPBR->OnCreate");

        // same thing as above but for the PBR pass
        m_gltfPBR = new GltfPbrPass();
        m_gltfPBR->OnCreate(
            m_pDevice,
            &m_UploadHeap,
            &m_resourceViewHeaps,
            &m_ConstantBufferRing,
            &m_VidMemBufferPool,
            m_pGLTFTexturesAndBuffers,
            &m_skyDome,
			true,
            false,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			4
        );
		m_gltfPBR->OnUpdateWindowSizeDependentResources(&m_FfxCacaoOutput);
    }
    else if (stage == 10)
    {
        Profile p("m_gltfBBox->OnCreate");

        // just a bounding box pass that will draw boundingboxes instead of the geometry itself
        m_gltfBBox = new GltfBBoxPass();
        m_gltfBBox->OnCreate(
            m_pDevice,
            &m_UploadHeap,
            &m_resourceViewHeaps,
            &m_ConstantBufferRing,
            &m_VidMemBufferPool,
            m_pGLTFTexturesAndBuffers,
            &m_wireframe
        );
#if (USE_VID_MEM==true)
        // we are borrowing the upload heap command list for uploading to the GPU the IBs and VBs
        m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
#endif
    }
    else if (stage == 11)
    {
        Profile p("Flush");

        m_UploadHeap.FlushAndFinish();

#if (USE_VID_MEM==true)
        //once everything is uploaded we dont need he upload heaps anymore
        m_VidMemBufferPool.FreeUploadHeap();
#endif

        // tell caller that we are done loading the map
        return 0;
    }

    stage++;
    return stage;
}

//--------------------------------------------------------------------------------------
//
// UnloadScene
//
//--------------------------------------------------------------------------------------
void SampleRenderer::UnloadScene()
{
	if (m_gltfPBRNonMsaa)
	{
		m_gltfPBRNonMsaa->OnDestroy();
		delete m_gltfPBRNonMsaa;
		m_gltfPBRNonMsaa = NULL;
	}
	
	if (m_gltfPBR)
    {
        m_gltfPBR->OnDestroy();
        delete m_gltfPBR;
        m_gltfPBR = NULL;
    }

    if (m_gltfDepth)
    {
        m_gltfDepth->OnDestroy();
        delete m_gltfDepth;
        m_gltfDepth = NULL;
    }

    if (m_gltfBBox)
    {
        m_gltfBBox->OnDestroy();
        delete m_gltfBBox;
        m_gltfBBox = NULL;
    }

    if (m_pGLTFTexturesAndBuffers)
    {
        m_pGLTFTexturesAndBuffers->OnDestroy();
        delete m_pGLTFTexturesAndBuffers;
        m_pGLTFTexturesAndBuffers = NULL;
    }

}

//--------------------------------------------------------------------------------------
//
// OnRender
//
//--------------------------------------------------------------------------------------
void SampleRenderer::OnRender(State *pState, SwapChain *pSwapChain)
{
    // Timing values
    //
    UINT64 gpuTicksPerSecond;
    m_pDevice->GetGraphicsQueue()->GetTimestampFrequency(&gpuTicksPerSecond);

    // Let our resource managers do some house keeping
    //
    m_ConstantBufferRing.OnBeginFrame();
    m_GPUTimer.OnBeginFrame(gpuTicksPerSecond, &m_TimeStamps);

    // Sets the perFrame data (Camera and lights data), override as necessary and set them as constant buffers --------------
    //
    per_frame *pPerFrame = NULL;
    if (m_pGLTFTexturesAndBuffers)
    {
        pPerFrame = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->SetPerFrameData(pState->camera);
		pPerFrame->invScreenResolution[0] = 1.0f / (float)m_Width;
		pPerFrame->invScreenResolution[1] = 1.0f / (float)m_Height;

        //apply jittering to the camera
        if (m_HasTAA)
        {
            static uint32_t sampleIndex=0;

            static const auto CalculateHaltonNumber = [](uint32_t index, uint32_t base)
            {
                float f = 1.0f, result = 0.0f;

                for (uint32_t i = index; i > 0;)
                {
                    f /= static_cast<float>(base);
                    result = result + f * static_cast<float>(i % base);
                    i = static_cast<uint32_t>(floorf(static_cast<float>(i) / static_cast<float>(base)));
                }

                return result;
            };

            sampleIndex = (sampleIndex + 1) % 16;   // 16x TAA
        }

        //override gltf camera with ours
        pPerFrame->mCameraViewProj = pState->camera.GetView() * pState->camera.GetProjection();
        pPerFrame->mInverseCameraViewProj = XMMatrixInverse(NULL, pPerFrame->mCameraViewProj);
        pPerFrame->cameraPos = pState->camera.GetPosition();
        pPerFrame->iblFactor = pState->iblFactor;
        pPerFrame->emmisiveFactor = pState->emmisiveFactor;

        //if the gltf doesn't have any lights set some spotlights
        if (pPerFrame->lightCount == 0)
        {
            pPerFrame->lightCount = pState->spotlightCount;
            for (uint32_t i = 0; i < pState->spotlightCount; i++)
            {
                GetXYZ(pPerFrame->lights[i].color, pState->spotlight[i].color);
                GetXYZ(pPerFrame->lights[i].position, pState->spotlight[i].light.GetPosition());
                GetXYZ(pPerFrame->lights[i].direction, pState->spotlight[i].light.GetDirection());

                pPerFrame->lights[i].range = 15.0f; // in meters
                pPerFrame->lights[i].type = LightType_Spot;
                pPerFrame->lights[i].intensity = pState->spotlight[i].intensity;
                pPerFrame->lights[i].innerConeCos = cosf(pState->spotlight[i].light.GetFovV() * 0.9f / 2.0f);
                pPerFrame->lights[i].outerConeCos = cosf(pState->spotlight[i].light.GetFovV() / 2.0f);
                pPerFrame->lights[i].mLightViewProj = pState->spotlight[i].light.GetView() * pState->spotlight[i].light.GetProjection();
            }
        }

        // Up to 4 spotlights can have shadowmaps. Each spot the light has a shadowMap index which is used to find the shadowmap in the atlas
        // Additionally, directional lights shadows can be raytraced.
        uint32_t shadowMapIndex = 0;
        for (uint32_t i = 0; i < pPerFrame->lightCount; i++)
        {
            if ((shadowMapIndex < 4) && (pPerFrame->lights[i].type == LightType_Spot))
            {
                pPerFrame->lights[i].shadowMapIndex = shadowMapIndex++; // set the shadowmap index so the color pass knows which shadow map to use
                pPerFrame->lights[i].depthBias = 70.0f / 100000.0f;
            }
            else
            {
                pPerFrame->lights[i].shadowMapIndex = -1;   // no shadow for this light
            }
        }

        m_pGLTFTexturesAndBuffers->SetPerFrameConstants();

        m_pGLTFTexturesAndBuffers->SetSkinningMatricesForSkeletons();
    }

    // command buffer calls
    //
    ID3D12GraphicsCommandList* pCmdLst1 = m_CommandListRing.GetNewCommandList();

    m_GPUTimer.GetTimeStamp(pCmdLst1, "Begin Frame");

    // Clear GBuffer and depth stencil
    //
    pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetCurrentBackBufferResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clears -----------------------------------------------------------------------
    //
    pCmdLst1->ClearDepthStencilView(m_ShadowMapDSV.GetCPU(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_GPUTimer.GetTimeStamp(pCmdLst1, "Clear shadow map");

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    pCmdLst1->ClearRenderTargetView(m_HDRRTVMSAA.GetCPU(), clearColor, 0, nullptr);
    m_GPUTimer.GetTimeStamp(pCmdLst1, "Clear HDR");

    pCmdLst1->ClearDepthStencilView(m_depthBufferDSV.GetCPU(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_GPUTimer.GetTimeStamp(pCmdLst1, "Clear depth");

	pCmdLst1->ClearDepthStencilView(m_depthBufferNonMsaaDSV.GetCPU(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
	m_GPUTimer.GetTimeStamp(pCmdLst1, "Clear depth (Non MSAA)");

    // Render to shadow map atlas for spot lights ------------------------------------------
    //
    if (m_gltfDepth && pPerFrame != NULL)
    {
        uint32_t shadowMapIndex = 0;
        for (uint32_t i = 0; i < pPerFrame->lightCount; i++)
        {
            if (pPerFrame->lights[i].type != LightType_Spot)
                continue;

            // Set the RT's quadrant where to render the shadomap (these viewport offsets need to match the ones in shadowFiltering.h)
            uint32_t viewportOffsetsX[4] = { 0, 1, 0, 1 };
            uint32_t viewportOffsetsY[4] = { 0, 0, 1, 1 };
            uint32_t viewportWidth = m_ShadowMap.GetWidth() / 2;
            uint32_t viewportHeight = m_ShadowMap.GetHeight() / 2;
            SetViewportAndScissor(pCmdLst1, viewportOffsetsX[i] * viewportWidth, viewportOffsetsY[i] * viewportHeight, viewportWidth, viewportHeight);
            pCmdLst1->OMSetRenderTargets(0, NULL, true, &m_ShadowMapDSV.GetCPU());

            GltfDepthPass::per_frame *cbDepthPerFrame = m_gltfDepth->SetPerFrameConstants();
            cbDepthPerFrame->mViewProj = pPerFrame->lights[i].mLightViewProj;

            m_gltfDepth->Draw(pCmdLst1);

            m_GPUTimer.GetTimeStamp(pCmdLst1, "Shadow map");
            shadowMapIndex++;
        }
    }
    pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// Render normal/depth buffer
	if (pPerFrame)
	{
		// Render Scene to the MSAA HDR RT ------------------------------------------------
		//
		pCmdLst1->RSSetViewports(1, &m_viewPort);
		pCmdLst1->RSSetScissorRects(1, &m_RectScissor);
		pCmdLst1->OMSetRenderTargets(1, &m_normalBufferNonMsaaRTV.GetCPU(), true, &m_depthBufferNonMsaaDSV.GetCPU());

		// Render normal/depth buffer
		if (m_gltfPBRNonMsaa)
		{
			m_gltfPBRNonMsaa->Draw(pCmdLst1, &m_ShadowMapSRV);
		}

		// resource barriers
		{
			D3D12_RESOURCE_BARRIER barriers[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(m_depthBufferNonMsaa.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_normalBufferNonMsaa.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			};
			pCmdLst1->ResourceBarrier(_countof(barriers), barriers);
		}

		if (pState->bUseCACAO)
		{
			FfxCacaoMatrix4x4 proj, normalsWorldToView;
			{
				XMFLOAT4X4 p;
				XMMATRIX xProj = pState->camera.GetProjection();
				XMStoreFloat4x4(&p, xProj);
				proj.elements[0][0] = p._11; proj.elements[0][1] = p._12; proj.elements[0][2] = p._13; proj.elements[0][3] = p._14;
				proj.elements[1][0] = p._21; proj.elements[1][1] = p._22; proj.elements[1][2] = p._23; proj.elements[1][3] = p._24;
				proj.elements[2][0] = p._31; proj.elements[2][1] = p._32; proj.elements[2][2] = p._33; proj.elements[2][3] = p._34;
				proj.elements[3][0] = p._41; proj.elements[3][1] = p._42; proj.elements[3][2] = p._43; proj.elements[3][3] = p._44;
				XMMATRIX xView = pState->camera.GetView();
				XMMATRIX xNormalsWorldToView = XMMATRIX(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1) * XMMatrixInverse(NULL, xView); // should be transpose(inverse(view)), but XMM is row-major and HLSL is column-major
				XMStoreFloat4x4(&p, xNormalsWorldToView);
				normalsWorldToView.elements[0][0] = p._11; normalsWorldToView.elements[0][1] = p._12; normalsWorldToView.elements[0][2] = p._13; normalsWorldToView.elements[0][3] = p._14;
				normalsWorldToView.elements[1][0] = p._21; normalsWorldToView.elements[1][1] = p._22; normalsWorldToView.elements[1][2] = p._23; normalsWorldToView.elements[1][3] = p._24;
				normalsWorldToView.elements[2][0] = p._31; normalsWorldToView.elements[2][1] = p._32; normalsWorldToView.elements[2][2] = p._33; normalsWorldToView.elements[2][3] = p._34;
				normalsWorldToView.elements[3][0] = p._41; normalsWorldToView.elements[3][1] = p._42; normalsWorldToView.elements[3][2] = p._43; normalsWorldToView.elements[3][3] = p._44;
			}

			FfxCacaoD3D12Context *context = NULL;
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
			context = pState->bUseDownsampledSSAO ? m_pFfxCacaoContextDownsampled : m_pFfxCacaoContextNative;
#else
			context = m_pFfxCacaoContextDownsampled;
#endif

			pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_FfxCacaoOutput.GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

			ffxCacaoD3D12UpdateSettings(context, &pState->cacaoSettings);
			ffxCacaoD3D12Draw(context, pCmdLst1, &proj, &normalsWorldToView);
		}
		else
		{
			pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_FfxCacaoOutput.GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
			uint32_t dummy = 0;
			D3D12_GPU_VIRTUAL_ADDRESS dummyConstantBufferAddress = m_ConstantBufferRing.AllocConstantBuffer(sizeof(dummy), &dummy);
			m_cacaoUavClear.Draw(pCmdLst1, dummyConstantBufferAddress, &m_FfxCacaoOutputUAV, NULL, m_Width, m_Height, 1);
			pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_FfxCacaoOutput.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
		}

		// resource barriers
		{
			D3D12_RESOURCE_BARRIER barriers[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(m_depthBufferNonMsaa.GetResource(),  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_normalBufferNonMsaa.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
			};
			pCmdLst1->ResourceBarrier(_countof(barriers), barriers);
		}
	}

    // Render Scene to the MSAA HDR RT ------------------------------------------------
    //
    pCmdLst1->RSSetViewports(1, &m_viewPort);
    pCmdLst1->RSSetScissorRects(1, &m_RectScissor);
    pCmdLst1->OMSetRenderTargets(1, &m_HDRRTVMSAA.GetCPU(), true, &m_depthBufferDSV.GetCPU());

    if (pPerFrame != NULL)
    {
        // Render skydome
        //
        if (pState->skyDomeType == 1)
        {
            XMMATRIX clipToView = XMMatrixInverse(NULL, pPerFrame->mCameraViewProj);
            m_skyDome.Draw(pCmdLst1, clipToView);
            m_GPUTimer.GetTimeStamp(pCmdLst1, "Skydome");
        }
        else if (pState->skyDomeType == 0)
        {
            SkyDomeProc::Constants skyDomeConstants;
            skyDomeConstants.invViewProj = XMMatrixInverse(NULL, pPerFrame->mCameraViewProj);
            skyDomeConstants.vSunDirection = XMVectorSet(1.0f, 0.05f, 0.0f, 0.0f);
            skyDomeConstants.turbidity = 10.0f;
            skyDomeConstants.rayleigh = 2.0f;
            skyDomeConstants.mieCoefficient = 0.005f;
            skyDomeConstants.mieDirectionalG = 0.8f;
            skyDomeConstants.luminance = 1.0f;
            skyDomeConstants.sun = false;
            m_skyDomeProc.Draw(pCmdLst1, skyDomeConstants);

            m_GPUTimer.GetTimeStamp(pCmdLst1, "Skydome proc");
        }

        // Render scene to color buffer
        //
        if (m_gltfPBR && pPerFrame != NULL)
        {
            //set per frame constant buffer values
            m_gltfPBR->Draw(pCmdLst1, &m_ShadowMapSRV);
        }

        // draw object's bounding boxes
        //
        if (m_gltfBBox && pPerFrame != NULL)
        {
            if (pState->bDrawBoundingBoxes)
            {
                m_gltfBBox->Draw(pCmdLst1, pPerFrame->mCameraViewProj);

                m_GPUTimer.GetTimeStamp(pCmdLst1, "Bounding Box");
            }
        }

        // draw light's frustums
        //
        if (pState->bDrawLightFrustum && pPerFrame != NULL)
        {
            UserMarker marker(pCmdLst1, "light frustrums");

            XMVECTOR vCenter = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            XMVECTOR vRadius = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
            XMVECTOR vColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
            for (uint32_t i = 0; i < pPerFrame->lightCount; i++)
            {
                XMMATRIX spotlightMatrix = XMMatrixInverse(NULL, pPerFrame->lights[i].mLightViewProj);
                XMMATRIX worldMatrix = spotlightMatrix * pPerFrame->mCameraViewProj;
                m_wireframeBox.Draw(pCmdLst1, &m_wireframe, worldMatrix, vCenter, vRadius, vColor);
            }

            m_GPUTimer.GetTimeStamp(pCmdLst1, "Light's frustum");
        }
    }
    pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
    // pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthBuffer.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    m_GPUTimer.GetTimeStamp(pCmdLst1, "Rendering scene");

    // Resolve MSAA ------------------------------------------------------------------------
    //
    {
        UserMarker marker(pCmdLst1, "Resolving MSAA");

        D3D12_RESOURCE_BARRIER preResolve[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_HDR.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_DEST),
            CD3DX12_RESOURCE_BARRIER::Transition(m_HDRMSAA.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
        };
        pCmdLst1->ResourceBarrier(2, preResolve);

        pCmdLst1->ResolveSubresource(m_HDR.GetResource(), 0, m_HDRMSAA.GetResource(), 0, DXGI_FORMAT_R16G16B16A16_FLOAT);

        D3D12_RESOURCE_BARRIER postResolve[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_HDR.GetResource(), D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_HDRMSAA.GetResource(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
        };
        pCmdLst1->ResourceBarrier(2, postResolve);

        m_GPUTimer.GetTimeStamp(pCmdLst1, "Resolve MSAA");
    }

    // Post proc---------------------------------------------------------------------------
    //
	if (0)
    {
        // Bloom, takes HDR as input and applies bloom to it.
        //
        {
            D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[] = { m_HDRRTV.GetCPU() };
            pCmdLst1->OMSetRenderTargets(ARRAYSIZE(renderTargets), renderTargets, false, NULL);

            m_downSample.Draw(pCmdLst1);
            //m_downSample.Gui();
            m_GPUTimer.GetTimeStamp(pCmdLst1, "Downsample");

            m_bloom.Draw(pCmdLst1, &m_HDR);
            //m_bloom.Gui();
            m_GPUTimer.GetTimeStamp(pCmdLst1, "Bloom");
        }
    }

    // submit command buffer

    ThrowIfFailed(pCmdLst1->Close());
    ID3D12CommandList* CmdListList1[] = { pCmdLst1 };
    m_pDevice->GetGraphicsQueue()->ExecuteCommandLists(1, CmdListList1);

    // Wait for swapchain (we are going to render to it) -----------------------------------
    //
    pSwapChain->WaitForSwapChain();

    m_CommandListRing.OnBeginFrame();

    ID3D12GraphicsCommandList* pCmdLst2 = m_CommandListRing.GetNewCommandList();

    // Tonemapping ------------------------------------------------------------------------
    //
	if (pState->bDisplayCacaoDirectly)
	{
		pCmdLst2->RSSetViewports(1, &m_viewPort);
		pCmdLst2->RSSetScissorRects(1, &m_RectScissor);
		pCmdLst2->OMSetRenderTargets(1, pSwapChain->GetCurrentBackBufferRTV(), true, NULL);

		m_applyDirect.Draw(pCmdLst2, 1, &m_applyDirectInput, NULL);
	}
	else
	{
        pCmdLst2->RSSetViewports(1, &m_viewPort);
        pCmdLst2->RSSetScissorRects(1, &m_RectScissor);
        pCmdLst2->OMSetRenderTargets(1, pSwapChain->GetCurrentBackBufferRTV(), true, NULL);

        m_toneMapping.Draw(pCmdLst2, &m_HDRSRV, pState->exposure, pState->toneMapper);
        m_GPUTimer.GetTimeStamp(pCmdLst2, "Tone mapping");

        pCmdLst2->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_HDR.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
    }

    // Render HUD  ------------------------------------------------------------------------
    //
    {
        pCmdLst2->RSSetViewports(1, &m_viewPort);
        pCmdLst2->RSSetScissorRects(1, &m_RectScissor);
        pCmdLst2->OMSetRenderTargets(1, pSwapChain->GetCurrentBackBufferRTV(), true, NULL);

		m_ImGUI.Draw(pCmdLst2);

        m_GPUTimer.GetTimeStamp(pCmdLst2, "ImGUI rendering");
    }

    // Transition swapchain into present mode

    pCmdLst2->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetCurrentBackBufferResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    m_GPUTimer.OnEndFrame();

    m_GPUTimer.CollectTimings(pCmdLst2);

    // Close & Submit the command list ----------------------------------------------------
    //
    ThrowIfFailed(pCmdLst2->Close());

    ID3D12CommandList* CmdListList2[] = { pCmdLst2 };
    m_pDevice->GetGraphicsQueue()->ExecuteCommandLists(1, CmdListList2);
}

#ifdef FFX_CACAO_ENABLE_PROFILING
void SampleRenderer::GetCacaoTimings(State *pState, FfxCacaoDetailedTiming* timings, uint64_t* gpuTicksPerSecond)
{
	FfxCacaoD3D12Context *context = NULL;
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	context = pState->bUseDownsampledSSAO ? m_pFfxCacaoContextDownsampled : m_pFfxCacaoContextNative;
#else
	context = m_pFfxCacaoContextDownsampled;
#endif

	ffxCacaoD3D12GetDetailedTimings(context, timings);
	m_pDevice->GetGraphicsQueue()->GetTimestampFrequency(gpuTicksPerSecond);
}
#endif
