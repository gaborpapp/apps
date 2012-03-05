#version 120

uniform sampler2D dtex[8];
uniform sampler2D ctex[8];

uniform float dbottom;
uniform float dtop;

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 c = vec4(0., 0., 0., 0.);
	float maxd = 1.;
	// unrolling loop is required on osx 10.6.8
	// fatal error C9999: marking a sampler that is not a scalar

	float d = texture2D(dtex[0], uv).r;
	if ((d < maxd) && (d > dbottom) && (d < dtop))
	{
		maxd = d;
		c = texture2D(ctex[0], uv);
	}
	d = texture2D(dtex[1], uv).r;
	if ((d < maxd) && (d > dbottom) && (d < dtop))
	{
		maxd = d;
		c = texture2D(ctex[1], uv);
	}
	d = texture2D(dtex[2], uv).r;
	if ((d < maxd) && (d > dbottom) && (d < dtop))
	{
		maxd = d;
		c = texture2D(ctex[2], uv);
	}
	d = texture2D(dtex[3], uv).r;
	if ((d < maxd) && (d > dbottom) && (d < dtop))
	{
		maxd = d;
		c = texture2D(ctex[3], uv);
	}
	d = texture2D(dtex[4], uv).r;
	if ((d < maxd) && (d > dbottom) && (d < dtop))
	{
		maxd = d;
		c = texture2D(ctex[4], uv);
	}
	d = texture2D(dtex[5], uv).r;
	if ((d < maxd) && (d > dbottom) && (d < dtop))
	{
		maxd = d;
		c = texture2D(ctex[5], uv);
	}
	d = texture2D(dtex[6], uv).r;
	if ((d < maxd) && (d > dbottom) && (d < dtop))
	{
		maxd = d;
		c = texture2D(ctex[6], uv);
	}
	d = texture2D(dtex[7], uv).r;
	if ((d < maxd) && (d > dbottom) && (d < dtop))
	{
		c = texture2D(ctex[7], uv);
	}

	// float gray = dot(c.rgb, vec3(0.299, 0.587, 0.114));
	// gl_FragColor = vec4(gray, gray, gray, 1.);
	gl_FragColor = c;
}

