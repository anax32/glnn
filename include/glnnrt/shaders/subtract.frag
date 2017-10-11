R"(
#version 450

layout(location = 0)	uniform	sampler2D	A;
layout(location = 1)	uniform	sampler2D	B;
layout(location = 0)	out	vec4			O;

void main ()
{
  ivec2 coord = ivec2 (gl_FragCoord.xy);

	vec4 A_col = texelFetch (A, coord, 0);
	vec4 B_col = texelFetch (B, coord, 0);

	O = A_col - B_col;
}
)"