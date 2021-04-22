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

#include "stdafx.h"

#include "Sample.h"
#include "Common.h"

Sample::Sample(LPCSTR name) : FrameworkWindows(name)
{
	m_lastFrameTime = MillisecondsNow();
	m_time = 0;
	m_bPlay = true;

	m_pGltfLoader = NULL;
	m_currentDisplayMode = DISPLAYMODE_SDR;
}

//--------------------------------------------------------------------------------------
//
// OnParseCommandLine
//
//--------------------------------------------------------------------------------------
void Sample::OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight, bool *pbFullScreen)
{
	// set some default values
	*pWidth = 1920;
	*pHeight = 1080;
	m_activeScene = 0; //load the first one by default
	*pbFullScreen = false;
	m_activeCamera = 0;
#ifdef FFX_CACAO_ENABLE_PROFILING
	m_isBenchmarking = false;
#endif

	//read globals
	auto process = [&](json jData)
	{
		*pWidth = jData.value("width", *pWidth);
		*pHeight = jData.value("height", *pHeight);
		*pbFullScreen = jData.value("fullScreen", *pbFullScreen);
		m_presetIndex = jData.value("preset", m_presetIndex);
		m_activeScene = jData.value("activeScene", m_activeScene);
		m_activeCamera = jData.value("activeCamera", m_activeCamera);
		m_isCpuValidationLayerEnabled = jData.value("CpuValidationLayerEnabled", m_isCpuValidationLayerEnabled);
		m_isGpuValidationLayerEnabled = jData.value("GpuValidationLayerEnabled", m_isGpuValidationLayerEnabled);
#ifdef FFX_CACAO_ENABLE_PROFILING
		m_isBenchmarking = jData.value("benchmark", m_isBenchmarking);
#endif
	};

	// read config file (and override values from commandline if so)
	//
	{
		std::ifstream f("SampleSettings.json");
		if (!f)
		{
			MessageBox(NULL, "Config file not found!\n", "Cauldron Panic!", MB_ICONERROR);
			exit(0);
		}

		try
		{
			f >> m_jsonConfigFile;
		}
		catch (json::parse_error)
		{
			MessageBox(NULL, "Error parsing FFX_CACAO_Sample.json!\n", "Cauldron Panic!", MB_ICONERROR);
			exit(0);
		}
	}


	json globals = m_jsonConfigFile["globals"];
	process(globals);

	// get the list of scenes
	for (const auto & scene : m_jsonConfigFile["scenes"])
		m_sceneNames.push_back(scene["name"]);

	//read json globals from commandline
	//
	try
	{
		if (strlen(lpCmdLine) > 0)
		{
			auto j3 = json::parse(lpCmdLine);
			process(j3);
		}
	}
	catch (json::parse_error)
	{
		Trace("Error parsing commandline\n");
		exit(0);
	}

#ifdef FFX_CACAO_ENABLE_PROFILING
	if (m_isBenchmarking)
	{
		bool downsampled = FFX_CACAO_PRESETS[m_presetIndex].useDownsampledSsao;
		uint32_t quality = FFX_CACAO_PRESETS[m_presetIndex].settings.qualityLevel;
		m_benchmarkScreenWidth = *pWidth;
		m_benchmarkScreenHeight = *pHeight;
		m_benchmarkWarmUpFramesToRun = 100;
		snprintf(m_benchmarkFilename, _countof(m_benchmarkFilename), "FFX_CACAO_Vulkan_Benchmark_%s_%ux%u_Q%u.csv", downsampled ? "downsampled" : "native", *pWidth, *pHeight, quality);
		m_vsyncEnabled = false;
		m_isGpuValidationLayerEnabled = false;
		m_isCpuValidationLayerEnabled = false;
	}
#endif
}


