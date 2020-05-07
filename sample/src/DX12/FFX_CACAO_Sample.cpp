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

#include "FFX_CACAO_Sample.h"

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

class vaMath
{
public:

	static inline float     TimeIndependentLerpF(float deltaTime, float lerpRate);

	template<class T>
	static inline T         Min(T const & a, T const & b);

	template<class T>
	static inline T         Max(T const & a, T const & b);

	template<class T>
	static inline T         Clamp(T const & v, T const & min, T const & max);

	template<class T>
	static inline T         Saturate(T const & a);

	template<class T>
	static inline T         Lerp(T const & a, T const & b, const float f);

	// why not just use std::swap?
	//template<class T>
	//static inline void      Swap(T & a, T & b);

	static inline float     Abs(float a);
	static inline double    Abs(double a);

	static inline double    Frac(double a);
	static inline float     Frac(float a);
	static inline int       FloorLog2(unsigned int n);

	static inline float     AngleWrap(float angle);
	static inline float     AngleSmallestDiff(float a, float b);

	static inline float     DegreeToRadian(float degree) { return degree * 3.1415926535f / 180.0f; }
	static inline float     RadianToDegree(float radian) { return radian * 180.0f / 3.1415926535f; }

	static inline bool      IsPowOf2(int val);
	static inline int       PowOf2Ceil(int val);
	static inline int       Log2(int val);

	template<class T>
	static inline T         Sq(const T & a);

	static inline float     Sqrt(float a) { return ::sqrtf(a); }
	static inline double    Sqrt(double a) { return ::sqrt(a); }

	static inline float     Pow(float a, float p) { return powf(a, p); }
	static inline double    Pow(double a, double p) { return pow(a, p); }

	static inline float     Exp(float p) { return expf(p); }
	static inline double    Exp(double p) { return exp(p); }

	static inline float     Sin(float a) { return sinf(a); }
	static inline double    Sin(double a) { return sin(a); }

	static inline float     Cos(float a) { return cosf(a); }
	static inline double    Cos(double a) { return cos(a); }

	static inline float     ASin(float a) { return asinf(a); }
	static inline double    ASin(double a) { return asin(a); }

	static inline float     ACos(float a) { return acosf(a); }
	static inline double    ACos(double a) { return acos(a); }

	static inline float     ATan2(float y, float x) { return atan2f(y, x); }

	static inline float       Round(float x) { return ::roundf(x); }
	static inline double      Round(double x) { return ::round(x); }

	static inline float       Ceil(float x) { return ::ceilf(x); }
	static inline double      Ceil(double x) { return ::ceil(x); }

	static inline float       Floor(float x) { return ::floorf(x); }
	static inline double      Floor(double x) { return ::floor(x); }

	template<class T>
	static inline T         Sign(const T & a);

	static float              Randf();

	static inline bool      NearEqual(float a, float b, float epsilon = 1e-5f) { return vaMath::Abs(a - b) < epsilon; }

	static inline float     Smoothstep(const float t);                                                                        // gives something similar to "sin( (x-0.5) * PI)*0.5 + 0.5 )" for [0, 1]

private:
	friend class vaCore;
	static void             Initialize();
	static void             Deinitialize();
};

// Time independent lerp function. The bigger the lerpRate, the faster the lerp!
inline float vaMath::TimeIndependentLerpF(float deltaTime, float lerpRate)
{
	return 1.0f - expf(-fabsf(deltaTime*lerpRate));
}

template<class T>
inline T vaMath::Min(T const & a, T const & b)
{
	return (a < b) ? (a) : (b);
}

template<class T>
inline T vaMath::Max(T const & a, T const & b)
{
	return (a > b) ? (a) : (b);
}

template<class T>
inline T vaMath::Clamp(T const & v, T const & min, T const & max)
{
	if (v < min) return min;
	if (v > max) return max;
	return v;
}

// short for clamp( a, 0, 1 )
template<class T>
inline T vaMath::Saturate(T const & a)
{
	return Clamp(a, (T)0.0, (T)1.0);
}

template<class T>
inline T vaMath::Lerp(T const & a, T const & b, const float f)
{
	return a + (b - a)*f;
}

// why not just use std::swap?
//template<class T>
//inline void vaMath::Swap(T & a, T & b)
//{
//    assert( &a != &b ); // swapping with itself? 
//	T temp = b;
//	b = a;
//	a = temp;
//}

template<class T>
inline T vaMath::Sq(const T & a)
{
	return a * a;
}

inline double vaMath::Frac(double a)
{
	return fmod(a, 1.0);
}

inline float vaMath::Frac(float a)
{
	return fmodf(a, 1.0);
}

inline int vaMath::FloorLog2(unsigned int n)
{
	int pos = 0;
	if (n >= 1 << 16) { n >>= 16; pos += 16; }
	if (n >= 1 << 8) { n >>= 8; pos += 8; }
	if (n >= 1 << 4) { n >>= 4; pos += 4; }
	if (n >= 1 << 2) { n >>= 2; pos += 2; }
	if (n >= 1 << 1) { pos += 1; }
	return ((n == 0) ? (-1) : pos);
}

inline float vaMath::AngleWrap(float angle)
{
	return (angle > 0) ? (fmodf(angle + 3.1415926535f, 3.1415926535f * 2.0f) - 3.1415926535f) : (fmodf(angle - 3.1415926535f, 3.1415926535f * 2.0f) + 3.1415926535f);
}

inline float vaMath::AngleSmallestDiff(float a, float b)
{
	a = AngleWrap(a);
	b = AngleWrap(b);
	float v = AngleWrap(a - b);
	if (v > 3.1415926535f)
		v -= 3.1415926535f * 2.0f;
	return v;
}

inline bool vaMath::IsPowOf2(int val)
{
	if (val < 1) return false;
	return (val & (val - 1)) == 0;
}

inline int vaMath::PowOf2Ceil(int val)
{
	int l2 = Log2(Max(0, val - 1)) + 1;
	return 1 << l2;
}

inline int vaMath::Log2(int val)
{
	unsigned r = 0;

	while (val >>= 1)
	{
		r++;
	}

	return r;
}

inline float vaMath::Abs(float a)
{
	return fabsf(a);
}

inline double vaMath::Abs(double a)
{
	return fabs(a);
}

template<class T>
inline T vaMath::Sign(const T & a)
{
	if (a > 0) return 1;
	if (a < 0) return -1;
	return 0;
}

// "fat" version:
// float smoothstep(float edge0, float edge1, float x)
// {
//	// Scale, bias and saturate x to 0..1 range
//	x = clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0); 
//  // Evaluate polynomial
//	return x*x*(3 - 2*x);
//}

