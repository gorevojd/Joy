#version 330 core

//TODO(dima): I can still add something to the last component of this
layout (location = 0) out vec4 GAlbedoSpec;
layout (location = 1) out vec4 GNormalMetalRough;

in Vertex_Shader_Out{
    vec3 WorldP;
    vec3 ViewN;
    vec2 UV;
} FsIn;

uniform vec3 AlbedoColor;

uniform sampler2D Albedo;
uniform sampler2D Normals;
uniform sampler2D Specular;
uniform sampler2D Emissive;

uniform int TexturesSetFlags;

const int ALBEDO_SET_FLAG = 1;
const int NORMALS_SET_FLAG = 1 << 1;
const int SPECULAR_SET_FLAG = 1 << 2;
const int EMISSIVE_SET_FLAG = 1 << 3;

float PackTo01(float value);
float UnpackFrom01(float Value);
vec3 PackVec3To01(vec3 value);
vec3 UnpackVec3From01(vec3 Value);
vec2 PackNormal(vec3 Value);
vec3 UnpackNormal(vec2 Value);

void main()
{
    bool AlbedoIsSet = (TexturesSetFlags & ALBEDO_SET_FLAG) != 0;
    bool SpecularIsSet = (TexturesSetFlags & SPECULAR_SET_FLAG) != 0;
    bool NormalsIsSet = (TexturesSetFlags & NORMALS_SET_FLAG) != 0;
    bool EmissiveIsSet = (TexturesSetFlags & EMISSIVE_SET_FLAG) != 0;

    vec3 SampledAlbedo = AlbedoColor;
    if(AlbedoIsSet){
        SampledAlbedo = texture2D(Albedo, FsIn.UV).rgb;
    }

	if(NormalsIsSet){
		SampledAlbedo = texture2D(Normals, FsIn.UV).rgb;
	}

	vec3 NormalizedNormal = normalize(FsIn.ViewN);

	GNormalMetalRough.rg = PackNormal(NormalizedNormal);
	GNormalMetalRough.b = 0.6f;
	GNormalMetalRough.a = 0.2f;
	GAlbedoSpec.rgb = SampledAlbedo;
	GAlbedoSpec.a = 0.0f;
}

float PackTo01(float value)
{
	float Result = value * 0.5f + 0.5f;

	return(Result);
}

float UnpackFrom01(float Value)
{
	float Result = Value * 2.0f - 1.0f;

	return(Result);
}

vec3 PackVec3To01(vec3 value)
{
	vec3 Result;

	Result.x = PackTo01(value.x);
	Result.y = PackTo01(value.y);
	Result.z = PackTo01(value.z);

	return(Result);
}

vec3 UnpackVec3From01(vec3 Value)
{
	vec3 Result;
	
	Result.x = UnpackFrom01(Value.x);
	Result.y = UnpackFrom01(Value.y);
	Result.z = UnpackFrom01(Value.z);

	return(Result);
}

vec2 PackNormal(vec3 Value)
{
	vec2 Result = Value.xy;

	return(Result);
}

vec3 UnpackNormal(vec2 Value)
{
	float ResultZ = sqrt(1.0f - dot(Value, Value));

	vec3 Result;
	Result.xy = Value;
	Result.z = ResultZ;

	return(Result);
}