//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void Sample::OnCreate(HWND hWnd)
{
	// Create Device
	//
	m_device.OnCreate("FfxCacaoSample", "Cauldron", m_isCpuValidationLayerEnabled, m_isGpuValidationLayerEnabled, hWnd);
	m_device.CreatePipelineCache();

	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_device.GetPhysicalDevice(), &physicalDeviceProperties);
	m_microsecondsPerGpuTick = 1e-3f * physicalDeviceProperties.limits.timestampPeriod;

	//init the shader compiler
	InitDirectXCompiler();
	CreateShaderCache();

	// Create Swapchain
	//

	uint32_t dwNumberOfBackBuffers = 2;
	m_swapChain.OnCreate(&m_device, dwNumberOfBackBuffers, hWnd);

	// Create a instance of the renderer and initialize it, we need to do that for each GPU
	//
	m_node = new SampleRenderer();
	m_node->OnCreate(&m_device, &m_swapChain);

	// init GUI (non gfx stuff)
	//
	ImGUI_Init((void *)hWnd);

	// Init Camera, looking at the origin
	//
	m_roll = 0.0f;
	m_pitch = 0.0f;
	m_distance = 3.5f;

	// init GUI state
	m_state.toneMapper = 0;
	m_state.useTAA = false; // no TAA in VK
	m_state.skyDomeType = 0;
	m_state.exposure = 1.0f;
	m_state.iblFactor = 2.0f;
	m_state.emmisiveFactor = 1.0f;
	m_state.drawLightFrustum = false;
	m_state.drawBoundingBoxes = false;
	m_state.camera.LookAt(m_roll, m_pitch, m_distance, XMVectorSet(0, 0, 0, 0));

	m_state.useCacao = true;
	m_state.dispalyCacaoDirectly = true;

	m_state.cacaoSettings = FFX_CACAO_PRESETS[m_presetIndex].settings;
	m_state.useDownsampledSsao = FFX_CACAO_PRESETS[m_presetIndex].useDownsampledSsao;
}

//--------------------------------------------------------------------------------------
//
// OnDestroy
//
//--------------------------------------------------------------------------------------
void Sample::OnDestroy()
{
#ifdef FFX_CACAO_ENABLE_PROFILING
	m_isBenchmarking = false;
#endif

	ImGUI_Shutdown();

	m_device.GPUFlush();

	// Fullscreen state should always be false before exiting the app.
	m_swapChain.SetFullScreen(false);

	m_node->UnloadScene();
	m_node->OnDestroyWindowSizeDependentResources();
	m_node->OnDestroy();

	delete m_node;

	m_swapChain.OnDestroyWindowSizeDependentResources();
	m_swapChain.OnDestroy();

	//shut down the shader compiler
	DestroyShaderCache(&m_device);

	if (m_pGltfLoader)
	{
		delete m_pGltfLoader;
		m_pGltfLoader = NULL;
	}

	m_device.DestroyPipelineCache();
	m_device.OnDestroy();
}

//--------------------------------------------------------------------------------------
//
// OnEvent, win32 sends us events and we forward them to ImGUI
//
//--------------------------------------------------------------------------------------
bool Sample::OnEvent(MSG msg)
{
	if (ImGUI_WndProcHandler(msg.hwnd, msg.message, msg.wParam, msg.lParam))
		return true;
	return true;
}

//--------------------------------------------------------------------------------------
//
// SetFullScreen
//
//--------------------------------------------------------------------------------------
void Sample::SetFullScreen(bool fullscreen)
{
	m_device.GPUFlush();

	if (!fullscreen)
	{
		m_currentDisplayMode = DISPLAYMODE_SDR;
	}

	m_swapChain.SetFullScreen(fullscreen);
}

