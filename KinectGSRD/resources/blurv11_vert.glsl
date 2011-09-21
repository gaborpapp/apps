#version 120

uniform float width;

varying vec4 tex_coords_pos;
varying vec4 tex_coords_neg;

void main()
{
	float inv_offset1 = 1.3846153846 / width;
	float inv_offset2 = 3.2307692308 / width;

	vec2 uv = gl_MultiTexCoord0.st;
	tex_coords_pos = vec4(uv.x, uv.y, uv.y + inv_offset1, uv.y + inv_offset2);
	tex_coords_neg = vec4(uv.x, uv.y, uv.y - inv_offset1, uv.y - inv_offset2);

	gl_Position = ftransform();
}

