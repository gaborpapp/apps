#version 120

uniform sampler2D dtex[8];
uniform sampler2D ctex[8];

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 c;
	float maxd = 1.;
	// unrolling loop is required on osx 10.6.8
	// fatal error C9999: marking a sampler that is not a scalar

	float d = texture2D(dtex[0], uv).r;
	if (d < maxd)
	{
		maxd = d;
		c = texture2D(ctex[0], uv);
	}
	d = texture2D(dtex[1], uv).r;
	if (d < maxd)
	{
		maxd = d;
		c = texture2D(ctex[1], uv);
	}
	d = texture2D(dtex[2], uv).r;
	if (d < maxd)
	{
		maxd = d;
		c = texture2D(ctex[2], uv);
	}
	d = texture2D(dtex[3], uv).r;
	if (d < maxd)
	{
		maxd = d;
		c = texture2D(ctex[3], uv);
	}
	d = texture2D(dtex[4], uv).r;
	if (d < maxd)
	{
		maxd = d;
		c = texture2D(ctex[4], uv);
	}
	d = texture2D(dtex[5], uv).r;
	if (d < maxd)
	{
		maxd = d;
		c = texture2D(ctex[5], uv);
	}
	d = texture2D(dtex[6], uv).r;
	if (d < maxd)
	{
		maxd = d;
		c = texture2D(ctex[6], uv);
	}
	d = texture2D(dtex[7], uv).r;
	if (d < maxd)
	{
		c = texture2D(ctex[7], uv);
	}

	gl_FragColor = c;
}

