# FidelityFX CACAO

The **FidelityFX CACAO** library implements screen space ambient occlusion for use in real time applications. A full sample can be found on the [FidelityFX CACAO Github page](https://github.com/GPUOpen-Effects/FidelityFX-CACAO).

# Context Initialisation and Shutdown (D3D12)

First the FFX CACAO header must be included. This can be found at `ffx-cacao/inc/ffx_cacao.h`. This header contains the definitions for D3D12 and in future will contain the definitions for Vulkan. To restrict FFX CACAO to either API the macros `FFX_CACAO_ENABLE_D3D12` or `FFX_CACAO_ENABLE_VK` must be set to 1 or 0 to enable or disable the API. It is intended that these macros are either edited in the `ffx_cacao.h` header or passed as compile flags when integrating FFX CACAO into a project.

Then a context must be created. This is usually done only once per device. To create a context you must first query for the size of a context, allocate space for a context, then inintialise the context.

```C++
// initialisation
size_t ffxCacaoContextSize = ffxCacaoD3D12GetContextSize();
FfxCacaoD3D12Context *context = (FfxCacaoD3D12Context*)malloc(ffxCacaoContextSize);
FfxCacaoStatus status = ffxCacaoD3D12InitContext(context, d3d12Device);
assert(status == FFX_CACAO_STATUS_OK);
...
// finalisation
status = ffxCacaoD3D12DestroyContext(context);
assert(status == FFX_CACAO_STATUS_OK);
free(context);
```

Once the context is initialised, it will need to have screen size dependent resources initialised each time the screen size is changed. To do this, an `FfxCacaoD3D12ScreenSizeInfo` struct must be filled out. The FFX CACAO effect is computed using a depth buffer and optional normal buffer. FFX CACAO writes its output to a user provided output buffer. The depth buffer, normal buffer and output buffer provided to FFX CACAO must all be the same size.

In addition, at this point it must be specified whether FFX CACAO should generate SSAO on a downsampled texture or a native resolution texture.

```C++
// initialisation
FfxCacaoD3D12ScreenSizeInfo screenSizeInfo = {};
screenSizeInfo.width = /* width of the input/output buffers */;
screenSizeInfo.height = /* height of the input/output buffers */;
screenSizeInfo.depthBufferResource = /* ID3D12Resource* for the depth input buffer */;
screenSizeInfo.depthBufferSrvDesc = /* D3D12_SHADER_RESOURCE_VIEW_DESC for the depth input buffer */;
screenSizeInfo.normalBufferResource = /* ID3D12Resource* for the normal input buffer - or NULL if none shall be provided */;
screenSizeInfo.normalBufferSrvDesc = /* D3D12_SHADER_RESOURCE_VIEW_DESC for the normal input buffer */;
screenSizeInfo.outputResource = /* ID3D12Resource* for the output buffer */;
screenSizeInfo.depthBufferSrvDesc = /* D3D12_SHADER_RESOURCE_VIEW_DESC for the depth output */;

FfxCacaoBool useDownsampledSsaoGeneration = /* whether or not SSAO should be generated on a downsampled texture */;

status = ffxCacaoD3D12InitScreenSizeDependentResources(context, &screenSizeInfo, useDownsampledSsaoGeneration);
assert(status == FFX_CACAO_STATUS_OK);
...
// finalisation
status = ffxCacaoD3D12DestroyScreenSizeDependentResources(context);
assert(status == FFX_CACAO_STATUS_OK);
```

The settings for the FFX CACAO effect may be changed via the `FfxCacaoSettings` struct and the `ffxCacaoD3D12UpdateSettings` function as follows.

```C++
FfxCacaoSettings settings = {};
settings.radius = /* world view radius of the occlusion sphere */;
settings.shadowMultiplier = /* effect strength linear multiplier */;
settings.shadowPower = /* effect strength power multiplier */;
settings.shadowClamp = /* effect max limit */;
settings.horizonAngleThreshold = /* minimum horizon angle for contributions to occlusion to limit self shadowing */
settings.fadeOutFrom = /* effect fade out from world space distance */;
settings.fadeOutTo = /* effect fade out to world space distance */;
settings.qualityLevel = /* the quality of the effect, ranging from lowest to highest (adaptive). This affects the number of samples taken to generate SSAO. */;
settings.adaptiveQualityLimit = /* quality limit for adaptive quality */;
settings.blurPassCount = /* a number of edge sensitive blurs from 1 to 8 to perform after SSAO generation */;
settings.sharpness = /* how much to bleed over edges - 0 = ignore edges, 1 = don't bleed over edges */;
settings.temporalSupersamplingAngleOffset = /* sampling angle offset for temporal super sampling */;
settings.temporalSupersamplingRaidusOffset = /* sampling effect radius offset for temporal super sampling */;
settings.detailShadowStrength = /* used to generate details in high res AO */;
settings.generateNormals = /* should the effect generate normals from the depth buffer or use a provided normal buffer */;
settings.bilateralSigmaSquared = /* a parameter for use in bilateral upsampling. Higher values create more blur to help reduce noise */;
settings.bilateralSimilarityDistanceSigma = /* a parameter for use in bilateral upsampling. Lower values create reduce bluring across edge boundaries */;
```

These settings can be set to sensible defaults from the constant `FFX_CACAO_DEFAULT_SETTINGS` and updated using the function `ffxCacaoD3D12UpdateSettings` as follows.

```C++
FfxCacaoSettings settings = FFX_CACAO_DEFAULT_SETTINGS;
status = ffxCacaoD3D12UpdateSettings(context, &settings);
assert(status == FFX_CACAO_STATUS_OK);
```

Note that the `FFX_CACAO_DEFAULT_SETTINGS` provides a sensible quick start for high quality settings. The parameters `radius`, `fadeOutFrom` and `fadeOutTo` should
be changed to match the world space of the target scene, and the parameter `blurPassCount` is recommended to be increased for lower quality settings. A more complete
set of sensible defaults may be found in the FFX CACAO sample in the file `sample/src/DX12/FFX_CACAO_Sample.cpp`, where multiple parameters have been varied to move
from high to low quality presets.

Finally, a FFX CACAO can be called to add commands to a `ID3D12GraphicsCommandList` using the `ffxCacaoD3D12Draw` function as follows.

```C++
FfxCacaoMatrix4x4 proj = /* row major projection matrix */;
FfxCacaoMatrix4x4 normalsToView = /* row major matrix to convert normals to viewspace */
status = ffxCacaoD3D12Draw(context, commandList, &proj, &normalsToView);
assert(status == FFX_CACAO_STATUS_OKAY);
```

The matrix `proj` is the projection matrix used from viewspace to normalised device coordinates. The matrix `normalsToView` is a matrix to convert the normals provided in the normal buffer to viewspace.
