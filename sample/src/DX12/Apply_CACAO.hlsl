#ifndef WIDTH
#define WIDTH 8
#endif

#ifndef HEIGHT
#define HEIGHT 8
#endif

#ifndef DEPTH
#define DEPTH 1
#endif

RWTexture2D<float> g_ClearTex : register(u0);

[numthreads(WIDTH, HEIGHT, DEPTH)]
void CSClear(uint2 tid : SV_DispatchThreadID)
{
	g_ClearTex[tid] = 1.0f;
}

// pass through
Texture2D <float> g_Input    : register(t0);
SamplerState      g_Bilinear : register(s0);

struct VERTEX_OUT
{
	float2 vTexture  : TEXCOORD;
	float4 vPosition : SV_POSITION;
};

float4 mainPS(VERTEX_OUT Input) : SV_Target
{
	return float4(pow(g_Input.Sample(g_Bilinear, Input.vTexture).xxx, 2.2), 1.0f);
}