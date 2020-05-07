# FidelityFX CACAO

Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## Combined Adaptive Compute Ambient Occlusion (CACAO)

Combined Adaptive Compute Ambient Occlusion (CACAO) is a highly optimized adaptive sampling ambient occlusion implementation. This directory contains the source code for CACAO as well as a sample demonstrating usage and integration of the library. The directory structure is as follows:

- ffx-cacao contains the [CACAO library](https://github.com/GPUOpen-Effects/FidelityFX-CACAO/tree/master/ffx-cacao)
- sample contains the [CACAO sample](https://github.com/GPUOpen-Effects/FidelityFX-CACAO/tree/master/sample)

You can find the binaries for FidelityFX CACAO in the release section on GitHub.

# Sponza Model Issue

In the provided Sponza model for the FFX CACAO sample, at the bottom of some curtains,
there is an issue with incorrect normals
in the mesh causing ambient occlusion to be incorrectly calculated as light in places where occlusion
should be dark. This is a known issue with the mesh, and not an issue with FFX CACAO itself.
