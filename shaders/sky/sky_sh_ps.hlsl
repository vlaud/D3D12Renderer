#include "sky_rs.hlsli"
#include "light_source.hlsli"


ConstantBuffer<sky_cb> cb : register(b1);
StructuredBuffer<spherical_harmonics> sh : register(t0);

struct ps_input
{
	float3 uv				: TEXCOORDS;
	float3 ndc				: NDC;
	float3 prevFrameNDC		: PREV_FRAME_NDC;
};

struct ps_output
{
	float4 color			: SV_Target0;
	float2 screenVelocity	: SV_Target1;
	uint objectID			: SV_Target2;
};

[RootSignature(SKY_SH_RS)]
ps_output main(ps_input IN)
{
	float2 ndc = (IN.ndc.xy / IN.ndc.z) - cb.jitter;
	float2 prevNDC = (IN.prevFrameNDC.xy / IN.prevFrameNDC.z) - cb.prevFrameJitter;

	float2 motion = (prevNDC - ndc) * float2(0.5f, -0.5f);

	ps_output OUT;
	OUT.color = float4(sh[0].evaluate(normalize(IN.uv)) * cb.intensity, 0.f);
	OUT.screenVelocity = motion;
	OUT.objectID = 0xFFFFFFFF; // -1.
	return OUT;
}
