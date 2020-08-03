#version 330 core

in vec2 VsUV;

out float Occlusion;

uniform sampler2D InputTex;
uniform float Filter[25];
uniform int FilterType;

void main()
{
	vec2 TexelSize = vec2(1.0f) / textureSize(InputTex, 0);
	
	float Result = 0.0f;

	switch(FilterType)
	{
		case 0:
		{
			//NOTE(dima): Gaussian blur 5x5
			
			for(int y = -2; y <= 2; y++)
			{
				for(int x = -2; x <= 2; x++)
				{
					vec2 Offset = vec2(x, y) * TexelSize;
					float BoxFilterValue = Filter[(y + 2) * 5 + (x + 2)];

					Result += texture2D(InputTex, VsUV + Offset).r * BoxFilterValue;
				}
			}
		}break;

		case 1:
		{
			//NOTE(dima): Gaussian blur 3x3
			
			for(int y = -1; y <= 1; y++)
			{
				for(int x = -1; x <= 1; x++)
				{
					vec2 Offset = vec2(x, y) * TexelSize;
					float BoxFilterValue = Filter[(y + 1) * 3 + (x + 1)];

					Result += texture2D(InputTex, VsUV + Offset).r * BoxFilterValue;
				}
			}
		}break;


		case 2:
		{
			//NOTE(dima): Box blur 5x5
			
			for(int y = -2; y <= 2; y++)
			{
				for(int x = -2; x <= 2; x++)
				{
					vec2 Offset = vec2(x, y) * TexelSize;

					Result += texture2D(InputTex, VsUV + Offset).r * Filter[0];
				}
			}
		}break;


		case 3:
		{
			//NOTE(dima): Box blur 3x3
			
			for(int y = -1; y <= 1; y++)
			{
				for(int x = -1; x <= 1; x++)
				{
					vec2 Offset = vec2(x, y) * TexelSize;

					Result += texture2D(InputTex, VsUV + Offset).r * Filter[0];
				}
			}
		}break;
	}

	Occlusion = Result;
}