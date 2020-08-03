#version 330 core

uniform vec2 FarNear;
uniform vec4 PerspProjCoefs;

uniform sampler2D DepthTex;
uniform sampler2D NormalMetalRoughTex;

uniform sampler2D SSAONoiseTex;
uniform samplerBuffer SSAOKernelBuf;
uniform int SSAOKernelSamplesCount;
uniform float SSAOKernelRadius;
uniform float SSAOContribution;
uniform float SSAORangeCheck;

in vec2 TexCoord;
flat in vec2 WH;

out float Occlusion;

float GetLinearizedDepth(sampler2D DepthTexture, vec2 UV, vec4 Coefs);
vec3 UnpackNormal(vec2 Value);

void main()
{
	float LinearDepth = GetLinearizedDepth(DepthTex, TexCoord, PerspProjCoefs);

	float ViewSpaceX = ((gl_FragCoord.x / WH.x) * 2.0f - 1.0f) * -LinearDepth / PerspProjCoefs.x;
	float ViewSpaceY = ((gl_FragCoord.y / WH.y) * 2.0f - 1.0f) * -LinearDepth / PerspProjCoefs.y;
		
	vec3 ViewSpaceP = vec3(ViewSpaceX, ViewSpaceY, LinearDepth);

	vec4 SampleNormalMetalRough = texture2D(NormalMetalRoughTex, TexCoord);
	vec3 ViewSpaceN = UnpackNormal(SampleNormalMetalRough.rg);
	
	vec2 ScaledTexCoords = TexCoord * WH * 0.25f;

	vec3 RandomVector = texture2D(SSAONoiseTex, ScaledTexCoords).rgb;

	vec3 N = ViewSpaceN;
	vec3 T = normalize(RandomVector - N * dot(RandomVector, N));
	vec3 B = normalize(cross(N, T));

	//mat3 TBN = mat3(T, B, N);
	mat3 TBN = transpose(mat3(T, B, N));

	float Result = 0.0f;
	
	float OneSampleContribution = 1.0f / float(SSAOKernelSamplesCount) * SSAOContribution;
	for(int SampleIndex = 0;
		SampleIndex < SSAOKernelSamplesCount;
		SampleIndex++)
	{
		vec3 FetchedSample = texelFetch(SSAOKernelBuf, SampleIndex).xyz;
	
		vec3 OrientedSample = (FetchedSample * TBN);
		vec3 SamplePos = ViewSpaceP + OrientedSample * SSAOKernelRadius;

		vec3 SampleProjected;
		SampleProjected.x = SamplePos.x * PerspProjCoefs.x;
		SampleProjected.y = SamplePos.y * PerspProjCoefs.y;
		SampleProjected /= -SamplePos.z;

		SampleProjected = (SampleProjected * 0.5f) + vec3(0.5f);

		float DepthAtSampleUV = texture2D(DepthTex, SampleProjected.xy).r;
		float RecovDepthZ = -PerspProjCoefs.w / (PerspProjCoefs.z + 2.0f * DepthAtSampleUV - 1.0f);		

		float Diff = SamplePos.z - RecovDepthZ; 
		if((Diff < 0) && (-RecovDepthZ < FarNear.x))
		{
			//Raycast hit!!!
			//float DistPower = max(mix(0.0f, 20.0f, -RecovDepthZ / FarNear.x), 1.0f);
			float DistPower = 1.0f;
			float RangeCheck = (Diff > -SSAORangeCheck) ? 1.0f : 0.0f;	
			Result += OneSampleContribution * DistPower * RangeCheck;
		}
	}
	
	Occlusion = 1.0f - Result;
}

float GetLinearizedDepth(sampler2D DepthTexture, vec2 UV, vec4 Coefs)
{
	vec4 Sample = texture2D(DepthTexture, UV);

	float LinDepth = -Coefs.w / (2.0f * Sample.r + Coefs.z - 1.0f);
	
	return(LinDepth);
}

vec3 UnpackNormal(vec2 Value)
{
	float ResultZ = sqrt(1.0f - dot(Value, Value));

	vec3 Result;
	Result.xy = Value;
	Result.z = ResultZ;

	return(Result);
}
