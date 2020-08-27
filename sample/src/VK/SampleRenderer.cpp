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

#include "stdafx.h"

#include "SampleRenderer.h"

//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void SampleRenderer::OnCreate(Device *pDevice, SwapChain *pSwapChain)
{
    m_pDevice = pDevice;

    // Initialize helpers

    // Create all the heaps for the resources views
    const uint32_t cbvDescriptorCount = 2000;
    const uint32_t srvDescriptorCount = 2000;
    const uint32_t uavDescriptorCount = 10;
    const uint32_t samplerDescriptorCount = 20;
    m_resourceViewHeaps.OnCreate(pDevice, cbvDescriptorCount, srvDescriptorCount, uavDescriptorCount, samplerDescriptorCount);

    // Create a commandlist ring for the Direct queue
    uint32_t commandListsPerBackBuffer = 8;
    m_CommandListRing.OnCreate(pDevice, backBufferCount, commandListsPerBackBuffer);

    // Create a 'dynamic' constant buffer
    const uint32_t constantBuffersMemSize = 20 * 1024 * 1024;
    m_ConstantBufferRing.OnCreate(pDevice, backBufferCount, constantBuffersMemSize, "Uniforms");

    // Create a 'static' pool for vertices and indices
    const uint32_t staticGeometryMemSize = 128 * 1024 * 1024;
    const uint32_t systemGeometryMemSize = 32 * 1024;
    m_VidMemBufferPool.OnCreate(pDevice, staticGeometryMemSize, USE_VID_MEM, "StaticGeom");
    m_SysMemBufferPool.OnCreate(pDevice, systemGeometryMemSize, false, "PostProcGeom");

    // initialize the GPU time stamps module
    m_GPUTimer.OnCreate(pDevice, backBufferCount);

    // Quick helper to upload resources, it has it's own commandList and uses suballocation.
    // for 4K textures we'll need 100Megs
    const uint32_t uploadHeapMemSize = 1000 * 1024 * 1024;
    m_UploadHeap.OnCreate(pDevice, staticGeometryMemSize);    // initialize an upload heap (uses suballocation for faster results)

    // Create a 2Kx2K Shadowmap atlas to hold 4 cascades/spotlights
    m_shadowMap.InitDepthStencil(m_pDevice, 2 * 1024, 2 * 1024, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, "ShadowMap");
    m_shadowMap.CreateSRV(&m_shadowMapSRV);
    m_shadowMap.CreateDSV(&m_shadowMapDSV);

    // Create render pass shadow, will clear contents
    //
    {
        VkAttachmentDescription depthAttachments;
        AttachClearBeforeUse(m_shadowMap.GetFormat(), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &depthAttachments);
        m_render_pass_shadow = CreateRenderPassOptimal(m_pDevice->GetDevice(), 0, NULL, &depthAttachments);

        // Create frame buffer, its size is now window dependant so we can do this here.
        //
        VkImageView attachmentViews[1] = { m_shadowMapDSV };
        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.pNext = NULL;
        fb_info.renderPass = m_render_pass_shadow;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = attachmentViews;
        fb_info.width = m_shadowMap.GetWidth();
        fb_info.height = m_shadowMap.GetHeight();
        fb_info.layers = 1;
        VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_pFrameBuffer_shadow);
        assert(res == VK_SUCCESS);
    }

    // Create HDR MSAA render pass + clear, for the sky, PBR and Wireframe passes
    //
    {
        VkAttachmentDescription colorAttachment, depthAttachment;
        AttachClearBeforeUse(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_4_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachment);
		AttachClearBeforeUse(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_4_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &depthAttachment);
        m_render_pass_HDR_MSAA = CreateRenderPassOptimal(m_pDevice->GetDevice(), 1, &colorAttachment, &depthAttachment);
    }

	// Create non msaa render pass
	//
	{
		VkAttachmentDescription colorAttachment, depthAttachment;
		AttachClearBeforeUse(VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &colorAttachment);
		AttachClearBeforeUse(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &depthAttachment);
		m_render_pass_non_msaa = CreateRenderPassOptimal(m_pDevice->GetDevice(), 1, &colorAttachment, &depthAttachment);
	}

    // Create HDR render pass, for the GUI
    //
    {
        VkAttachmentDescription colorAttachment;
        AttachBlending(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &colorAttachment);
        m_render_pass_PBR_HDR = CreateRenderPassOptimal(m_pDevice->GetDevice(), 1, &colorAttachment, NULL);
    }

    m_skyDome.OnCreate(pDevice, m_render_pass_HDR_MSAA, &m_UploadHeap, VK_FORMAT_R16G16B16A16_SFLOAT, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, "..\\media\\envmaps\\papermill\\diffuse.dds", "..\\media\\envmaps\\papermill\\specular.dds", VK_SAMPLE_COUNT_4_BIT);
    m_skyDomeProc.OnCreate(pDevice, m_render_pass_HDR_MSAA, &m_UploadHeap, VK_FORMAT_R16G16B16A16_SFLOAT, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, VK_SAMPLE_COUNT_4_BIT);
    m_wireframe.OnCreate(pDevice, m_render_pass_HDR_MSAA, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, VK_SAMPLE_COUNT_4_BIT);
    m_wireframeBox.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool);
    m_downSample.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, VK_FORMAT_R16G16B16A16_SFLOAT);
    m_bloom.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, VK_FORMAT_R16G16B16A16_SFLOAT);

    // Create tonemapping pass
    m_toneMappingCS.OnCreate(pDevice, &m_resourceViewHeaps, &m_ConstantBufferRing);
    m_toneMappingPS.OnCreate(m_pDevice, pSwapChain->GetRenderPass(), &m_resourceViewHeaps, &m_VidMemBufferPool, &m_ConstantBufferRing);
    m_colorConversionPS.OnCreate(pDevice, pSwapChain->GetRenderPass(), &m_resourceViewHeaps, &m_VidMemBufferPool, &m_ConstantBufferRing);

    // Initialize UI rendering resources
    m_ImGUI.OnCreate(m_pDevice, pSwapChain->GetRenderPass(), &m_UploadHeap, &m_ConstantBufferRing);

    // Make sure upload heap has finished uploading before continuing
