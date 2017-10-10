R"(
#version 450

void main ()
{
	switch (gl_VertexID)
	{
		case 0:
		{	// top left
			gl_Position = vec4 (-1.0, -1.0, 0.0, 1.0);
			break;
		}
		case 1:
		{	// bottom left
			gl_Position = vec4 (-1.0,  1.0, 0.0, 1.0);
			break;
		}
		case 2:
		{
			gl_Position = vec4 ( 1.0, -1.0, 0.0, 1.0);
			break;
		}
		case 3:
		{	// bottom right
			gl_Position = vec4 ( 1.0,  1.0, 0.0, 1.0);	
			break;
		}
	}
}
)"