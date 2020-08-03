#version 330 core

uniform sampler2D GNormalMetalRough;
uniform sampler2D GAlbedoSpec;
uniform sampler2D GDepthTex;
uniform sampler2D SSAOInput;

uniform vec2 FarNear;

uniform bool FogEnabled;
uniform float FogDensity;
uniform float FogGradient;
uniform vec3 FogColor;

in vec2 TexCoord;
in vec2 ViewSpaceZDevidedP;
out vec4 Color;

struct point_light
{
	vec3 P;
	vec3 C;
	float Radius;
	float OneOverRadiusSq;
};

vec3 CalcDirLit(vec3 FragP, vec3 FragN, vec3 FragC);
vec3 CalcPointLit(vec3 FragP, vec3 FragN, vec3 FragC, point_light Lit);
float GetLinearizedDepth(sampler2D DepthTexture, vec2 UV, float Far, float Near);

vec3 UnpackNormal(vec2 Value);

void main()
{
	/*
	vec3 LitsColors[11];
	LitsColors[0] = vec3(1.0f, 0.0f, 0.0f);
	LitsColors[1] = vec3(0.0f, 1.0f, 0.0f);
	LitsColors[2] = vec3(0.0f, 0.0f, 1.0f);
	LitsColors[3] = vec3(1.0f, 1.0f, 0.0f);
	LitsColors[4] = vec3(1.0f, 0.0f, 1.0f);
	LitsColors[5] = vec3(0.0f, 1.0f, 1.0f);
	LitsColors[6] = vec3(1.0f, 0.5f, 0.0f);
	LitsColors[7] = vec3(0.0f, 0.5f, 1.0f);
	LitsColors[8] = vec3(1.0f, 0.0f, 0.5f);
	LitsColors[9] = vec3(0.5f, 0.0f, 1.0f);
	LitsColors[10] = vec3(0.5f, 0.8f, 0.3f);

	point_light Lits[100];
	for(int i = 0; i < 100; i++)
	{
		Lits[i].P = vec3((i / 10) * 5.0f, 4.0f, (i % 10) * 5.0f);
		Lits[i].C = LitsColors[i % 11];
		Lits[i].Radius = 7.0f;
		Lits[i].OneOverRadiusSq = 1.0f / (Lits[i].Radius * Lits[i].Radius);
	}
	*/

	float LinearDepth = GetLinearizedDepth(GDepthTex, TexCoord, FarNear.x, FarNear.y);
	float Occlusion = texture2D(SSAOInput, TexCoord).r;

	vec4 SampleNormalMetalRough = texture2D(GNormalMetalRough, TexCoord);
	vec4 SampleAlbedoSpec = texture2D(GAlbedoSpec, TexCoord);

	float ViewSpaceX = ViewSpaceZDevidedP.x * LinearDepth;
	float ViewSpaceY = ViewSpaceZDevidedP.y * LinearDepth;
	vec3 ViewSpaceP = vec3(ViewSpaceX, ViewSpaceY, LinearDepth);

    vec3 FragP = ViewSpaceP;
    vec3 FragN = UnpackNormal(SampleNormalMetalRough.xy);
	vec3 FragC = SampleAlbedoSpec.rgb;
    
    float AmbientFactor = 0.02f;
    
    vec3 ResultColor = vec3(0.0f, 0.0f, 0.0f);
    ResultColor = AmbientFactor * FragC;
	ResultColor += CalcDirLit(FragP, FragN, FragC);
	
	/*
	for(int LitIndex = 0;
		LitIndex < 100;
		LitIndex++)
	{
		ResultColor += CalcPointLit(FragP, FragN, FragC, Lits[LitIndex]);
	}
	*/

	float CamToFragLen = LinearDepth;
	float Visibility = 1.0f;
	if(FogEnabled){
		Visibility = exp(-pow((FogDensity * CamToFragLen), FogGradient));
		Visibility = clamp(Visibility, 0.0f, 1.0f);
	}
    
	//ResultColor = (1.0f - Visibility) * FogColor + Visibility * ResultColor; 
	ResultColor = FragC * vec3(Occlusion);

    Color = vec4(ResultColor, 1.0f);
}

vec3 CalcDirLit(vec3 FragP, vec3 FragN, vec3 FragC){
    vec3 DirLitDir = vec3(0.5f, -0.5f, -0.5f);
    DirLitDir = normalize(DirLitDir);
    vec3 DirLitColor = vec3(1.0f, 1.0f, 1.0f);
    
    vec3 ToLit = normalize(-DirLitDir);
    
    float DiffuseFactor = clamp(dot(ToLit, FragN), 0.0f, 1.0f);
    vec3 Result = DiffuseFactor * DirLitColor * FragC;
    
    return(Result);
}

vec3 CalcPointLit(vec3 FragP, vec3 FragN, vec3 FragC, point_light Lit)
{
	vec3 ToLitRaw = Lit.P - FragP;
	float ToLitLen = length(ToLitRaw);
	vec3 ToLit = ToLitRaw / ToLitLen;	

	float DistSq = ToLitLen * ToLitLen;
	float Temp = max(1.0f - (DistSq * Lit.OneOverRadiusSq), 0.0f);
	float Fdist = Temp * Temp;

	float DiffuseFactor = max(dot(ToLit, FragN), 0.0f);

	vec3 Result = DiffuseFactor * Lit.C * Fdist * FragC;
	return(Result);
}

float GetLinearizedDepth(sampler2D DepthTexture, vec2 UV, float Far, float Near)
{
	vec4 Sample = texture2D(DepthTexture, UV);

	float SampleDepth = Sample.r;

	float S = (-Far - Near) / (Far - Near);
	float T = -(2.0f * Far * Near) / (Far - Near);
	float LinDepth = T / (2.0f * SampleDepth + S - 1.0f);
	
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
