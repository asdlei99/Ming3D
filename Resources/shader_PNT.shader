ShaderUniforms
{
	mat4 MVP;
	vec4 test;
#ifdef use_mat_colour
    vec4 colour;
#endif
}

ShaderTextures
{
	Texture2D inTexture;
}

// Vertex shader input
struct VSInput
{
	vec4 Position : POSITION;
	vec3 Normal : NORMAL;
	vec2 TexCoord : TEXCOORD;
}

// Fragment shader input
struct FSInput
{
	vec4 Position : SV_POSITION;
	vec3 Normal : NORMAL;
	vec2 TexCoord : TEXCOORD;
}

// Vertex shader
shader VertexShader
{
	void main(VSInput input, FSInput output)
	{
		output.Position = MVP * input.Position;
		output.Normal = input.Normal;
		output.TexCoord = input.TexCoord;
	}
}

// Fragment shader
shader FragmentShader
{
	void main(FSInput input)
	{
    #ifdef use_mat_colour
        SetFragmentColour(colour);
    #else
		SetFragmentColour(ReadTexture(inTexture, input.TexCoord));
    #endif
	}
}

