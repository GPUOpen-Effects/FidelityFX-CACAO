# FidelityFX CACAO Sample

A small demo to show integration and usage of the [FidelityFX CACAO library](https://github.com/GPUOpen-Effects/FidelityFX-CACAO/tree/master/ffx-cacao).

![Screenshot](screenshot.png)

# Sponza Model Issue

In the provided Sponza model, at the bottom of some curtains, there is an issue with incorrect normals
in the mesh causing ambient occlusion to be incorrectly calculated as light in places where occlusion
should be dark. This is a known issue with the mesh, and not an issue with FFX CACAO itself.

# Build Instructions

### Prerequisites

To build this sample, the following tools are required:

- [CMake 3.4](https://cmake.org/download/)
- [Visual Studio 2017](https://visualstudio.microsoft.com/downloads/)
- [Windows 10 SDK 10.0.17763.0](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)

Then follow these steps:

1) Clone the repository with its submodules:
    ```
    > git clone https://github.com/GPUOpen-Effects/FidelityFX-CACAO.git --recurse-submodules
    ```

2) Generate the solutions:
    ```
    > cd FidelityFX-CACAO\sample\build
    > GenerateSolutions.bat
    ```

3) Open the solution in the DX12 directory, compile and run.
