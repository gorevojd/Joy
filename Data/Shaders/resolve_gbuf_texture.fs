#version 330 core

in vec2 VsUV;
out vec4 Color;

uniform sampler2D TextureToResolve;
uniform int TextureResolveType;
uniform vec2 FarNear;

float PackTo01(float value);
float UnpackFrom01(float Value);
vec3 PackVec3To01(vec3 value);
vec3 UnpackVec3From01(vec3 Value);
vec2 PackNormal(vec3 Value);
vec3 UnpackNormal(vec2 Value);

void main(){
	float Near = FarNear.y;
	float Far = FarNear.x;

	vec3 FetchedColor;

	vec4 Sample = texture2D(TextureToResolve, VsUV);

	switch(TextureResolveType)
	{
		case 1:
		{
			//NOTE(dima): ALBEDO Color

			FetchedColor = Sample.rgb;
		}break;

		case 2:
		{
			//NOTE(dima): Specular color

			FetchedColor = Sample.aaa;
		}break;

		case 3:
		{
			//NOTE(dima): DEPTH
			float d = Sample.r;
	
			float S = (-Far - Near) / (Far - Near);
			float T = -(2.0f * Far * Near) / (Far - Near);
			float lin_depth = T / (2.0f * d + S - 1.0f);

			float ColorComp = lin_depth / Far;
			FetchedColor = vec3(ColorComp);
		}break;

		case 4:
		{
			//NOTE(dima): Unpacked normal
			vec2 Packed = Sample.xy;
			FetchedColor = PackVec3To01(UnpackNormal(Packed));
		}break;

		case 5:
		{
			//NOTE(dima): Metal
			FetchedColor = Sample.bbb;
		}break;

		case 6:
		{
			//NOTE(dima): Roughness
			FetchedColor = Sample.aaa;
		}break;

		case 7:
		case 8:
		{
			//NOTE(dima): SSAO, SSAOBlur
			FetchedColor = Sample.rrr;
		}break;
	}

	Color = vec4(FetchedColor, 1.0f);
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

vec3 UnpackNormal(vec2 Value)
{
	float ResultZ = sqrt(1.0f - dot(Value, Value));

	vec3 Result;
	Result.xy = Value;
	Result.z = ResultZ;

	return(Result);
}

vec2 PackNormal(vec3 Value)
{
	vec2 Result = Value.xy;

	return(Result);
}