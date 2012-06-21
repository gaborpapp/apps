#version 120
#extension GL_EXT_gpu_shader4 : enable

void main(void)
{
	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;

	gl_Position = ftransform();
}

