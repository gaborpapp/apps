
uniform sampler2DRect hmap;
uniform sampler2D envmap;

uniform float xoffset;
uniform float yoffset;

void main()
{
	vec2 uv = gl_TexCoord[0].st;

	vec2 n = vec2(texture2DRect(hmap, uv + vec2(xoffset, 0.0)).r -
			texture2DRect(hmap, uv - vec2(xoffset, 0.0)).r,
			texture2DRect(hmap, uv + vec2(0.0, yoffset)).r -
			texture2DRect(hmap, uv - vec2(0.0, yoffset)).r);
	n = vec2(.5, .5) + .5 * n;

	gl_FragColor = texture2D(envmap, n);
}