#if (USE_VID_MEM==true)
    m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
    m_UploadHeap.FlushAndFinish();
#endif

	// =======================================================================
	// CACAO

	size_t cacaoContextSize = ffxCacaoVkGetContextSize();
	FfxCacaoVkCreateInfo info = {};
	info.physicalDevice = pDevice->GetPhysicalDevice();
	info.device = pDevice->GetDevice();
	info.flags = FFX_CACAO_VK_CREATE_USE_DEBUG_MARKERS | FFX_CACAO_VK_CREATE_NAME_OBJECTS;
	if (pDevice->IsFp16Supported())
	{
		info.flags |= FFX_CACAO_VK_CREATE_USE_16_BIT;
	}
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	m_cacaoContextNative = (FfxCacaoVkContext*)malloc(cacaoContextSize);
	ffxCacaoVkInitContext(m_cacaoContextNative, &info);
#endif
	m_cacaoContextDownsampled = (FfxCacaoVkContext*)malloc(cacaoContextSize);
	ffxCacaoVkInitContext(m_cacaoContextDownsampled, &info);

	// create direct output PS descriptor set layout
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings(2);

		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[0].pImmutableSamplers = NULL;

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].pImmutableSamplers = NULL;

		bool succeeded = m_resourceViewHeaps.CreateDescriptorSetLayout(&bindings, &m_directOutputDescriptorSetLayout);
		assert(succeeded);
	}

	// create direct output PS sampler
	{
		VkSamplerCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.magFilter = VK_FILTER_LINEAR;
		info.minFilter = VK_FILTER_LINEAR;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.minLod = -1000;
		info.maxLod = 1000;
		info.maxAnisotropy = 1.0f;
		VkResult res = vkCreateSampler(m_pDevice->GetDevice(), &info, NULL, &m_directOutputSampler);
		assert(res == VK_SUCCESS);
	}

	// alloc direct output PS descriptor sets
	for (uint32_t i = 0; i < _countof(m_directOutputDescriptorSets); ++i)
	{
		m_resourceViewHeaps.AllocDescriptor(m_directOutputDescriptorSetLayout, &m_directOutputDescriptorSets[i]);
	}

	m_directOutputPS.OnCreate(m_pDevice, pSwapChain->GetRenderPass(), "Apply_CACAO_Direct.glsl", "main", "", &m_VidMemBufferPool, &m_ConstantBufferRing, m_directOutputDescriptorSetLayout);
}

