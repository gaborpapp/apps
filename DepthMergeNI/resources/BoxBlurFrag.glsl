uniform sampler2D tex;
uniform float radius;
uniform vec2 offset_step;

void main()
{
	vec2 st = gl_TexCoord[0].st;
	vec4 color = vec4(0.);

	vec2 offset = offset_step * radius;

	color += 1. * texture2D(tex, st - offset * 4.);
	color += 2. * texture2D(tex, st - offset * 3.);
	color += 3. * texture2D(tex, st - offset * 2.);
	color += 4. * texture2D(tex, st - offset);
	color += 5. * texture2D(tex, st);
	color += 4. * texture2D(tex, st + offset);
	color += 3. * texture2D(tex, st + offset * 2.);
	color += 2. * texture2D(tex, st + offset * 3.);
	color += 1. * texture2D(tex, st + offset * 4.);
	color /= 25.;

	gl_FragColor = color;
}

