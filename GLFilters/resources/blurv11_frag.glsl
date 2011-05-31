#version 120

uniform sampler2D txt;

varying vec4 tex_coords_pos;
varying vec4 tex_coords_neg;

void main()
{
	float weight[3];
	weight[0] = 0.2270270270;
	weight[1] = 0.3162162162;
	weight[2] = 0.0702702703;

	vec4 tc = texture2D(txt, tex_coords_pos.xy) * weight[0];
	tc += texture2D(txt, tex_coords_pos.xz) * weight[1];
	tc += texture2D(txt, tex_coords_pos.xw) * weight[2];

	tc += texture2D(txt, tex_coords_neg.xz) * weight[1];
	tc += texture2D(txt, tex_coords_neg.xw) * weight[2];

	gl_FragColor = vec4(tc);
}

