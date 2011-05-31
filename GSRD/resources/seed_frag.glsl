uniform sampler2D tex;

void main(void)
{
	vec4 c = texture2D(tex, gl_TexCoord[0].st);
	float br = c.a * dot(vec3(.3, .59, .11), c.rgb);
	if (br > .005)
		gl_FragColor = vec4(gl_Color.rgb, 1.);
		//gl_FragColor = gl_Color * br;
	else
		discard;
}

