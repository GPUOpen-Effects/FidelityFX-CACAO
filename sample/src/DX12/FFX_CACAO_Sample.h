// AMD SampleDX12 sample code
// 
// Copyright(c) 2017 Advanced Micro Devices, Inc.All rights reserved.
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

#include "SampleRenderer.h"

#include "ffx_cacao.h"

#define FFX_CACAO_PROFILE_SAMPLE (0 && FFX_CACAO_ENABLE_PROFILING)

// =======================================================================================
// stuff ripped from intel ASSAO
// =======================================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016, Intel Corporation
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of 
// the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
// SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File changes (yyyy-mm-dd)
// 2016-09-07: filip.strugar@intel.com: first commit
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class vaVector3
{
public:
	float x, y, z;

public:
	vaVector3() { };
	vaVector3(const float * p) { assert(p != NULL); x = p[0]; y = p[1]; z = p[2]; }
	vaVector3(float x, float y, float z) : x(x), y(y), z(z) { }

	const float & operator [] (int i) const { assert(i >= 0 && i < 3); return (&x)[i]; }
	float & operator [] (int i) { assert(i >= 0 && i < 3); return (&x)[i]; }

	// assignment operators
	vaVector3& operator += (const vaVector3 &);
	vaVector3& operator -= (const vaVector3 &);
	vaVector3& operator *= (float);
	vaVector3& operator /= (float);

	// unary operators
	vaVector3 operator + () const;
	vaVector3 operator - () const;

	// binary operators
	vaVector3 operator + (const vaVector3 &) const;
	vaVector3 operator - (const vaVector3 &) const;
	vaVector3 operator * (const vaVector3 &) const;
	vaVector3 operator / (const vaVector3 &) const;

	vaVector3 operator * (float) const;
	vaVector3 operator / (float) const;
	vaVector3 operator + (float) const;
	vaVector3 operator - (float) const;

	friend vaVector3 operator * (float, const class vaVector3 &);

	bool operator == (const vaVector3 &) const;
	bool operator != (const vaVector3 &) const;

	float                   Length() const;
	float                   LengthSq() const;
	vaVector3               Normalize() const;      // this should be called NormalizeD or something similar to indicate it returning value
	vaVector3               ComponentAbs() const;
	bool                    IsNormal(float epsilon = 1e-6f);

public:
	static float            Dot(const vaVector3 & a, const vaVector3 & b);
	static vaVector3        Cross(const vaVector3 & a, const vaVector3 & b);
	static bool             CloseEnough(const vaVector3 & a, const vaVector3 & b, float epsilon = 1e-6f);

	static vaVector3        ComponentMul(const vaVector3 & a, const vaVector3 & b);
	static vaVector3        ComponentDiv(const vaVector3 & a, const vaVector3 & b);
	static vaVector3        ComponentMin(const vaVector3 & a, const vaVector3 & b);
	static vaVector3        ComponentMax(const vaVector3 & a, const vaVector3 & b);

	static vaVector3        BaryCentric(const vaVector3 & v1, const vaVector3 & v2, const vaVector3 & v3, float f, float g);

	// Hermite interpolation between position v1, tangent t1 (when s == 0) and position v2, tangent t2 (when s == 1).
	static vaVector3        Hermite(const vaVector3 & v1, const vaVector3 & t1, const vaVector3 & v2, const vaVector3 &t2, float s);

	// CatmullRom interpolation between v1 (when s == 0) and v2 (when s == 1)
	static vaVector3        CatmullRom(const vaVector3 &v0, const vaVector3 &v1, const vaVector3 & v2, const vaVector3 & v3, float s);

	static float            AngleBetweenVectors(const vaVector3 & a, const vaVector3 & b);
};

class vaQuaternion
{
public:
	float x, y, z, w;

public:
	static vaQuaternion        Identity;

public:
	vaQuaternion() { }
	explicit vaQuaternion(const float * p) { assert(p != NULL); x = p[0]; y = p[1]; z = p[2]; w = p[3]; }
	vaQuaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }

	// assignment
	vaQuaternion & operator += (const vaQuaternion &);
	vaQuaternion & operator -= (const vaQuaternion &);
	vaQuaternion & operator *= (const vaQuaternion &);
	vaQuaternion & operator *= (float);
	vaQuaternion & operator /= (float);

	// unary
	vaQuaternion  operator + () const;
	vaQuaternion  operator - () const;

	// binary
	vaQuaternion operator + (const vaQuaternion &) const;
	vaQuaternion operator - (const vaQuaternion &) const;
	vaQuaternion operator * (const vaQuaternion &) const;
	vaQuaternion operator * (float) const;
	vaQuaternion operator / (float) const;

	friend vaQuaternion operator * (float, const vaQuaternion &);

	bool operator == (const vaQuaternion &) const;
	bool operator != (const vaQuaternion &) const;


	float                Length() const;
	float                LengthSq() const;
	vaQuaternion         Conjugate() const;
	void                 ToAxisAngle(vaVector3 & outAxis, float & outAngle) const;
	vaQuaternion         Normalize() const;      // this should be called NormalizeD or something similar to indicate it returning value
	vaQuaternion         Inverse() const;

	// Expects unit quaternions.
	vaQuaternion         Ln() const;

	// Expects pure quaternions. (w == 0)  w is ignored in calculation.
	vaQuaternion         Exp() const;

	// VA convention: X is forward
	vaVector3             GetAxisX() const;

	// VA convention: Y is right
	vaVector3             GetAxisY() const;

	// VA convention: Z is up
	vaVector3             GetAxisZ() const;