//--------------------------------------------------------------------------------------
//
// OnResize
//
//--------------------------------------------------------------------------------------
void Sample::OnResize(uint32_t width, uint32_t height, DisplayModes displayMode, bool force)
{
#ifdef FFX_CACAO_ENABLE_PROFILING
	if (m_isBenchmarking && !m_benchmarkWarmUpFramesToRun)
	{
		if (width != m_benchmarkScreenWidth || height != m_benchmarkScreenHeight)
		{
			MessageBox(NULL, "Attempt to change screen resolution when benchmarking!\n", "Benchmark Failed!", MB_ICONERROR);
			exit(0);
		}
	}
#endif

	if (m_Width != width || m_Height != height || m_currentDisplayMode != displayMode || force)
	{
		// Flush GPU
		//
		m_device.GPUFlush();

		// If resizing but no minimizing
		//
		if (m_Width > 0 && m_Height > 0)
		{
			if (m_node != NULL)
			{
				m_node->OnDestroyWindowSizeDependentResources();
			}
			m_swapChain.OnDestroyWindowSizeDependentResources();
		}

		m_Width = width;
		m_Height = height;
		m_currentDisplayMode = displayMode;

		// if resizing but not minimizing the recreate it with the new size
		//
		if (m_Width > 0 && m_Height > 0)
		{
			m_swapChain.OnCreateWindowSizeDependentResources(m_Width, m_Height, m_vsyncEnabled, m_currentDisplayMode);
			if (m_node != NULL)
			{
				m_node->OnCreateWindowSizeDependentResources(&m_swapChain, m_Width, m_Height);
			}
		}
	}
	m_state.camera.SetFov(XM_PI / 4, m_Width, m_Height, 0.1f, 1000.0f);
}

