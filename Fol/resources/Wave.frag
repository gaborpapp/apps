varying float depth;

uniform float clip;

void main()
{
	if ( depth > clip )
		discard;

	gl_FragColor = gl_Color;
}

