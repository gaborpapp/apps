uniform sampler2D tex;
uniform float threshold;

void main(void)
{
	vec4 c = texture2D(tex, gl_TexCoord[0].st);
	if (c.r >= threshold)
		gl_FragColor = vec4(gl_Color.rgb, 1.);
	else
		discard;
}