//--------------------------------------------------------------------------------------
//
// OnDestroy
//
//--------------------------------------------------------------------------------------
void SampleRenderer::OnDestroy()
{
	m_directOutputPS.OnDestroy();
	vkDestroySampler(m_pDevice->GetDevice(), m_directOutputSampler, NULL);

	ffxCacaoVkDestroyContext(m_cacaoContextDownsampled);
	free(m_cacaoContextDownsampled);
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	ffxCacaoVkDestroyContext(m_cacaoContextNative);
	free(m_cacaoContextNative);
#endif

    m_ImGUI.OnDestroy();
    m_colorConversionPS.OnDestroy();
    m_toneMappingPS.OnDestroy();
    m_toneMappingCS.OnDestroy();
    m_bloom.OnDestroy();
    m_downSample.OnDestroy();
    m_wireframeBox.OnDestroy();
    m_wireframe.OnDestroy();
    m_skyDomeProc.OnDestroy();
    m_skyDome.OnDestroy();
    m_shadowMap.OnDestroy();

    vkDestroyImageView(m_pDevice->GetDevice(), m_shadowMapDSV, nullptr);
    vkDestroyImageView(m_pDevice->GetDevice(), m_shadowMapSRV, nullptr);

	vkDestroyRenderPass(m_pDevice->GetDevice(), m_render_pass_non_msaa, NULL);
    vkDestroyRenderPass(m_pDevice->GetDevice(), m_render_pass_shadow, nullptr);
    vkDestroyRenderPass(m_pDevice->GetDevice(), m_render_pass_PBR_HDR, nullptr);
    vkDestroyRenderPass(m_pDevice->GetDevice(), m_render_pass_HDR_MSAA, nullptr);

    vkDestroyFramebuffer(m_pDevice->GetDevice(), m_pFrameBuffer_shadow, nullptr);

    m_UploadHeap.OnDestroy();
    m_GPUTimer.OnDestroy();
    m_VidMemBufferPool.OnDestroy();
    m_SysMemBufferPool.OnDestroy();
    m_ConstantBufferRing.OnDestroy();
    m_resourceViewHeaps.OnDestroy();
    m_CommandListRing.OnDestroy();
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
    m_viewport.x = 0;
    m_viewport.y = (float)Height;
    m_viewport.width = (float)Width;
    m_viewport.height = -(float)(Height);
    m_viewport.minDepth = (float)0.0f;
    m_viewport.maxDepth = (float)1.0f;

    // Create scissor rectangle
    //
    m_rectScissor.extent.width = Width;
    m_rectScissor.extent.height = Height;
    m_rectScissor.offset.x = 0;
    m_rectScissor.offset.y = 0;

    // Create depth buffer
    //
	{
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.pNext = NULL;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = VK_FORMAT_D32_SFLOAT;
		image_info.extent.width = Width;
		image_info.extent.height = Height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.samples = VK_SAMPLE_COUNT_4_BIT;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.queueFamilyIndexCount = 0;
		image_info.pQueueFamilyIndices = NULL;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT; //TODO
		image_info.flags = 0;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		m_depthBuffer.Init(m_pDevice, &image_info, "DepthBuffer");
	}
    m_depthBuffer.CreateDSV(&m_depthBufferDSV);

    // Create Texture + RTV with x4 MSAA
    //
    m_HDRMSAA.InitRenderTarget(m_pDevice, m_Width, m_Height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_4_BIT, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT), false, "HDRMSAA");
    m_HDRMSAA.CreateRTV(&m_HDRMSAASRV);

    // Create Texture + RTV, to hold the resolved scene
    //
    m_HDR.InitRenderTarget(m_pDevice, m_Width, m_Height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "HDR");
    m_HDR.CreateSRV(&m_HDRSRV);
    m_HDR.CreateSRV(&m_HDRUAV);

    // Create framebuffer for the MSAA RT
    //
    {
        VkImageView attachments_PBR_HDR_MSAA[] = { m_HDRMSAASRV, m_depthBufferDSV };
        VkImageView attachments_PBR_HDR[1] = { m_HDRSRV };

        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.pNext = NULL;
        fb_info.attachmentCount = _countof(attachments_PBR_HDR_MSAA);
        fb_info.pAttachments = attachments_PBR_HDR_MSAA;
        fb_info.width = Width;
        fb_info.height = Height;
        fb_info.layers = 1;

        VkResult res;

        fb_info.renderPass = m_render_pass_HDR_MSAA;
        res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_pFrameBuffer_HDR_MSAA);
        assert(res == VK_SUCCESS);

        fb_info.attachmentCount = 1;
        fb_info.pAttachments = attachments_PBR_HDR;
        fb_info.renderPass = m_render_pass_PBR_HDR;
        res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_pFrameBuffer_PBR_HDR);
        assert(res == VK_SUCCESS);
    }

    // update bloom and downscaling effect
    //
    m_downSample.OnCreateWindowSizeDependentResources(m_Width, m_Height, &m_HDR, 6); //downsample the HDR texture 6 times
    m_bloom.OnCreateWindowSizeDependentResources(m_Width / 2, m_Height / 2, m_downSample.GetTexture(), 6, &m_HDR);

    // update the pipelines if the swapchain render pass has changed (for example when the format of the swapchain changes)
    //
    m_colorConversionPS.UpdatePipelines(pSwapChain->GetRenderPass(), pSwapChain->GetDisplayMode());
    m_toneMappingPS.UpdatePipelines(pSwapChain->GetRenderPass());

    m_ImGUI.UpdatePipeline((pSwapChain->GetDisplayMode() == DISPLAYMODE_SDR) ? pSwapChain->GetRenderPass() : m_render_pass_PBR_HDR);

	// ==========================================================
	// CACAO

	m_NormalBufferNonMsaa.InitRenderTarget(m_pDevice, m_Width, m_Height, VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_SAMPLE_COUNT_1_BIT, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT), false, "NormalBufferNonMsaa");
	m_NormalBufferNonMsaa.CreateRTV(&m_NormalBufferNonMsaaView);

	m_DepthBufferNonMsaa.InitDepthStencil(m_pDevice, Width, Height, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, "DepthBufferNonMsaa");
	m_DepthBufferNonMsaa.CreateSRV(&m_DepthBufferNonMsaaView);

	// Create framebuffer for the MSAA RT
	//
	{
		VkImageView attachments[] = { m_NormalBufferNonMsaaView, m_DepthBufferNonMsaaView };

		VkFramebufferCreateInfo fb_info = {};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.pNext = NULL;
		fb_info.attachmentCount = _countof(attachments);
		fb_info.pAttachments = attachments;
		fb_info.width = Width;
		fb_info.height = Height;
		fb_info.layers = 1;
		fb_info.renderPass = m_render_pass_non_msaa;

		VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_pFrameBuffer_non_msaa);
		assert(res == VK_SUCCESS);
	}

	// create cacao output image
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = NULL;
		info.flags = 0;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R32_SFLOAT;
		info.extent.width = Width;
		info.extent.height = Height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.queueFamilyIndexCount = 0;
		info.pQueueFamilyIndices = NULL;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_cacaoOutput.Init(m_pDevice, &info, "FFX CACAO Output");
	}
	m_cacaoOutput.CreateSRV(&m_cacaoOutputSRV);
	if (m_gltfPBR)
	{
		m_gltfPBR->OnUpdateWindowSizeDependentResources(m_cacaoOutputSRV);
	}

	FfxCacaoVkScreenSizeInfo ssi = {};

	ssi.width = Width;
	ssi.height = Height;
	ssi.depthView = m_DepthBufferNonMsaaView;
	ssi.normalsView = m_NormalBufferNonMsaaView;
	ssi.output = m_cacaoOutput.Resource();
	ssi.outputView = m_cacaoOutputSRV;

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	ssi.useDownsampledSsao = FFX_CACAO_TRUE;
	ffxCacaoVkInitScreenSizeDependentResources(m_cacaoContextDownsampled, &ssi);
	ssi.useDownsampledSsao = FFX_CACAO_FALSE;
	ffxCacaoVkInitScreenSizeDependentResources(m_cacaoContextNative, &ssi);
#else
	ffxCacaoVkInitScreenSizeDependentResources(m_cacaoContextDownsampled, &ssi);
#endif

	m_directOutputPS.UpdatePipeline(pSwapChain->GetRenderPass());
}