inline float vaMath::Smoothstep(const float t)
{
	return t * t * (3 - 2 * t);
}


inline vaVector3 &   vaVector3::operator += (const vaVector3 & v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

inline vaVector3 &   vaVector3::operator -= (const vaVector3 & v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

inline vaVector3 &   vaVector3::operator *= (float f)
{
	x *= f;
	y *= f;
	z *= f;
	return *this;
}

inline vaVector3 &   vaVector3::operator /= (float f)
{
	float oneOverF = 1.0f / f;
	x *= oneOverF;
	y *= oneOverF;
	z *= oneOverF;
	return *this;
}

// unary
inline vaVector3     vaVector3::operator + () const
{
	return *this;
}

inline vaVector3     vaVector3::operator - () const
{
	return vaVector3(-x, -y, -z);
}

// binary operators
inline vaVector3     vaVector3::operator + (const vaVector3 & v) const
{
	return vaVector3(x + v.x, y + v.y, z + v.z);
}

inline vaVector3     vaVector3::operator - (const vaVector3 & v) const
{
	return vaVector3(x - v.x, y - v.y, z - v.z);
}

inline vaVector3     vaVector3::operator * (const vaVector3 & v) const
{
	return vaVector3(x * v.x, y * v.y, z * v.z);
}

inline vaVector3     vaVector3::operator / (const vaVector3 & v) const
{
	return vaVector3(x / v.x, y / v.y, z / v.z);
}

inline vaVector3     vaVector3::operator + (float f) const
{
	return vaVector3(x + f, y + f, z + f);
}

inline vaVector3     vaVector3::operator - (float f) const
{
	return vaVector3(x - f, y - f, z - f);
}

inline vaVector3     vaVector3::operator * (float f) const
{
	return vaVector3(x * f, y * f, z * f);
}

inline vaVector3     vaVector3::operator / (float f) const
{
	float oneOverF = 1.0f / f;
	return vaVector3(x * oneOverF, y * oneOverF, z * oneOverF);
}

// friend
inline vaVector3 operator * (float f, const class vaVector3 & v)
{
	return vaVector3(f * v.x, f * v.y, f * v.z);
}

inline bool          vaVector3::operator == (const vaVector3 & v) const
{
	return x == v.x && y == v.y && z == v.z;
}

inline bool          vaVector3::operator != (const vaVector3 & v) const
{
	return x != v.x || y != v.y || z != v.z;
}

// other
inline float         vaVector3::Length() const
{
	return vaMath::Sqrt(x * x + y * y + z * z);
}
inline float         vaVector3::LengthSq() const
{
	return x * x + y * y + z * z;
}
inline vaVector3     vaVector3::Normalize() const
{
	vaVector3 ret;
	float length = Length();

	if (length < 1e-6f)
	{
		ret.x = 0.0f;
		ret.y = 0.0f;
		ret.z = 0.0f;
	}
	else
	{
		ret.x = this->x / length;
		ret.y = this->y / length;
		ret.z = this->z / length;
	}

	return ret;
}

inline bool vaVector3::IsNormal(float epsilon)
{
	return vaMath::Abs(Length() - 1.0f) <= epsilon;
}

inline vaVector3     vaVector3::ComponentAbs() const
{
	return vaVector3(vaMath::Abs(x), vaMath::Abs(y), vaMath::Abs(z));
}

// static

inline float         vaVector3::Dot(const vaVector3 & a, const vaVector3 & b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vaVector3     vaVector3::Cross(const vaVector3 & a, const vaVector3 & b)
{
	vaVector3 ret;
	ret.x = a.y * b.z - a.z * b.y;
	ret.y = a.z * b.x - a.x * b.z;
	ret.z = a.x * b.y - a.y * b.x;
	return ret;
}

inline bool vaVector3::CloseEnough(const vaVector3 & a, const vaVector3 & b, float epsilon)
{
	vaVector3 r = (a - b).ComponentAbs();
	return (r.x < epsilon) && (r.y < epsilon) && (r.z < epsilon);
}

inline vaVector3     vaVector3::ComponentMul(const vaVector3 & a, const vaVector3 & b)
{
	return vaVector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline vaVector3     vaVector3::ComponentDiv(const vaVector3 & a, const vaVector3 & b)
{
	return vaVector3(a.x / b.x, a.y / b.y, a.z / b.z);
}

inline vaVector3     vaVector3::ComponentMin(const vaVector3 & a, const vaVector3 & b)
{
	return vaVector3(vaMath::Min(a.x, b.x), vaMath::Min(a.y, b.y), vaMath::Min(a.z, b.z));
}

inline vaVector3     vaVector3::ComponentMax(const vaVector3 & a, const vaVector3 & b)
{
	return vaVector3(vaMath::Max(a.x, b.x), vaMath::Max(a.y, b.y), vaMath::Max(a.z, b.z));
}

inline vaVector3     vaVector3::BaryCentric(const vaVector3 & v1, const vaVector3 & v2, const vaVector3 & v3, float f, float g)
{
	return v1 + f * (v2 - v1) + g * (v3 - v1);
}

inline vaVector3     vaVector3::Hermite(const vaVector3 & v1, const vaVector3 & t1, const vaVector3 & v2, const vaVector3 &t2, float s)
{
	float h1, h2, h3, h4;

	h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
	h2 = s * s * s - 2.0f * s * s + s;
	h3 = -2.0f * s * s * s + 3.0f * s * s;
	h4 = s * s * s - s * s;

	vaVector3 ret;
	ret.x = h1 * v1.x + h2 * t1.x + h3 * v2.x + h4 * t2.x;
	ret.y = h1 * v1.y + h2 * t1.y + h3 * v2.y + h4 * t2.y;
	ret.z = h1 * v1.z + h2 * t1.z + h3 * v2.z + h4 * t2.z;
	return ret;
}

inline vaVector3     vaVector3::CatmullRom(const vaVector3 &v0, const vaVector3 &v1, const vaVector3 & v2, const vaVector3 & v3, float s)
{
	vaVector3 ret;
	ret.x = 0.5f * (2.0f * v1.x + (v2.x - v0.x) *s + (2.0f * v0.x - 5.0f * v1.x + 4.0f * v2.x - v3.x) * s * s + (v3.x - 3.0f * v2.x + 3.0f * v1.x - v0.x) * s * s * s);
	ret.y = 0.5f * (2.0f * v1.y + (v2.y - v0.y) *s + (2.0f * v0.y - 5.0f * v1.y + 4.0f * v2.y - v3.y) * s * s + (v3.y - 3.0f * v2.y + 3.0f * v1.y - v0.y) * s * s * s);
	ret.z = 0.5f * (2.0f * v1.z + (v2.z - v0.z) *s + (2.0f * v0.z - 5.0f * v1.z + 4.0f * v2.z - v3.z) * s * s + (v3.z - 3.0f * v2.z + 3.0f * v1.z - v0.z) * s * s * s);
	return ret;
}

inline float vaVector3::AngleBetweenVectors(const vaVector3 & a, const vaVector3 & b)
{
	return vaMath::ACos(vaVector3::Dot(a, b) / (a.Length() * b.Length()));
}



inline vaQuaternion & vaQuaternion::operator += (const vaQuaternion & q)
{
	x += q.x;
	y += q.y;
	z += q.z;
	w += q.w;
	return *this;
}

inline vaQuaternion & vaQuaternion::operator -= (const vaQuaternion& q)
{
	x -= q.x;
	y -= q.y;
	z -= q.z;
	w -= q.w;
	return *this;
}

inline vaQuaternion & vaQuaternion::operator *= (const vaQuaternion& q)
{
	*this = vaQuaternion::Multiply(*this, q);
	return *this;
}

inline vaQuaternion & vaQuaternion::operator *= (float f)
{
	x *= f;
	y *= f;
	z *= f;
	w *= f;
	return *this;
}

inline vaQuaternion & vaQuaternion::operator /= (float f)
{
	float oneOverF = 1.0f / f;
	x *= oneOverF;
	y *= oneOverF;
	z *= oneOverF;
	w *= oneOverF;
	return *this;
}

// unary
inline vaQuaternion vaQuaternion::operator + () const
{
	return *this;
}

inline vaQuaternion vaQuaternion::operator - () const
{
	return vaQuaternion(-x, -y, -z, -w);
}

// binary
inline vaQuaternion vaQuaternion::operator + (const vaQuaternion & q) const
{
	return vaQuaternion(x + q.x, y + q.y, z + q.z, w + q.w);
}

inline vaQuaternion vaQuaternion::operator - (const vaQuaternion & q) const
{
	return vaQuaternion(x - q.x, y - q.y, z - q.z, w - q.w);
}

inline vaQuaternion vaQuaternion::operator * (const vaQuaternion & q) const
{
	return vaQuaternion::Multiply(*this, q);
}

inline vaQuaternion vaQuaternion::operator * (float f) const
{
	return vaQuaternion(x * f, y * f, z * f, w * f);
}

inline vaQuaternion vaQuaternion::operator / (float f) const
{
	float oneOverF = 1.0f / f;
	return vaQuaternion(x * oneOverF, y * oneOverF, z * oneOverF, w * oneOverF);
}

inline vaQuaternion operator * (float f, const vaQuaternion & q)
{
	return vaQuaternion(f * q.x, f * q.y, f * q.z, f * q.w);
}

inline bool vaQuaternion::operator == (const vaQuaternion & q) const
{
	return x == q.x && y == q.y && z == q.z && w == q.w;
}

inline bool vaQuaternion::operator != (const vaQuaternion & q) const
{
	return x != q.x || y != q.y || z != q.z || w != q.w;
}

// static
inline vaQuaternion vaQuaternion::Multiply(const vaQuaternion & a, const vaQuaternion & b)
{
	vaQuaternion ret;
	ret.x = b.w * a.x + b.x * a.w + b.y * a.z - b.z * a.y;
	ret.y = b.w * a.y - b.x * a.z + b.y * a.w + b.z * a.x;
	ret.z = b.w * a.z + b.x * a.y - b.y * a.x + b.z * a.w;
	ret.w = b.w * a.w - b.x * a.x - b.y * a.y - b.z * a.z;
	return ret;
}

inline vaQuaternion vaQuaternion::Normalize() const
{
	return (*this) / Length();
}

inline vaQuaternion vaQuaternion::Inverse() const
{
	vaQuaternion ret;

	float norm = LengthSq();

	ret.x = -this->x / norm;
	ret.y = -this->y / norm;
	ret.z = -this->z / norm;
	ret.w = this->w / norm;

	return ret;
}

inline float vaQuaternion::Length() const
{
	return vaMath::Sqrt(x * x + y * y + z * z + w * w);
}

inline float vaQuaternion::LengthSq() const
{
	return x * x + y * y + z * z + w * w;
}

inline vaQuaternion vaQuaternion::Conjugate() const
{
	return vaQuaternion(-x, -y, -z, w);
}

inline void vaQuaternion::ToAxisAngle(vaVector3 & outAxis, float & outAngle) const
{
	outAxis.x = this->x;
	outAxis.y = this->y;
	outAxis.z = this->z;
	outAngle = 2.0f * vaMath::ACos(this->w);
}

inline vaQuaternion vaQuaternion::Ln() const
{
	vaQuaternion ret;

	float norm, normvec, theta;

	norm = LengthSq();

	if (norm > 1.0001f)
	{
		ret.x = this->x;
		ret.y = this->y;
		ret.z = this->z;
		ret.w = 0.0f;
	}
	else if (norm > 0.99999f)
	{
		normvec = sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
		theta = vaMath::ATan2(normvec, this->w) / normvec;
		ret.x = theta * this->x;
		ret.y = theta * this->y;
		ret.z = theta * this->z;
		ret.w = 0.0f;
	}
	else
	{
		assert(false);
		// FIXME("The quaternion (%f, %f, %f, %f) has a norm <1. This should not happen. Windows returns a result anyway. This case is not implemented yet.\n", pq->x, pq->y, pq->z, pq->w);
	}

	return ret;
}

inline vaQuaternion vaQuaternion::Exp() const
{
	vaQuaternion ret;
	float norm;

	norm = vaMath::Sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
	if (norm)
	{
		ret.x = sin(norm) * this->x / norm;
		ret.y = sin(norm) * this->y / norm;
		ret.z = sin(norm) * this->z / norm;
		ret.w = cos(norm);
	}
	else
	{
		ret.x = 0.0f;
		ret.y = 0.0f;
		ret.z = 0.0f;
		ret.w = 1.0f;
	}
	return ret;
}

inline vaVector3 vaQuaternion::GetAxisX() const
{
	vaVector3 ret;

	ret[0] = 1.0f - 2.0f * (this->y * this->y + this->z * this->z);
	ret[1] = 2.0f * (this->x *this->y + this->z * this->w);
	ret[2] = 2.0f * (this->x * this->z - this->y * this->w);

	return ret;
}

inline vaVector3 vaQuaternion::GetAxisY() const
{
	vaVector3 ret;

	ret[0] = 2.0f * (this->x * this->y - this->z * this->w);
	ret[1] = 1.0f - 2.0f * (this->x * this->x + this->z * this->z);
	ret[2] = 2.0f * (this->y *this->z + this->x *this->w);

	return ret;
}

inline vaVector3 vaQuaternion::GetAxisZ() const
{
	vaVector3 ret;

	ret[0] = 2.0f * (this->x * this->z + this->y * this->w);
	ret[1] = 2.0f * (this->y *this->z - this->x *this->w);
	ret[2] = 1.0f - 2.0f * (this->x * this->x + this->y * this->y);

	return ret;
}

vaQuaternion vaQuaternion::RotationAxis(const vaVector3 & v, float angle)
{
	vaVector3 temp = v.Normalize();

	vaQuaternion ret;
	float hsin = vaMath::Sin(angle / 2.0f);
	ret.x = hsin * temp.x;
	ret.y = hsin * temp.y;
	ret.z = hsin * temp.z;
	ret.w = vaMath::Cos(angle / 2.0f);
	return ret;
}

vaQuaternion vaQuaternion::RotationYawPitchRoll(float yaw, float pitch, float roll)
{
	vaQuaternion ret;
	ret.x = vaMath::Cos(yaw / 2.0f) * vaMath::Cos(pitch / 2.0f) * vaMath::Sin(roll / 2.0f) - vaMath::Sin(yaw / 2.0f) * vaMath::Sin(pitch / 2.0f) * vaMath::Cos(roll / 2.0f);
	ret.y = vaMath::Sin(yaw / 2.0f) * vaMath::Cos(pitch / 2.0f) * vaMath::Sin(roll / 2.0f) + vaMath::Cos(yaw / 2.0f) * vaMath::Sin(pitch / 2.0f) * vaMath::Cos(roll / 2.0f);
	ret.z = vaMath::Sin(yaw / 2.0f) * vaMath::Cos(pitch / 2.0f) * vaMath::Cos(roll / 2.0f) - vaMath::Cos(yaw / 2.0f) * vaMath::Sin(pitch / 2.0f) * vaMath::Sin(roll / 2.0f);
	ret.w = vaMath::Cos(yaw / 2.0f) * vaMath::Cos(pitch / 2.0f) * vaMath::Cos(roll / 2.0f) + vaMath::Sin(yaw / 2.0f) * vaMath::Sin(pitch / 2.0f) * vaMath::Sin(roll / 2.0f);
	return ret;
}

inline float vaQuaternion::Dot(const vaQuaternion & a, const vaQuaternion & b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline vaQuaternion vaQuaternion::BaryCentric(const vaQuaternion & q1, const vaQuaternion & q2, const vaQuaternion & q3, float f, float g)
{
	vaQuaternion temp1 = vaQuaternion::Slerp(q1, q2, f + g);
	vaQuaternion temp2 = vaQuaternion::Slerp(q1, q3, f + g);
	return vaQuaternion::Slerp(temp1, temp2, g / (f + g));
}

vaQuaternion vaQuaternion::Slerp(const vaQuaternion & q1, const vaQuaternion & q2, float t)
{
	vaQuaternion ret;

	float dot, epsilon, temp, theta, u;

	epsilon = 1.0f;
	temp = 1.0f - t;
	u = t;
	dot = vaQuaternion::Dot(q1, q2);
	if (dot < 0.0f)
	{
		epsilon = -1.0f;
		dot = -dot;
	}
	if (1.0f - dot > 0.001f)
	{
		theta = acos(dot);
		temp = sin(theta * temp) / sin(theta);
		u = sin(theta * u) / sin(theta);
	}
	ret.x = temp * q1.x + epsilon * u * q2.x;
	ret.y = temp * q1.y + epsilon * u * q2.y;
	ret.z = temp * q1.z + epsilon * u * q2.z;
	ret.w = temp * q1.w + epsilon * u * q2.w;
	return ret;
}

vaQuaternion vaQuaternion::Squad(const vaQuaternion & q1, const vaQuaternion & q2, const vaQuaternion & q3, const vaQuaternion & q4, float t)
{
	vaQuaternion temp1 = vaQuaternion::Slerp(q1, q4, t);
	vaQuaternion temp2 = vaQuaternion::Slerp(q2, q3, t);

	return vaQuaternion::Slerp(temp1, temp2, 2.0f * t * (1.0f - t));
}


void vaCameraControllerFocusLocationsFlythrough::CameraTick(float deltaTime, Camera *camera, bool hasFocus)
{
	// TODO -- replace camera with cauldron camera
	if (m_keys.size() == 0)
		return;

	m_currentKeyTimeRemaining -= deltaTime;

	while (m_currentKeyTimeRemaining < 0)
	{
		m_currentKeyIndex = (m_currentKeyIndex + 1) % m_keys.size();
		m_currentKeyTimeRemaining += m_keys[m_currentKeyIndex].ShowTime;
	}

	Keyframe & currentKey = m_keys[m_currentKeyIndex];
	Keyframe & nextKey = m_keys[(m_currentKeyIndex + 1) % m_keys.size()];

	float lerpK = vaMath::Smoothstep(1.0f - m_currentKeyTimeRemaining / currentKey.ShowTime);

	vaVector3 pos = currentKey.Position * (1.0f - lerpK) + nextKey.Position * lerpK;
	m_userParam0 = currentKey.UserParam0 * (1.0f - lerpK) + nextKey.UserParam0 * lerpK;
	m_userParam1 = currentKey.UserParam1 * (1.0f - lerpK) + nextKey.UserParam1 * lerpK;
	vaQuaternion rot = vaQuaternion::Slerp(currentKey.Orientation, nextKey.Orientation, lerpK);

	if (m_fixedUp)
	{
		vaVector3 currentUp = rot.GetAxisY();

		vaVector3 rotAxis = vaVector3::Cross(currentUp, m_fixedUpVec);
		float rotAngle = vaVector3::AngleBetweenVectors(currentUp, m_fixedUpVec);

		rot *= vaQuaternion::RotationAxis(rotAxis, rotAngle);
	}

	float lf = vaMath::TimeIndependentLerpF(deltaTime, 5.0f / (currentKey.ShowTime + 2.0f));

	/*
	XMVECTOR cameraPosXMVec = camera->GetPosition();
	vaVector3 cameraPos = vaVector3(cameraPosXMVec.m128_f32[0], cameraPosXMVec.m128_f32[1], cameraPosXMVec.m128_f32[2]);
	vaQuaternion cameraOrientation = vaQuaternion::RotationYawPitchRoll(camera->GetYaw(), camera->GetPitch(), 0.0f);
	pos = vaMath::Lerp(cameraPos, pos, lf);
	rot = vaQuaternion::Slerp(cameraOrientation, rot, lf);
	*/

	pos *= 5.0f / 7.0f;
	vaVector3 lookAtPos = pos + camera->GetDistance() * rot.GetAxisX();
	XMVECTOR cameraEyePos = XMVectorSet(pos.x, pos.z, pos.y, 0.0f);
	XMVECTOR cameraLookAtPos = XMVectorSet(lookAtPos.x, lookAtPos.z, lookAtPos.y, 0.0f);
	camera->LookAt(cameraEyePos, cameraLookAtPos);
}

// =======================================================================================
// end of stuff ripped from intel ASSAO
// =======================================================================================

static const FfxCacaoPreset FFX_CACAO_PRESETS[] = {
	// Native - High Quality
	{
		/* useDownsampledSsao */ false,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_HIGHEST,
			/* adaptiveQualityLimit              */ 0.45f,
			/* blurPassCount                     */ 2,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.01f,
		}
	},
	// Native - Medium Quality
	{
		/* useDownsampledSsao */ false,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_MEDIUM,
			/* adaptiveQualityLimit              */ 0.45f,
			/* blurPassCount                     */ 2,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.01f,
		}
	},
	// Native - Low Quality
	{
		/* useDownsampledSsao */ false,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_LOW,
			/* adaptiveQualityLimit              */ 0.45f,
			/* blurPassCount                     */ 4,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.01f,
		}
	},
	// Downsampled - High Quality
	{
		/* useDownsampledSsao */ true,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_HIGHEST,
			/* adaptiveQualityLimit              */ 0.45f,
			/* blurPassCount                     */ 2,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.01f,
		}
	},
	// Downsampled - Medium Quality
	{
		/* useDownsampledSsao */ true,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_MEDIUM,
			/* adaptiveQualityLimit              */ 0.45f,
			/* blurPassCount                     */ 3,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 5.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.2f,
		}
	},
	// Downsampled - Low Quality
	{
		/* useDownsampledSsao */ true,
		{
			/* radius                            */ 1.2f,
			/* shadowMultiplier                  */ 1.0f,
			/* shadowPower                       */ 1.50f,
			/* shadowClamp                       */ 0.98f,
			/* horizonAngleThreshold             */ 0.06f,
			/* fadeOutFrom                       */ 20.0f,
			/* fadeOutTo                         */ 40.0f,
			/* qualityLevel                      */ FFX_CACAO_QUALITY_LOWEST,
			/* adaptiveQualityLimit              */ 0.45f,
			/* blurPassCount                     */ 6,
			/* sharpness                         */ 0.98f,
			/* temporalSupersamplingAngleOffset  */ 0.0f,
			/* temporalSupersamplingRadiusOffset */ 0.0f,
			/* detailShadowStrength              */ 0.5f,
			/* generateNormals                   */ FFX_CACAO_FALSE,
			/* bilateralSigmaSquared             */ 8.0f,
			/* bilateralSimilarityDistanceSigma  */ 0.8f,
		}
	}
};