public:
	static float         Dot(const vaQuaternion & a, const vaQuaternion & b);

	// Quaternion multiplication. The result represents the rotation b followed by the rotation a.
	static vaQuaternion  Multiply(const vaQuaternion & a, const vaQuaternion & b);

	// Build quaternion from axis and angle.
	static vaQuaternion  RotationAxis(const vaVector3 & v, float angle);

	// Yaw around the +Z (up) axis, a pitch around the +Y (right) axis, and a roll around the +X (forward) axis.
	static vaQuaternion  RotationYawPitchRoll(float yaw, float pitch, float roll);

	// Spherical linear interpolation between Q1 (t == 0) and Q2 (t == 1).
	// Expects unit quaternions.
	static vaQuaternion  Slerp(const vaQuaternion & q1, const vaQuaternion & q2, float t);

	// Spherical quadrangle interpolation.
	static vaQuaternion  Squad(const vaQuaternion & q1, const vaQuaternion & q2, const vaQuaternion & q3, const vaQuaternion & q4, float t);

	// Barycentric interpolation.
	// Slerp(Slerp(Q1, Q2, f+g), Slerp(Q1, Q3, f+g), g/(f+g))
	static vaQuaternion  BaryCentric(const vaQuaternion & q1, const vaQuaternion & q2, const vaQuaternion & q3, float f, float g);
};

class vaCameraControllerFocusLocationsFlythrough
{

public:
	struct Keyframe
	{
		vaVector3               Position;
		vaQuaternion            Orientation;
		float                   ShowTime;
		float                   UserParam0;
		float                   UserParam1;

		Keyframe(const vaVector3 & position, const vaQuaternion & orientation, float showTime, float userParam0 = 0.0f, float userParam1 = 0.0f) : Position(position), Orientation(orientation), ShowTime(showTime), UserParam0(userParam0), UserParam1(userParam1) { }
	};

private:
	std::vector<Keyframe>       m_keys;
	int                         m_currentKeyIndex;
	float                       m_currentKeyTimeRemaining;

	float                       m_userParam0;
	float                       m_userParam1;

	bool                        m_fixedUp;
	vaVector3                   m_fixedUpVec;

public:
	void                                                AddKey(const Keyframe & key) { m_keys.push_back(key); }
	void                                                ResetTime() { m_currentKeyIndex = -1; m_currentKeyTimeRemaining = 0.0f; }

	void                                                SetFixedUp(bool enabled, vaVector3 & upVec = vaVector3(0.0f, 0.0f, 1.0f)) { m_fixedUp = enabled; m_fixedUpVec = upVec; }

	void                                                CameraTick(float deltaTime, Camera *camera, bool hasFocus);
};

// =======================================================================================
// end of stuff ripped from intel ASSAO
// =======================================================================================

struct FfxCacaoPreset
{
	bool useDownsampledSsao;
	FfxCacaoSettings settings;
};

class FfxCacaoSample : public FrameworkWindows
{
public:
    FfxCacaoSample(LPCSTR name);
    void OnCreate(HWND hWnd);
    void OnDestroy();
    void OnRender();
    bool OnEvent(MSG msg);
	void OnResize(uint32_t width, uint32_t height) { OnResize(width, height, false); }
    void OnResize(uint32_t Width, uint32_t Height, bool force);
    void SetFullScreen(bool fullscreen);
    
private:
#if FFX_CACAO_PROFILE_SAMPLE
	void WriteCaptureFile(uint64_t gpuTicksPerMicrosecond);

	static const uint32_t  NUM_CAPTURE_SAMPLES = 5000;
	FfxCacaoDetailedTiming m_captureTimings[NUM_CAPTURE_SAMPLES];
	uint32_t               m_curTiming;
	char                   m_captureFilename[1024];
#endif

	HWND                  m_hWnd;

    Device                m_device;
    SwapChain             m_swapChain;

    GLTFCommon           *m_pGltfLoader = NULL;

    SampleRenderer       *m_Node = NULL;
    SampleRenderer::State m_state;

	int                   m_loadingStage;
	bool                  m_requiresLoad = true;
	int                   m_preset;

	bool                  m_cacaoOutputDirectly = true;

	int                   m_presetIndex = 0;
	FfxCacaoPreset        m_currentSettings;

    float                 m_distance;
    float                 m_roll;
    float                 m_pitch;

    float                 m_time;             // WallClock in seconds.
    double                m_deltaTime;        // The elapsed time in milliseconds since the previous frame.
    double                m_lastFrameTime;

	bool                  m_isCapturing = false;
	bool                  m_vsyncEnabled = true;
	int                   m_cameraControlSelected = 0;
    bool                  m_bPlay;
	bool                  m_displayGUI;
	bool                  m_fullscreen;
	int                   m_selectedResolution;
	char                  m_filenameBuffer[1024];
	vaCameraControllerFocusLocationsFlythrough m_flythroughCameraController;
};
