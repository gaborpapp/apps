uniform sampler2DRect tex;

void main(void)
{
	float c = texture2DRect(tex, gl_TexCoord[0].st).r;
	gl_FragColor = vec4(c, c, c, 1.0);
}