static inline void SetWindowClientSize(HWND hWnd, LONG width, LONG height)
{
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;
	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	AdjustWindowRect(&rect, style, FALSE);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
	SetWindowPos(hWnd, NULL, -1, -1, width, height, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}


const bool VALIDATION_ENABLED = false;

FfxCacaoSample::FfxCacaoSample(LPCSTR name) : FrameworkWindows(name)
{
    m_lastFrameTime = MillisecondsNow();
    m_time = 0;
    m_bPlay = true;

    m_pGltfLoader = NULL;
}

//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnCreate(HWND hWnd)
{
	m_hWnd = hWnd;
	m_selectedResolution = 0;
	m_filenameBuffer[0] = 0;
	GetCurrentDirectory(_countof(m_filenameBuffer), m_filenameBuffer);
	strncat(m_filenameBuffer, "\\output", _countof(m_filenameBuffer));

	m_displayGUI = true;
	m_cameraControlSelected = 1;
	m_flythroughCameraController.SetFixedUp(true); // so that we can seamlessly switch between flythrough and manual camera

	m_presetIndex = 0;
	m_currentSettings = FFX_CACAO_PRESETS[m_presetIndex];
	// Camera fly path taken from Intel ASSAO
	const float keyTime = 10.0f;
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(9.142f, -0.315f, 3.539f), vaQuaternion(0.555f, 0.552f, 0.439f, 0.441f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(11.782f, -0.078f, 1.812f), vaQuaternion(0.463f, -0.433f, -0.528f, 0.565f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(5.727f, -1.077f, 2.716f), vaQuaternion(-0.336f, 0.619f, 0.624f, -0.339f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-2.873f, 1.043f, 2.808f), vaQuaternion(0.610f, -0.378f, -0.367f, 0.592f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-7.287f, 1.254f, 2.598f), vaQuaternion(0.757f, 0.004f, 0.003f, 0.654f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-12.750f, 0.051f, 2.281f), vaQuaternion(0.543f, 0.448f, 0.452f, 0.548f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-14.431f, -3.854f, 2.411f), vaQuaternion(0.556f, 0.513f, 0.443f, 0.481f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-14.471f, -6.127f, 1.534f), vaQuaternion(0.422f, 0.520f, 0.577f, 0.467f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-8.438f, -5.876f, 4.094f), vaQuaternion(0.391f, 0.784f, 0.432f, 0.215f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-2.776f, -4.915f, 1.890f), vaQuaternion(0.567f, 0.646f, 0.384f, 0.337f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-1.885f, -4.796f, 2.499f), vaQuaternion(0.465f, 0.536f, 0.532f, 0.462f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(1.569f, -4.599f, 3.303f), vaQuaternion(0.700f, 0.706f, 0.079f, 0.078f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(4.799f, -5.682f, 3.353f), vaQuaternion(0.037f, 0.900f, 0.434f, 0.018f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(7.943f, -5.405f, 3.416f), vaQuaternion(-0.107f, 0.670f, 0.725f, -0.115f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(11.445f, -3.276f, 3.319f), vaQuaternion(-0.455f, 0.589f, 0.529f, -0.409f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(12.942f, 2.277f, 3.367f), vaQuaternion(0.576f, -0.523f, -0.423f, 0.465f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(12.662f, 3.895f, 4.186f), vaQuaternion(0.569f, -0.533f, -0.428f, 0.457f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(8.688f, 4.170f, 4.107f), vaQuaternion(0.635f, -0.367f, -0.340f, 0.588f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(6.975f, 1.525f, 4.299f), vaQuaternion(0.552f, -0.298f, -0.369f, 0.685f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(5.497f, -0.418f, 7.013f), vaQuaternion(0.870f, -0.124f, -0.067f, 0.473f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(9.520f, -2.108f, 6.619f), vaQuaternion(0.342f, 0.599f, 0.629f, 0.359f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(11.174f, 3.226f, 6.969f), vaQuaternion(-0.439f, 0.536f, 0.558f, -0.457f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-2.807f, 5.621f, 7.026f), vaQuaternion(0.694f, 0.013f, 0.014f, 0.720f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-11.914f, 5.271f, 7.026f), vaQuaternion(0.694f, 0.013f, 0.014f, 0.720f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-12.168f, 1.401f, 7.235f), vaQuaternion(0.692f, -0.010f, -0.011f, 0.722f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-6.541f, 0.038f, 7.491f), vaQuaternion(0.250f, -0.287f, -0.697f, 0.608f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-6.741f, 0.257f, 2.224f), vaQuaternion(0.511f, -0.465f, -0.487f, 0.535f), keyTime));
	m_flythroughCameraController.AddKey(vaCameraControllerFocusLocationsFlythrough::Keyframe(vaVector3(-10.913f, -0.020f, 2.766f), vaQuaternion(0.511f, -0.471f, -0.487f, 0.529f), keyTime));


	DWORD dwAttrib = GetFileAttributes("..\\media\\");
    if ((dwAttrib == INVALID_FILE_ATTRIBUTES) || ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) == 0)
    {
        MessageBox(NULL, "Media files not found!\n\nPlease check the readme on how to get the media files.", "Cauldron Panic!", MB_ICONERROR);
        exit(0);
    }

	m_fullscreen = false;

    // Create Device
    //
    m_device.OnCreate("myapp", "myEngine", VALIDATION_ENABLED, hWnd);
    m_device.CreatePipelineCache();

    //init the shader compiler
    CreateShaderCache();

    // Create Swapchain
    //

    // Init FS2 and choose format
    fs2Init(m_device.GetAGSContext(), m_device.GetAGSGPUInfo(), hWnd);

    uint32_t dwNumberOfBackBuffers = 2;
    m_swapChain.OnCreate(&m_device, dwNumberOfBackBuffers, hWnd);

    // Create a instance of the renderer and initialize it, we need to do that for each GPU
    //
    m_Node = new SampleRenderer();
    m_Node->OnCreate(&m_device, &m_swapChain);

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
    m_state.skyDomeType = 0;
    m_state.exposure = 1.0f;
    m_state.iblFactor = 2.0f;
    m_state.emmisiveFactor = 1.0f;
    m_state.bDrawLightFrustum = false;
    m_state.bDrawBoundingBoxes = false;
    m_state.camera.LookAt(m_roll, m_pitch, m_distance, XMVectorSet(0, 0, 0, 0));

    m_state.spotlightCount = 1;

    m_state.spotlight[0].intensity = 50.0f;
    m_state.spotlight[0].color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    m_state.spotlight[0].light.SetFov(XM_PI / 2.0f, 1024, 1024, 0.1f, 100.0f);
    m_state.spotlight[0].light.LookAt(XM_PI / 2.0f, 0.58f, 3.5f, XMVectorSet(0, 0, 0, 0));

	m_state.bUseCACAO = true;
}

//--------------------------------------------------------------------------------------
//
// OnDestroy
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnDestroy()
{
    ImGUI_Shutdown();

    m_device.GPUFlush();

    // Fullscreen state should always be false before exiting the app.
    m_swapChain.SetFullScreen(false);

    m_Node->UnloadScene();
    m_Node->OnDestroyWindowSizeDependentResources();
    m_Node->OnDestroy();

    delete m_Node;

    m_swapChain.OnDestroyWindowSizeDependentResources();
    m_swapChain.OnDestroy();

    //shut down the shader compiler 
    DestroyShaderCache(&m_device);

    if (m_pGltfLoader)
    {
        delete m_pGltfLoader;
        m_pGltfLoader = NULL;
    }

    m_device.OnDestroy();
}

//--------------------------------------------------------------------------------------
//
// OnEvent
//
//--------------------------------------------------------------------------------------
bool FfxCacaoSample::OnEvent(MSG msg)
{
    if (ImGUI_WndProcHandler(msg.hwnd, msg.message, msg.wParam, msg.lParam))
        return true;

	if (msg.message == WM_KEYDOWN)
	{
		switch (msg.wParam)
		{
		case VK_TAB:
			m_displayGUI = !m_displayGUI;
			break;
		case '1':
			m_cacaoOutputDirectly = true;
			m_state.bUseCACAO = true;
			break;
		case '2':
			m_cacaoOutputDirectly = false;
			m_state.bUseCACAO = true;
			break;
		case '3':
			m_cacaoOutputDirectly = false;
			m_state.bUseCACAO = false;
			break;
		default:
			break;
		}
	}

    return true;
}

//--------------------------------------------------------------------------------------
//
// SetFullScreen
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::SetFullScreen(bool fullscreen)
{
    m_device.GPUFlush();

    m_swapChain.SetFullScreen(fullscreen);
}

//--------------------------------------------------------------------------------------
//
// OnResize
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnResize(uint32_t width, uint32_t height, bool force)
{
    if (m_Width != width || m_Height != height || force)
    {
        // Flush GPU
        //
        m_device.GPUFlush();

        // If resizing but no minimizing
        //
        if (m_Width > 0 && m_Height > 0)
        {
            if (m_Node!=NULL)
            {
                m_Node->OnDestroyWindowSizeDependentResources();
            }
            m_swapChain.OnDestroyWindowSizeDependentResources();
        }

        m_Width = width;
        m_Height = height;

        // if resizing but not minimizing the recreate it with the new size
        //
        if (m_Width > 0 && m_Height > 0)
        {
            m_swapChain.OnCreateWindowSizeDependentResources(m_Width, m_Height, m_vsyncEnabled, DISPLAYMODE_SDR);
            if (m_Node != NULL)
            {
                m_Node->OnCreateWindowSizeDependentResources(&m_swapChain, m_Width, m_Height);
            }
        }
    }
    m_state.camera.SetFov(XM_PI / 4, m_Width, m_Height, 0.1f, 1000.0f);
}

//--------------------------------------------------------------------------------------
//
// OnRender, updates the state from the UI, animates, transforms and renders the scene
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnRender()
{
	// Build UI and set the scene state. Note that the rendering of the UI happens later.
	//
	ImGUI_UpdateIO();
	ImGui::NewFrame();

	bool toggleVsync = false;

	if (m_requiresLoad)
	{
		m_requiresLoad = false;
		m_pGltfLoader = new GLTFCommon();
		bool res = false;
		m_state.iblFactor = 0.36f;
		m_state.emmisiveFactor = 1.0f;
		m_state.spotlight[0].intensity = 10.0f;
		m_pitch = 0.182035938f; m_roll = 1.92130506f; m_distance = 4.83333349f;
		m_state.camera.LookAt(m_roll, m_pitch, m_distance, XMVectorSet(0.703276634f, 1.02280307f, 0.218072295f, 0));
		res = m_pGltfLoader->Load("..\\media\\sponza\\gltf\\", "sponza.gltf");

		if (res == false)
		{
			MessageBox(NULL, "The selected model couldn't be found, please check the documentation", "Cauldron Panic!", MB_ICONERROR);
			exit(0);
		}
		else
		{
			m_loadingStage = m_Node->LoadScene(m_pGltfLoader, 0);
		}
	}


    // Get timings
    //
    double timeNow = MillisecondsNow();
    m_deltaTime = timeNow - m_lastFrameTime;
    m_lastFrameTime = timeNow;

    if (m_loadingStage == 0)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameBorderSize = 1.0f;

        bool opened = true;
		if (m_displayGUI)
		{
			ImGui::Begin("CACAO Sample", &opened);

#if FFX_CACAO_PROFILE_SAMPLE
			if (!m_isCapturing)
			{
#endif
				if (ImGui::CollapsingHeader("Sample Settings", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Resolution       : %ix%i", m_Width, m_Height);

#if FFX_CACAO_PROFILE_SAMPLE
					if (ImGui::Button("Toggle Fullscreen"))
					{
						m_fullscreen = !m_fullscreen;
						SetFullScreen(m_fullscreen);
					}
					if (ImGui::Button("Set Resolution 3840x2160"))
					{
						SetWindowClientSize(m_hWnd, 3840, 2160);
					}
					if (ImGui::Button("Set Resolution 2560x1440"));
					{
						SetWindowClientSize(m_hWnd, 2560, 1440);
					}
					if (ImGui::Button("Set Resolution 1920x1080"))
					{
						SetWindowClientSize(m_hWnd, 1920, 1080);
					}
#endif

					const char *cameraControl[] = { "WASD", "Orbit", "Auto fly" };
					// static int cameraControlSelected = 1;
					if (ImGui::Combo("Camera", &m_cameraControlSelected, cameraControl, _countof(cameraControl)))
					{
						if (m_cameraControlSelected == 2)
						{
							m_flythroughCameraController.ResetTime();
						}
					}

					if (ImGui::Checkbox("VSync", &m_vsyncEnabled))
					{
						OnResize(m_Width, m_Height, true);
					}
				}

				if (ImGui::CollapsingHeader("CACAO Settings", ImGuiTreeNodeFlags_DefaultOpen))
				{
					const char *presets[] = { "Native - High Quality", "Native - Medium Quality", "Native - Low Quality", "Downsampled - High Quality", "Downsampled - Medium Quality", "Downsampled - Low Quality", "Custom" };
					if (ImGui::Combo("Preset", &m_presetIndex, presets, _countof(presets)) && m_presetIndex < 6)
					{
						m_currentSettings = FFX_CACAO_PRESETS[m_presetIndex];
					}

					FfxCacaoSettings *settings = &m_currentSettings.settings;
					ImGui::SliderFloat("Radius", &settings->radius, 0.0f, 10.0f);
					ImGui::SliderFloat("Shadow Multiplier", &settings->shadowMultiplier, 0.0f, 5.0f);
					ImGui::SliderFloat("Shadow Power", &settings->shadowPower, 0.5f, 5.0f);
					ImGui::SliderFloat("Shadow Clamp", &settings->shadowClamp, 0.0f, 1.0f);
					ImGui::SliderFloat("Horizon Angle Threshold", &settings->horizonAngleThreshold, 0.0f, 0.2f);
					ImGui::SliderFloat("Fade Out From", &settings->fadeOutFrom, 1.0f, 20.0f);
					ImGui::SliderFloat("Fade Out To", &settings->fadeOutTo, 1.0f, 40.0f);
					ImGui::SliderFloat("Adaptive Quality Limit", &settings->adaptiveQualityLimit, 0.0f, 1.0f);
					const char *qualityLevels[] = { "Lowest", "Low", "Medium", "High", "Highest" };
					int idx = (int)settings->qualityLevel;
					ImGui::Combo("Quality Level", &idx, qualityLevels, _countof(qualityLevels));
					settings->qualityLevel = (FfxCacaoQuality)idx;
					ImGui::SliderInt("Blur Pass Count", (int*)&settings->blurPassCount, 0, 8);
					ImGui::SliderFloat("Sharpness", &settings->sharpness, 0.0f, 1.0f);
					ImGui::SliderFloat("Detail Shadow Strength", &settings->detailShadowStrength, 0.0f, 5.0f);
					if (m_currentSettings.useDownsampledSsao)
					{
						ImGui::SliderFloat("Bilateral Sigma Squared", &settings->bilateralSigmaSquared, 0.0f, 10.0f);
						ImGui::SliderFloat("Bilateral Similarity Distance Sigma", &settings->bilateralSimilarityDistanceSigma, 0.0f, 1.0f);
					}

					bool generateNormals = settings->generateNormals ? true : false;
					ImGui::Checkbox("Generate Normal Buffer From Depth Buffer", &generateNormals);
					settings->generateNormals = generateNormals ? FFX_CACAO_TRUE : FFX_CACAO_FALSE;
					ImGui::Checkbox("Display CACAO Output Directly", &m_cacaoOutputDirectly);
					ImGui::Checkbox("Generate SSAO downsampled", &m_currentSettings.useDownsampledSsao);


					if (!m_cacaoOutputDirectly)
					{
						ImGui::Checkbox("Use CACAO", &m_state.bUseCACAO);
					}
					m_state.bUseCACAO |= m_cacaoOutputDirectly;

					if (m_presetIndex < 6 && memcmp(&m_currentSettings, &FFX_CACAO_PRESETS[m_presetIndex], sizeof(m_currentSettings)))
					{
						m_presetIndex = 6;
					}
				}
#if FFX_CACAO_PROFILE_SAMPLE
			}
#endif

#if FFX_CACAO_ENABLE_PROFILING
			if (m_vsyncEnabled || !m_state.bUseCACAO)
			{
				// ImGui::Text("Profiling Disabled (turn off vsync)");
				ImGui::CollapsingHeader("Profiler Disabled (enable CACAO and turn off vsync)");
			}
			else
			{
				bool displayProfiling = ImGui::CollapsingHeader("Profiler", ImGuiTreeNodeFlags_DefaultOpen);

				FfxCacaoDetailedTiming timings;
				uint64_t gpuTicksPerMicrosecond;
				m_Node->GetCacaoTimings(&timings, &gpuTicksPerMicrosecond);
				gpuTicksPerMicrosecond /= 1000000;

				for (uint32_t i = 0; i < timings.numTimestamps; ++i)
				{
					FfxCacaoTimestamp *t = &timings.timestamps[i];
					if (displayProfiling)
					{
						ImGui::Text("%-50s: %7.1f us", t->label, ((double)t->ticks) / ((double)gpuTicksPerMicrosecond));
					}
				}

#if FFX_CACAO_PROFILE_SAMPLE
				if (m_isCapturing)
				{
					m_captureTimings[m_curTiming++] = timings;
					if (m_curTiming == NUM_CAPTURE_SAMPLES)
					{
						WriteCaptureFile(gpuTicksPerMicrosecond);
						m_isCapturing = false;
					}
				}

				if (displayProfiling)
				{

					if (!m_isCapturing)
					{
						ImGui::InputText("Output Filename", m_filenameBuffer, _countof(m_filenameBuffer));

						if (ImGui::Button("Make Capture"))
						{
							m_isCapturing = true;
							m_cameraControlSelected = 2;
							m_curTiming = 0;
							m_flythroughCameraController.ResetTime();
							snprintf(m_captureFilename, _countof(m_captureFilename), "%s_Q%d_%dx%d.txt", m_filenameBuffer, (int)m_cacaoSettings.qualityLevel, m_Width, m_Height);
						}
					}
					else
					{
						ImGui::Text("Capturing...");
						ImGui::Text("Filename: \"%s\"", m_captureFilename);

						if (ImGui::Button("Cancel Capture"))
						{
							m_isCapturing = false;
						}
					}
				}
#endif
			}
#endif
			ImGui::End();
		}



        // If the mouse was not used by the GUI then it's for the camera
        //
        ImGuiIO& io = ImGui::GetIO();

		if (m_cameraControlSelected == 2)
		{
			m_flythroughCameraController.CameraTick(io.DeltaTime, &m_state.camera, true);
			m_roll = m_state.camera.GetYaw();
			m_pitch = m_state.camera.GetPitch();
		}

		if (io.WantCaptureMouse == false)
        {
            if ((io.KeyCtrl == false) && (io.MouseDown[0] == true))
            {
                m_roll -= io.MouseDelta.x / 100.f;
                m_pitch += io.MouseDelta.y / 100.f;
            }

            // Choose camera movement depending on setting
            //

            if (m_cameraControlSelected == 0)
            {
                //  WASD
                //
                m_state.camera.UpdateCameraWASD(m_roll, m_pitch, io.KeysDown, io.DeltaTime);
            }
            else if (m_cameraControlSelected == 1)
            {
                //  Orbiting
                //
                m_distance -= (float)io.MouseWheel / 3.0f;
                m_distance = std::max<float>(m_distance, 0.1f);

                bool panning = (io.KeyCtrl == true) && (io.MouseDown[0] == true);

                m_state.camera.UpdateCameraPolar(m_roll, m_pitch, panning ? -io.MouseDelta.x / 100.0f : 0.0f, panning ? io.MouseDelta.y / 100.0f : 0.0f, m_distance);
            }
        }
    }
    else
    {
        // LoadScene needs to be called a number of times, the scene is not fully loaded until it returns 0
        // This is done so we can display a progress bar when the scene is loading
        m_loadingStage = m_Node->LoadScene(m_pGltfLoader, m_loadingStage);
    }

    // Set animation time
    //
    if (m_bPlay)
    {
        m_time += (float)m_deltaTime / 1000.0f;
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
	m_Node->OnRender(&m_state, &m_currentSettings.settings, &m_swapChain, m_cacaoOutputDirectly, m_currentSettings.useDownsampledSsao);

    m_swapChain.Present();
}

#if FFX_CACAO_PROFILE_SAMPLE
void FfxCacaoSample::WriteCaptureFile(uint64_t gpuTicksPerMicrosecond)
{
	FILE *fp = fopen(m_captureFilename, "w");

	fprintf(fp, "{\n");
	fprintf(fp, "\t\"settings\" : {\n");
	fprintf(fp, "\t\t\"width\"   : %d,\n", m_Width);
	fprintf(fp, "\t\t\"height\"  : %d,\n", m_Height);
	fprintf(fp, "\t\t\"quality\" : %d\n", (int)m_cacaoSettings.qualityLevel);
	fprintf(fp, "\t},\n");
	fprintf(fp, "\t\"timings\" : {\n");

	const uint32_t numTimingTypes = m_captureTimings[0].numTimestamps;
	for (uint32_t i = 0; i < numTimingTypes; ++i)
	{
		float average = (double)m_captureTimings[0].timestamps[i].ticks / (double)gpuTicksPerMicrosecond;

		fprintf(fp, "\t\t\"%s\" : {\n\t\t\t\"samples\" : [%.2f", m_captureTimings[0].timestamps[i].label, average);

		for (int j = 1; j < NUM_CAPTURE_SAMPLES; ++j)
		{
			float sample = (double)m_captureTimings[j].timestamps[i].ticks / (double)gpuTicksPerMicrosecond;
			fprintf(fp, ", %.2f", sample);
			average += sample;
		}
		average /= (float)NUM_CAPTURE_SAMPLES;
		fprintf(fp, "],\n\t\t\t\"average\" : %.2f\n\t\t}", average);

		if (i != numTimingTypes - 1)
		{
			fprintf(fp, ",");
		}

		fprintf(fp, "\n");
	}
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n");

	fclose(fp);
}
#endif


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
    LPCSTR Name = "FFX CACAO Sample v1.0";
    uint32_t Width = 1280;
    uint32_t Height = 720;

    // create new DX sample
    return RunFramework(hInstance, lpCmdLine, nCmdShow, Width, Height, new FfxCacaoSample(Name));
}
