R"(
#version 450

layout(location = 0)	uniform	sampler2D	A;
layout(location = 1)	uniform	sampler2D	B;
layout(location = 0)	out	vec4			O;

flat in ivec2 A_coord;
flat in ivec2 B_coord;

void main ()
{
	vec4 A_col = texelFetch (A, A_coord, 0);
	vec4 B_col = texelFetch (B, B_coord, 0);

	// multiply the matrix entries and pass on
	// to the blend stage for summation
	O = A_col * B_col;
}
)"