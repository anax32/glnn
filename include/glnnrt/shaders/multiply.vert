R"(
#version 450

uniform	vec3	IJK;
uniform int		offset = 0;

out	ivec2	A_coord;
out ivec2	B_coord;

void main ()
{
	double ijk = /*offset +*/ gl_VertexID;

	highp double ij = ijk / IJK[2];
	float i_ = floor( float (ij) / IJK[1]);
	float j_ = mod	( float (ij) , IJK[1]);
  float k_ = mod	( float (ijk), IJK[2]);

	A_coord = ivec2 (i_, k_);
	B_coord = ivec2 (k_, j_);

	// get the output coordinate
	dvec2 O_coord = dvec2 (i_, j_);

	// convert to normalised viewport coords
	O_coord /= dvec2 (IJK[0], IJK[1]);
	O_coord *= dvec2 (2.0, 2.0);
	O_coord -= dvec2 (1.0, 1.0);

	gl_Position = vec4 (O_coord, 0.0, 1.0);
}
)"