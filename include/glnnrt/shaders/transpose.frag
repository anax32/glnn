R"(
#version 450

layout(location = 0)	uniform	sampler2D	image;
layout(location = 0)	out	vec4			colour;

void main ()
{
	colour = texelFetch (image, ivec2 (gl_FragCoord.yx), 0);
}
)"