//--------------------------------------------------------------------------------------
//
// BuildUI, also loads the scene!
//
//--------------------------------------------------------------------------------------
void Sample::BuildUI()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(250, 700), ImGuiCond_FirstUseEver);

	bool opened = true;
	ImGui::Begin("CACAO Sample", &opened);

	if (ImGui::CollapsingHeader("Sample Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Resolution       : %ix%i", m_Width, m_Height);
		const char *cameraControls = "Orbit\0WASD\0";

		ImGui::Combo("Camera", &m_cameraControlSelected, cameraControls);

		if (ImGui::Checkbox("VSync Enabled", &m_vsyncEnabled))
		{
			OnResize(m_Width, m_Height, DISPLAYMODE_SDR, true);
		}
	}

	if (m_requiresLoad)
	{
		m_requiresLoad = false;
		json scene = m_jsonConfigFile["scenes"][m_activeScene];

		// release everything and load the GLTF, just the light json data, the rest (textures and geometry) will be done in the main loop
		if (m_pGltfLoader != NULL)
		{
			m_node->UnloadScene();
			m_node->OnDestroyWindowSizeDependentResources();
			m_node->OnDestroy();
			m_pGltfLoader->Unload();
			m_node->OnCreate(&m_device, &m_swapChain);
			m_node->OnCreateWindowSizeDependentResources(&m_swapChain, m_Width, m_Height);
		}

		delete(m_pGltfLoader);
		m_pGltfLoader = new GLTFCommon();
		if (m_pGltfLoader->Load(scene["directory"], scene["filename"]) == false)
		{
			MessageBox(NULL, "The selected model couldn't be found, please check the documentation", "Cauldron Panic!", MB_ICONERROR);
			exit(0);
		}

		// Load the UI settings, and also some defaults cameras and lights, in case the GLTF has none
		{
			#define LOAD(j, key, val) val = j.value(key, val)

			// global settings
			LOAD(scene, "TAA", m_state.useTAA);
			LOAD(scene, "toneMapper", m_state.toneMapper);
			LOAD(scene, "skyDomeType", m_state.skyDomeType);
			LOAD(scene, "exposure", m_state.exposure);
			LOAD(scene, "iblFactor", m_state.iblFactor);
			LOAD(scene, "emmisiveFactor", m_state.emmisiveFactor);
			LOAD(scene, "skyDomeType", m_state.skyDomeType);

			// Add a default light in case there are none
			if (m_pGltfLoader->m_lights.size() == 0)
			{
				tfNode n;
				n.m_tranform.LookAt(PolarToVector(XM_PI / 2.0f, 0.58f)*3.5f, XMVectorSet(0, 0, 0, 0));

				tfLight l;
				l.m_type = tfLight::LIGHT_SPOTLIGHT;
				l.m_intensity = scene.value("intensity", 1.0f);
				l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
				l.m_range = 15;
				l.m_outerConeAngle = XM_PI / 4.0f;
				l.m_innerConeAngle = (XM_PI / 4.0f) * 0.9f;

				m_pGltfLoader->AddLight(n, l);
			}

			// default camera (in case the gltf has none)
			json camera = scene["camera"];
			LOAD(camera, "yaw", m_roll);
			LOAD(camera, "pitch", m_pitch);
			LOAD(camera, "distance", m_distance);
			XMVECTOR lookAt = GetVector(GetElementJsonArray(camera, "lookAt", { 0.0, 0.0, 0.0 }));
			m_state.camera.LookAt(m_roll, m_pitch, m_distance, lookAt);
			m_activeCamera = scene.value("activeCamera", m_activeCamera);

#ifdef FFX_CACAO_ENABLE_PROFILING
			if (m_isBenchmarking)
			{
				std::string deviceName;
				std::string driverVersion;
				json benchmarkJson = scene["BenchmarkSettings"];
				benchmarkJson["resultsFilename"] = m_benchmarkFilename;
				m_device.GetDeviceInfo(&deviceName, &driverVersion);
				BenchmarkConfig(benchmarkJson, m_activeCamera, m_pGltfLoader, deviceName, driverVersion);
			}
#endif

			// indicate the mainloop we started loading a GLTF and it needs to load the rest (textures and geometry)
			m_loadingScene = true;

			//bail out as we need to reload everything
			ImGui::End();
			ImGui::EndFrame();
			ImGui::NewFrame();
			return;
		}
	}

	if (ImGui::CollapsingHeader("FFX CACAO Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Combo("Preset", &m_presetIndex, FFX_CACAO_PRESET_NAMES, _countof(FFX_CACAO_PRESET_NAMES)) && m_presetIndex < _countof(FFX_CACAO_PRESETS))
		{
			Preset preset = FFX_CACAO_PRESETS[m_presetIndex];
			m_state.cacaoSettings = preset.settings;
			m_state.useDownsampledSsao = preset.useDownsampledSsao;
		}

		FFX_CACAO_Settings *settings = &m_state.cacaoSettings;
		ImGui::SliderFloat("Radius", &settings->radius, 0.0f, 10.0f);
		ImGui::SliderFloat("Shadow Multiplier", &settings->shadowMultiplier, 0.0f, 5.0f);
		ImGui::SliderFloat("Shadow Power", &settings->shadowPower, 0.5f, 5.0f);
		ImGui::SliderFloat("Shadow Clamp", &settings->shadowClamp, 0.0f, 1.0f);
		ImGui::SliderFloat("Horizon Angle Threshold", &settings->horizonAngleThreshold, 0.0f, 0.2f);
		ImGui::SliderFloat("Fade Out From", &settings->fadeOutFrom, 1.0f, 20.0f);
		ImGui::SliderFloat("Fade Out To", &settings->fadeOutTo, 1.0f, 40.0f);
		int qualityIndex = settings->qualityLevel;
		char *qualityLevels = "Lowest\0Low\0Medium\0High\0Highest\0" ;
		ImGui::Combo("Quality Level", &qualityIndex, qualityLevels);
		settings->qualityLevel = (FFX_CACAO_Quality)qualityIndex;
		if (settings->qualityLevel == FFX_CACAO_QUALITY_HIGHEST)
		{
			ImGui::SliderFloat("Adaptive Quality Level", &settings->adaptiveQualityLimit, 0.5f, 1.0f);
		}
		ImGui::SliderInt("Blur Pass Count", (int*)&settings->blurPassCount, 0, 8);
		ImGui::SliderFloat("Sharpness", &settings->sharpness, 0.0f, 1.0f);
		ImGui::SliderFloat("Detail Shadow Strength", &settings->detailShadowStrength, 0.0f, 5.0f);
		bool generateNormals = settings->generateNormals;
		ImGui::Checkbox("Generate Normal Buffer From Depth Buffer", &generateNormals);
		settings->generateNormals = generateNormals ? FFX_CACAO_TRUE : FFX_CACAO_FALSE;
		ImGui::Checkbox("Use Downsampled SSAO", &m_state.useDownsampledSsao);
		if (m_state.useDownsampledSsao)
		{
			ImGui::SliderFloat("Bilateral Sigma Squared", &settings->bilateralSigmaSquared, 0.0f, 10.0f);
			ImGui::SliderFloat("Bilateral Similarity Distance Sigma", &settings->bilateralSimilarityDistanceSigma, 0.1f, 1.0f);
		}

		ImGui::Checkbox("Display FFX CACAO Output Directly", &m_state.dispalyCacaoDirectly);
		if (!m_state.dispalyCacaoDirectly)
		{
			ImGui::Checkbox("Use FFX CACAO", &m_state.useCacao);
		}
		m_state.useCacao |= m_state.dispalyCacaoDirectly;

		if (m_presetIndex < _countof(FFX_CACAO_PRESETS) && (memcmp(&FFX_CACAO_PRESETS[m_presetIndex].settings, &m_state.cacaoSettings, sizeof(m_state.cacaoSettings)) || (FFX_CACAO_PRESETS[m_presetIndex].useDownsampledSsao != m_state.useDownsampledSsao)))
		{
			m_presetIndex = _countof(FFX_CACAO_PRESETS);
		}
	}

#ifdef FFX_CACAO_ENABLE_PROFILING
	if (m_state.useCacao && !m_vsyncEnabled && !m_isCpuValidationLayerEnabled && !m_isGpuValidationLayerEnabled)
	{
		if (ImGui::CollapsingHeader("Profiler", ImGuiTreeNodeFlags_DefaultOpen))
		{
			FFX_CACAO_DetailedTiming timings = {};
			m_node->GetCacaoTimingValues(&m_state, &timings);
			for (uint32_t i = 0; i < timings.numTimestamps; ++i)
			{
				const char *name = timings.timestamps[i].label;
				uint64_t ticks = timings.timestamps[i].ticks;

				ImGui::Text("%-32s: %7.1f", name, m_microsecondsPerGpuTick * (float)ticks);
			}
		}
	}
	else
	{
		ImGui::CollapsingHeader("Profiler Disabled (enable CACAO and turn off vsync and validation)");
	}
#endif

	ImGui::End();

	// Sets Camera based on UI selection (WASD, Orbit or any of the GLTF cameras)
	//
	ImGuiIO& io = ImGui::GetIO();
	{
		//If the mouse was not used by the GUI then it's for the camera
		//
		if (io.WantCaptureMouse)
		{
			io.MouseDelta.x = 0;
			io.MouseDelta.y = 0;
			io.MouseWheel = 0;
		}
		else if ((io.KeyCtrl == false) && (io.MouseDown[0] == true))
		{
			m_roll -= io.MouseDelta.x / 100.f;
			m_pitch += io.MouseDelta.y / 100.f;
		}

		// Choose camera movement depending on setting
		//
		if (m_cameraControlSelected == 0)
		{
			//  Orbiting
			//
			m_distance -= (float)io.MouseWheel / 3.0f;
			m_distance = std::max<float>(m_distance, 0.1f);

			bool panning = (io.KeyCtrl == true) && (io.MouseDown[0] == true);

			m_state.camera.UpdateCameraPolar(m_roll, m_pitch, panning ? -io.MouseDelta.x / 100.0f : 0.0f, panning ? io.MouseDelta.y / 100.0f : 0.0f, m_distance);
		}
		else if (m_cameraControlSelected == 1)
		{
			//  WASD
			//
			m_state.camera.UpdateCameraWASD(m_roll, m_pitch, io.KeysDown, io.DeltaTime);
		}
		else if (m_cameraControlSelected > 1)
		{
			// Use a camera from the GLTF
			//
			m_pGltfLoader->GetCamera(m_cameraControlSelected - 2, &m_state.camera);
			m_roll = m_state.camera.GetYaw();
			m_pitch = m_state.camera.GetPitch();
		}
	}
}

