#version 120

uniform sampler2DRect tex; // u = r, v = b
uniform float dU;
uniform float dV;
uniform float f;
uniform float k;

void main(void)
{
	vec2 tidx = gl_TexCoord[0].st;

	vec2 txt_clr = texture2DRect(tex, tidx).rb;

	vec2 top_idx = tidx + vec2(0., -1);
	vec2 bottom_idx = tidx + vec2(0, 1);
	vec2 left_idx = tidx + vec2(-1, 0);
	vec2 right_idx = tidx + vec2(1, 0);
	
	float currF = f;
	float currK = k;
	float currU = txt_clr.r;
	float currV = txt_clr.g;
	float d2 = currU * currV * currV;

	vec2 uv = clamp(txt_clr +
			vec2(dU, dV) *
				 (texture2DRect(tex, right_idx).rb +
				  texture2DRect(tex, left_idx).rb +
				  texture2DRect(tex, bottom_idx).rb +
				  texture2DRect(tex, top_idx).rb -
				  4. * txt_clr) + 
			vec2(-d2 + currF * (1. - currU),
			      d2 - currK * currV), 0., 1.);

	gl_FragColor = vec4(uv.x, 0.0, uv.y, 1.0);
}