//--------------------------------------------------------------------------------------
//
// OnDestroyWindowSizeDependentResources
//
//--------------------------------------------------------------------------------------
void SampleRenderer::OnDestroyWindowSizeDependentResources()
{
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	ffxCacaoVkDestroyScreenSizeDependentResources(m_cacaoContextNative);
#endif
	ffxCacaoVkDestroyScreenSizeDependentResources(m_cacaoContextDownsampled);

	vkDestroyImageView(m_pDevice->GetDevice(), m_NormalBufferNonMsaaView, NULL);
	m_NormalBufferNonMsaa.OnDestroy();
	vkDestroyImageView(m_pDevice->GetDevice(), m_DepthBufferNonMsaaView, NULL);
	m_DepthBufferNonMsaa.OnDestroy();

	vkDestroyImageView(m_pDevice->GetDevice(), m_cacaoOutputSRV, NULL);
	m_cacaoOutput.OnDestroy();

    m_bloom.OnDestroyWindowSizeDependentResources();
    m_downSample.OnDestroyWindowSizeDependentResources();

    m_HDR.OnDestroy();
    m_HDRMSAA.OnDestroy();
    m_depthBuffer.OnDestroy();

	vkDestroyFramebuffer(m_pDevice->GetDevice(), m_pFrameBuffer_non_msaa, NULL);
    vkDestroyFramebuffer(m_pDevice->GetDevice(), m_pFrameBuffer_HDR_MSAA, nullptr);
    vkDestroyFramebuffer(m_pDevice->GetDevice(), m_pFrameBuffer_PBR_HDR, nullptr);

    vkDestroyImageView(m_pDevice->GetDevice(), m_depthBufferDSV, nullptr);
    vkDestroyImageView(m_pDevice->GetDevice(), m_HDRMSAASRV, nullptr);
    vkDestroyImageView(m_pDevice->GetDevice(), m_HDRSRV, nullptr);
    vkDestroyImageView(m_pDevice->GetDevice(), m_HDRUAV, nullptr);
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
        float progress = (float)stage / 12.0f;
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
        Profile p("m_gltfDepth->OnCreate");

        //create the glTF's textures, VBs, IBs, shaders and descriptors for this particular pass
        m_gltfDepth = new GltfDepthPass();
        m_gltfDepth->OnCreate(
            m_pDevice,
            m_render_pass_shadow,
            &m_UploadHeap,
            &m_resourceViewHeaps,
            &m_ConstantBufferRing,
            &m_VidMemBufferPool,
            m_pGLTFTexturesAndBuffers
        );

#if (USE_VID_MEM==true)
        m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
        m_UploadHeap.FlushAndFinish();
#endif
    }
    else if (stage == 8)
    {
        Profile p("m_gltfPBR->OnCreate");

        // same thing as above but for the PBR pass
        m_gltfPBR = new GltfPbrPass();
        m_gltfPBR->OnCreate(
            m_pDevice,
            m_render_pass_HDR_MSAA,
            &m_UploadHeap,
            &m_resourceViewHeaps,
            &m_ConstantBufferRing,
            &m_VidMemBufferPool,
            m_pGLTFTexturesAndBuffers,
            &m_skyDome,
			true, // we will pass in a buffer with AO
            m_shadowMapSRV,
            true,  // Exports ForwardPass
            false, // Won't export Specular Roughness
            false, // Won't export Diffuse Color
			false, // export normals
            VK_SAMPLE_COUNT_4_BIT
        );
		m_gltfPBR->OnUpdateWindowSizeDependentResources(m_cacaoOutputSRV);
#if (USE_VID_MEM==true)
        m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
        m_UploadHeap.FlushAndFinish();
#endif
    }
	else if (stage == 9)
	{
		Profile p("m_gltfPBR->OnCreate (Non MSAA)");

		m_gltfPbrNonMsaa = new GltfPbrPass();
		m_gltfPbrNonMsaa->OnCreate(
			m_pDevice,
			m_render_pass_non_msaa,
			&m_UploadHeap,
			&m_resourceViewHeaps,
			&m_ConstantBufferRing,
			&m_VidMemBufferPool,
			m_pGLTFTexturesAndBuffers,
			&m_skyDome,
			false, // We won't pass in a buffer with AO
			m_shadowMapSRV,
			false, // Won't export ForwardPass
			false, // Won't export Specular Roughness
			false, // Won't export Diffuse Color
			true,  // export normals
			VK_SAMPLE_COUNT_1_BIT
		);
	}
    else if (stage == 10)
    {
        Profile p("m_gltfBBox->OnCreate");

        // just a bounding box pass that will draw boundingboxes instead of the geometry itself
        m_gltfBBox = new GltfBBoxPass();
            m_gltfBBox->OnCreate(
            m_pDevice,
            m_render_pass_HDR_MSAA,
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
        //once everything is uploaded we dont need the upload heaps anymore
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
    m_pDevice->GPUFlush();

    if (m_gltfPBR)
    {
        m_gltfPBR->OnDestroy();
        delete m_gltfPBR;
        m_gltfPBR = NULL;
    }

	if (m_gltfPbrNonMsaa)
	{
		m_gltfPbrNonMsaa->OnDestroy();
		delete m_gltfPbrNonMsaa;
		m_gltfPbrNonMsaa = NULL;
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
    // Let our resource managers do some house keeping
    //
    m_ConstantBufferRing.OnBeginFrame();

    // command buffer calls
    //
    VkCommandBuffer cmdBuf1 = m_CommandListRing.GetNewCommandList();

    {
        VkCommandBufferBeginInfo cmd_buf_info;
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = NULL;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmd_buf_info.pInheritanceInfo = NULL;
        VkResult res = vkBeginCommandBuffer(cmdBuf1, &cmd_buf_info);
        assert(res == VK_SUCCESS);
    }

    m_GPUTimer.OnBeginFrame(cmdBuf1, &m_TimeStamps);

    // Sets the perFrame data
    //
    per_frame *pPerFrame = NULL;
    if (m_pGLTFTexturesAndBuffers)
    {
        // fill as much as possible using the GLTF (camera, lights, ...)
        pPerFrame = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->SetPerFrameData(pState->camera);

        // Set some lighting factors
        pPerFrame->iblFactor = pState->iblFactor;
        pPerFrame->emmisiveFactor = pState->emmisiveFactor;
		pPerFrame->invScreenResolution[0] = 1.0f / ((float)m_Width);
		pPerFrame->invScreenResolution[1] = 1.0f / ((float)m_Height);

        // Set shadowmaps bias and an index that indicates the rectangle of the atlas in which depth will be rendered
        uint32_t shadowMapIndex = 0;
        for (uint32_t i = 0; i < pPerFrame->lightCount; i++)
        {
            if ((shadowMapIndex < 4) && (pPerFrame->lights[i].type == LightType_Spot))
            {
                pPerFrame->lights[i].shadowMapIndex = shadowMapIndex++; // set the shadowmap index
                pPerFrame->lights[i].depthBias = 70.0f / 100000.0f;
            }
            else if ((shadowMapIndex < 4) && (pPerFrame->lights[i].type == LightType_Directional))
            {
                pPerFrame->lights[i].shadowMapIndex = shadowMapIndex++; // set the shadowmap index
                pPerFrame->lights[i].depthBias = 1000.0f / 100000.0f;
            }
            else
            {
                pPerFrame->lights[i].shadowMapIndex = -1;   // no shadow for this light
            }
        }

        m_pGLTFTexturesAndBuffers->SetPerFrameConstants();
        m_pGLTFTexturesAndBuffers->SetSkinningMatricesForSkeletons();
    }

    // Render to shadow map atlas for spot lights ------------------------------------------
    //
    if (m_gltfDepth && pPerFrame != NULL)
    {
        SetPerfMarkerBegin(cmdBuf1, "ShadowPass");

        VkClearValue depth_clear_values[1];
        depth_clear_values[0].depthStencil.depth = 1.0f;
        depth_clear_values[0].depthStencil.stencil = 0;

        {
            VkRenderPassBeginInfo rp_begin;
            rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rp_begin.pNext = NULL;
            rp_begin.renderPass = m_render_pass_shadow;
            rp_begin.framebuffer = m_pFrameBuffer_shadow;
            rp_begin.renderArea.offset.x = 0;
            rp_begin.renderArea.offset.y = 0;
            rp_begin.renderArea.extent.width = m_shadowMap.GetWidth();
            rp_begin.renderArea.extent.height = m_shadowMap.GetHeight();
            rp_begin.clearValueCount = 1;
            rp_begin.pClearValues = depth_clear_values;

            vkCmdBeginRenderPass(cmdBuf1, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
            m_GPUTimer.GetTimeStamp(cmdBuf1, "Clear Shadow Map");
        }

        uint32_t shadowMapIndex = 0;
        for (uint32_t i = 0; i < pPerFrame->lightCount; i++)
        {
            if (!(pPerFrame->lights[i].type == LightType_Spot || pPerFrame->lights[i].type == LightType_Directional))
                continue;

            // Set the RT's quadrant where to render the shadomap (these viewport offsets need to match the ones in shadowFiltering.h)
            uint32_t viewportOffsetsX[4] = { 0, 1, 0, 1 };
            uint32_t viewportOffsetsY[4] = { 0, 0, 1, 1 };
            uint32_t viewportWidth = m_shadowMap.GetWidth() / 2;
            uint32_t viewportHeight = m_shadowMap.GetHeight() / 2;
            SetViewportAndScissor(cmdBuf1, viewportOffsetsX[shadowMapIndex] * viewportWidth, viewportOffsetsY[shadowMapIndex] * viewportHeight, viewportWidth, viewportHeight);

            //set per frame constant buffer values
            GltfDepthPass::per_frame *cbPerFrame = m_gltfDepth->SetPerFrameConstants();
            cbPerFrame->mViewProj = pPerFrame->lights[i].mLightViewProj;

            m_gltfDepth->Draw(cmdBuf1);

            m_GPUTimer.GetTimeStamp(cmdBuf1, "Shadow maps");
            shadowMapIndex++;
        }
        vkCmdEndRenderPass(cmdBuf1);

        SetPerfMarkerEnd(cmdBuf1);
    }

	// ===============================================================================================
	// CACAO stuff

	m_curBackBuffer = (m_curBackBuffer + 1) % backBufferCount;

	// Create depth and normal buffer
	//
	if (m_gltfPbrNonMsaa && pPerFrame)
	{
		m_GPUTimer.GetTimeStamp(cmdBuf1, "PBR Non MSAA");
		SetPerfMarkerBegin(cmdBuf1, "PBR Non MSAA pass");

		{
			VkClearValue clearValues[2];
			clearValues[0].color.float32[0] = 0.0f;
			clearValues[0].color.float32[1] = 0.0f;
			clearValues[0].color.float32[2] = 0.0f;
			clearValues[0].color.float32[3] = 0.0f;
			clearValues[1].depthStencil.depth = 1.0f;
			clearValues[1].depthStencil.stencil = 0;

			VkRenderPassBeginInfo rp_begin;
			rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin.pNext = NULL;
			rp_begin.renderPass = m_render_pass_non_msaa;
			rp_begin.framebuffer = m_pFrameBuffer_non_msaa;
			rp_begin.renderArea.offset.x = 0;
			rp_begin.renderArea.offset.y = 0;
			rp_begin.renderArea.extent.width = m_NormalBufferNonMsaa.GetWidth();
			rp_begin.renderArea.extent.height = m_NormalBufferNonMsaa.GetHeight();
			rp_begin.clearValueCount = _countof(clearValues);
			rp_begin.pClearValues = clearValues;

			vkCmdBeginRenderPass(cmdBuf1, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
			m_GPUTimer.GetTimeStamp(cmdBuf1, "Clear Depth Buffer Non MSAA");
		}

		SetViewportAndScissor(cmdBuf1, 0, 0, m_NormalBufferNonMsaa.GetWidth(), m_NormalBufferNonMsaa.GetHeight());

		m_gltfPbrNonMsaa->Draw(cmdBuf1);
		m_GPUTimer.GetTimeStamp(cmdBuf1, "GLTF PBR Non MSAA");

		vkCmdEndRenderPass(cmdBuf1);

		SetPerfMarkerEnd(cmdBuf1);
	}

	// call CACAO
	if (pState->m_useCacao && m_gltfPbrNonMsaa && pPerFrame) {
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

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
		ffxCacaoVkUpdateSettings(m_cacaoContextNative, &pState->m_cacaoSettings);
#endif
		ffxCacaoVkUpdateSettings(m_cacaoContextDownsampled, &pState->m_cacaoSettings);

		FfxCacaoStatus status = FFX_CACAO_STATUS_OK;
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
		if (pState->m_useDownsampledSsao)
		{
			status = ffxCacaoVkDraw(m_cacaoContextDownsampled, cmdBuf1, &proj, &normalsWorldToView);
		}
		else
		{
			status = ffxCacaoVkDraw(m_cacaoContextNative, cmdBuf1, &proj, &normalsWorldToView);
		}
#else
		status = ffxCacaoVkDraw(m_cacaoContextDownsampled, cmdBuf1, &proj, &normalsWorldToView);
#endif
		assert(status == FFX_CACAO_STATUS_OK);
	}
	else
	{
		// clear the cacao output buffer
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = NULL;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_cacaoOutput.Resource();
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

		VkClearColorValue clearValue = {};
		clearValue.float32[0] = 1.0f;
		clearValue.float32[1] = 1.0f;
		clearValue.float32[2] = 1.0f;
		clearValue.float32[3] = 1.0f;

		vkCmdClearColorImage(cmdBuf1, m_cacaoOutput.Resource(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &barrier.subresourceRange);

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
	}

    // Render Scene to the MSAA HDR RT ------------------------------------------------
    //

    {
        SetPerfMarkerBegin(cmdBuf1, "Color pass");
        m_GPUTimer.GetTimeStamp(cmdBuf1, "before color RP");
        VkClearValue clear_values[2];
        clear_values[0].color.float32[0] = 0.0f;
        clear_values[0].color.float32[1] = 0.0f;
        clear_values[0].color.float32[2] = 0.0f;
        clear_values[0].color.float32[3] = 0.0f;
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;

        VkRenderPassBeginInfo rp_begin;
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = NULL;
        rp_begin.renderPass = m_render_pass_HDR_MSAA;
        rp_begin.framebuffer = m_pFrameBuffer_HDR_MSAA;
        rp_begin.renderArea.offset.x = 0;
        rp_begin.renderArea.offset.y = 0;
        rp_begin.renderArea.extent.width = m_Width;
        rp_begin.renderArea.extent.height = m_Height;
        rp_begin.clearValueCount = 2;
        rp_begin.pClearValues = clear_values;

        vkCmdBeginRenderPass(cmdBuf1, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetScissor(cmdBuf1, 0, 1, &m_rectScissor);
        vkCmdSetViewport(cmdBuf1, 0, 1, &m_viewport);
        m_GPUTimer.GetTimeStamp(cmdBuf1, "after color RP");
    }

    if (pPerFrame != NULL)
    {
        // Render skydome
        //
        if (pState->skyDomeType == 1)
        {
            XMMATRIX clipToView = XMMatrixInverse(NULL, pPerFrame->mCameraViewProj);
            m_skyDome.Draw(cmdBuf1, clipToView);

            m_GPUTimer.GetTimeStamp(cmdBuf1, "Skydome cube");
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
            m_skyDomeProc.Draw(cmdBuf1, skyDomeConstants);

            m_GPUTimer.GetTimeStamp(cmdBuf1, "Skydome Proc");
        }

        // Render scene to color buffer
        //
        if (m_gltfPBR && pPerFrame != NULL)
        {
            m_gltfPBR->Draw(cmdBuf1);
            m_GPUTimer.GetTimeStamp(cmdBuf1, "PBR Forward");
        }

        // draw object's bounding boxes
        //
        if (m_gltfBBox && pPerFrame != NULL)
        {
            if (pState->bDrawBoundingBoxes)
            {
                m_gltfBBox->Draw(cmdBuf1, pPerFrame->mCameraViewProj);

                m_GPUTimer.GetTimeStamp(cmdBuf1, "Bounding Box");
            }
        }

        // draw light's frustums
        //
        if (pState->bDrawLightFrustum && pPerFrame != NULL)
        {
            SetPerfMarkerBegin(cmdBuf1, "light frustrums");

            XMVECTOR vCenter = XMVectorSet(0.0f, 0.0f, 0.5f, 0.0f);
            XMVECTOR vRadius = XMVectorSet(1.0f, 1.0f, 0.5f, 0.0f);
            XMVECTOR vColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
            for (uint32_t i = 0; i < pPerFrame->lightCount; i++)
            {
                XMMATRIX spotlightMatrix = XMMatrixInverse(NULL, pPerFrame->lights[i].mLightViewProj);
                XMMATRIX worldMatrix = spotlightMatrix * pPerFrame->mCameraViewProj;
                m_wireframeBox.Draw(cmdBuf1, &m_wireframe, worldMatrix, vCenter, vRadius, vColor);
            }

            m_GPUTimer.GetTimeStamp(cmdBuf1, "Light's frustum");

            SetPerfMarkerEnd(cmdBuf1);
        }
    }

    {
        vkCmdEndRenderPass(cmdBuf1);
        SetPerfMarkerEnd(cmdBuf1);
    }

    // Resolve MSAA ------------------------------------------------------------------------
    // Ideally this resolve should be part of the previous rende pass, that would save a decompression
    //
    {
        SetPerfMarkerBegin(cmdBuf1, "Resolving MSAA");
        {
            VkImageMemoryBarrier barrier[2] = {};
            barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier[0].pNext = NULL;
            barrier[0].srcAccessMask = 0;
            barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier[0].subresourceRange.baseMipLevel = 0;
            barrier[0].subresourceRange.levelCount = 1;
            barrier[0].subresourceRange.baseArrayLayer = 0;
            barrier[0].subresourceRange.layerCount = 1;
            barrier[0].image = m_HDR.Resource();

            barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier[1].pNext = NULL;
            barrier[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier[1].subresourceRange.baseMipLevel = 0;
            barrier[1].subresourceRange.levelCount = 1;
            barrier[1].subresourceRange.baseArrayLayer = 0;
            barrier[1].subresourceRange.layerCount = 1;
            barrier[1].image = m_HDRMSAA.Resource();

            vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 2, barrier);
        }

        {
            VkImageResolve re = {};
            re.srcOffset.x = 0;
            re.srcOffset.y = 0;
            re.extent.width = m_Width;
            re.extent.height = m_Height;
            re.extent.depth = 1;
            re.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            re.srcSubresource.layerCount = 1;
            re.dstOffset.x = 0;
            re.dstOffset.y = 0;
            re.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            re.dstSubresource.layerCount = 1;
            vkCmdResolveImage(cmdBuf1, m_HDRMSAA.Resource(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_HDR.Resource(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &re);
        }

        {
            VkImageMemoryBarrier barrier[2] = {};
            barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier[0].pNext = NULL;
            barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier[0].subresourceRange.baseMipLevel = 0;
            barrier[0].subresourceRange.levelCount = 1;
            barrier[0].subresourceRange.baseArrayLayer = 0;
            barrier[0].subresourceRange.layerCount = 1;
            barrier[0].image = m_HDR.Resource();

            barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier[1].pNext = NULL;
            barrier[1].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier[1].subresourceRange.baseMipLevel = 0;
            barrier[1].subresourceRange.levelCount = 1;
            barrier[1].subresourceRange.baseArrayLayer = 0;
            barrier[1].subresourceRange.layerCount = 1;
            barrier[1].image = m_HDRMSAA.Resource();

            vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 2, barrier);
        }

        m_GPUTimer.GetTimeStamp(cmdBuf1, "Resolve MSAA");
        SetPerfMarkerEnd(cmdBuf1);
    }

    // Post proc---------------------------------------------------------------------------
    //

    // Bloom, takes HDR as input and applies bloom to it.
    //
	if (0)
    {
        SetPerfMarkerBegin(cmdBuf1, "post proc");

        // Downsample pass
        m_downSample.Draw(cmdBuf1);
        // m_downSample.Gui();
        m_GPUTimer.GetTimeStamp(cmdBuf1, "Downsample");

        // Bloom pass (needs the downsampled data)
        m_bloom.Draw(cmdBuf1);
        // m_bloom.Gui();
        m_GPUTimer.GetTimeStamp(cmdBuf1, "Bloom");

        SetPerfMarkerEnd(cmdBuf1);
    }

    // If using FreeSyncHDR we need to to the tonemapping in-place and then apply the GUI, later we'll apply the color conversion into the swapchain
    //
    if (pSwapChain->GetDisplayMode() != DISPLAYMODE_SDR && !pState->m_dispalyCacaoDirectly)
    {
        // In place Tonemapping ------------------------------------------------------------------------
        //
        {
            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.pNext = NULL;
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.image = m_HDR.Resource();
                vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
            }

            m_toneMappingCS.Draw(cmdBuf1, m_HDRUAV, pState->exposure, pState->toneMapper, m_Width, m_Height);

            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.pNext = NULL;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.image = m_HDR.Resource();
                vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
            }
        }

        // Render HUD  ------------------------------------------------------------------------
        //
        {
            // prepare render pass
            {
                VkRenderPassBeginInfo rp_begin = {};
                rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rp_begin.pNext = NULL;
                rp_begin.renderPass = m_render_pass_PBR_HDR;
                rp_begin.framebuffer = m_pFrameBuffer_PBR_HDR;
                rp_begin.renderArea.offset.x = 0;
                rp_begin.renderArea.offset.y = 0;
                rp_begin.renderArea.extent.width = m_Width;
                rp_begin.renderArea.extent.height = m_Height;
                rp_begin.clearValueCount = 0;
                rp_begin.pClearValues = NULL;
                vkCmdBeginRenderPass(cmdBuf1, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
            }

            vkCmdSetScissor(cmdBuf1, 0, 1, &m_rectScissor);
            vkCmdSetViewport(cmdBuf1, 0, 1, &m_viewport);

            m_ImGUI.Draw(cmdBuf1);

            vkCmdEndRenderPass(cmdBuf1);

            m_GPUTimer.GetTimeStamp(cmdBuf1, "ImGUI Rendering");
        }
    }

    // submit command buffer
    {
        VkResult res = vkEndCommandBuffer(cmdBuf1);
        assert(res == VK_SUCCESS);

        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmdBuf1;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = NULL;
        res = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
        assert(res == VK_SUCCESS);
    }

    // Wait for swapchain (we are going to render to it) -----------------------------------
    //
    int imageIndex = pSwapChain->WaitForSwapChain();

    m_CommandListRing.OnBeginFrame();

    VkCommandBuffer cmdBuf2 = m_CommandListRing.GetNewCommandList();

    {
        VkCommandBufferBeginInfo cmd_buf_info;
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = NULL;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmd_buf_info.pInheritanceInfo = NULL;
        VkResult res = vkBeginCommandBuffer(cmdBuf2, &cmd_buf_info);
        assert(res == VK_SUCCESS);
    }

    SetPerfMarkerBegin(cmdBuf2, "rendering to swap chain");

    // prepare render pass
    {
        VkRenderPassBeginInfo rp_begin = {};
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = NULL;
        rp_begin.renderPass = pSwapChain->GetRenderPass();
        rp_begin.framebuffer = pSwapChain->GetFramebuffer(imageIndex);
        rp_begin.renderArea.offset.x = 0;
        rp_begin.renderArea.offset.y = 0;
        rp_begin.renderArea.extent.width = m_Width;
        rp_begin.renderArea.extent.height = m_Height;
        rp_begin.clearValueCount = 0;
        rp_begin.pClearValues = NULL;
        vkCmdBeginRenderPass(cmdBuf2, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdSetScissor(cmdBuf2, 0, 1, &m_rectScissor);
    vkCmdSetViewport(cmdBuf2, 0, 1, &m_viewport);

	if (!pState->m_dispalyCacaoDirectly)
	{
		if (pSwapChain->GetDisplayMode() != DISPLAYMODE_SDR)
		{
			if (!pState->m_dispalyCacaoDirectly)
			{
				m_colorConversionPS.Draw(cmdBuf2, m_HDRSRV);
				m_GPUTimer.GetTimeStamp(cmdBuf2, "Color conversion");
			}
		}
		else
		{
			// Tonemapping ------------------------------------------------------------------------
			//
			{
				{
					m_toneMappingPS.Draw(cmdBuf2, m_HDRSRV, pState->exposure, pState->toneMapper);
					m_GPUTimer.GetTimeStamp(cmdBuf2, "Tone mapping");
				}
			}

			// Render HUD  ------------------------------------------------------------------------
			//
			{
				m_ImGUI.Draw(cmdBuf2);
				m_GPUTimer.GetTimeStamp(cmdBuf2, "ImGUI Rendering");
			}
		}
	}
	else
	{
		SetPerfMarkerBegin(cmdBuf2, "Output CACAO Directly");

		VkDescriptorBufferInfo cbDummyConstantBuffer;
		uint32_t *dummy;
		m_ConstantBufferRing.AllocConstantBuffer(sizeof(*dummy), (void **)&dummy, &cbDummyConstantBuffer);
		*dummy = 0;

		VkDescriptorSet descriptorSet = m_directOutputDescriptorSets[m_curBackBuffer];

		// modify Descriptor set
		SetDescriptorSet(m_pDevice->GetDevice(), 1, m_cacaoOutputSRV, &m_directOutputSampler, descriptorSet);
		m_ConstantBufferRing.SetDescriptorSet(0, sizeof(*dummy), descriptorSet);

		// Draw!
		m_directOutputPS.Draw(cmdBuf2, cbDummyConstantBuffer, descriptorSet);

		SetPerfMarkerEnd(cmdBuf2);

		m_ImGUI.Draw(cmdBuf2);
		m_GPUTimer.GetTimeStamp(cmdBuf2, "ImGUI Rendering");
	}

    SetPerfMarkerEnd(cmdBuf2);

    m_GPUTimer.OnEndFrame();

    vkCmdEndRenderPass(cmdBuf2);

    // Close & Submit the command list ----------------------------------------------------
    //
    {
        VkResult res = vkEndCommandBuffer(cmdBuf2);
        assert(res == VK_SUCCESS);

        VkSemaphore ImageAvailableSemaphore;
        VkSemaphore RenderFinishedSemaphores;
        VkFence CmdBufExecutedFences;
        pSwapChain->GetSemaphores(&ImageAvailableSemaphore, &RenderFinishedSemaphores, &CmdBufExecutedFences);

        VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info2;
        submit_info2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info2.pNext = NULL;
        submit_info2.waitSemaphoreCount = 1;
        submit_info2.pWaitSemaphores = &ImageAvailableSemaphore;
        submit_info2.pWaitDstStageMask = &submitWaitStage;
        submit_info2.commandBufferCount = 1;
        submit_info2.pCommandBuffers = &cmdBuf2;
        submit_info2.signalSemaphoreCount = 1;
        submit_info2.pSignalSemaphores = &RenderFinishedSemaphores;

        res = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submit_info2, CmdBufExecutedFences);
        assert(res == VK_SUCCESS);
    }
}

#ifdef FFX_CACAO_ENABLE_PROFILING
void SampleRenderer::GetCacaoTimingValues(State* pState, FfxCacaoDetailedTiming* timings)
{
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	if (pState->m_useDownsampledSsao)
	{
		ffxCacaoVkGetDetailedTimings(m_cacaoContextDownsampled, timings);
	}
	else
	{
		ffxCacaoVkGetDetailedTimings(m_cacaoContextNative, timings);
	}
#else
	ffxCacaoVkGetDetailedTimings(m_cacaoContextDownsampled, timings);
#endif
}
#endif