//--------------------------------------------------------------------------------------
//
// OnRender, updates the state from the UI, animates, transforms and renders the scene
//
//--------------------------------------------------------------------------------------
void Sample::OnRender()
{
	// Get timings
	//
	double timeNow = MillisecondsNow();
	float deltaTime = (m_timeStep == 0.0f) ? (float)(timeNow - m_lastFrameTime) : m_timeStep;
	m_lastFrameTime = timeNow;

	// Set animation time
	//
	if (m_bPlay)
	{
		m_time += (float)deltaTime / 1000.0f;
	}

	ImGUI_UpdateIO();
	ImGui::NewFrame();

	if (m_loadingScene)
	{
		// the scene loads in chuncks, that way we can show a progress bar
		static int loadingStage = 0;
		loadingStage = m_node->LoadScene(m_pGltfLoader, loadingStage);
		if (loadingStage == 0)
		{
			m_time = 0;
			m_loadingScene = false;
		}
	}
#ifdef FFX_CACAO_ENABLE_PROFILING
	else if (m_pGltfLoader && m_isBenchmarking)
	{
		// benchmarking takes control of the time, and exits the app when the animation is done

		if (m_benchmarkWarmUpFramesToRun)
		{
			--m_benchmarkWarmUpFramesToRun;
		}
		else
		{
			if (m_benchmarkScreenWidth != m_Width || m_benchmarkScreenHeight != m_Height || m_vsyncEnabled)
			{
				MessageBox(NULL, "Screen resolution not correct or vsync enabled!\n", "Benchmark Failed!", MB_ICONERROR);
				exit(0);
			}

			FFX_CACAO_DetailedTiming timings = {};
			m_node->GetCacaoTimingValues(&m_state, &timings);

			if (timings.numTimestamps)
			{
				std::vector<TimeStamp> timestamps;
				timestamps.reserve(timings.numTimestamps);

				for (uint32_t i = 0; i < timings.numTimestamps; ++i)
				{
					TimeStamp timestamp;
					timestamp.m_label = timings.timestamps[i].label;
					timestamp.m_microseconds = m_microsecondsPerGpuTick * (float)timings.timestamps[i].ticks;
					timestamps.push_back(timestamp);
				}

				std::string *pFilename = NULL;
				m_time = BenchmarkLoop(timestamps, &m_state.camera, (const std::string**)&pFilename);
			}
		}
	}
#endif
	else
	{
		// Build the UI. Note that the rendering of the UI happens later.
		BuildUI();

		if (m_bPlay)
		{
			m_time += (float)deltaTime / 1000.0f;
		}
	}


	// Animate and transform the scene
	//
	if (m_pGltfLoader)
	{
		m_pGltfLoader->SetAnimationTime(0, m_time);
		m_pGltfLoader->TransformScene(0, XMMatrixIdentity());
	}

	m_state.time = m_time;

	// Do Render frame using AFR
	//
	m_node->OnRender(&m_state, &m_swapChain);

	m_swapChain.Present();
}


//--------------------------------------------------------------------------------------
//
// WinMain
//
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	LPCSTR Name = "FFX CACAO Vulkan Sample v1.2";

	// create new Vulkan sample
	return RunFramework(hInstance, lpCmdLine, nCmdShow, new Sample(Name));
}

