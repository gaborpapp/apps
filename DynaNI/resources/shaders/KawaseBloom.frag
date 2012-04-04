#version 120

uniform sampler2D tex;

varying vec2 topLeft;
varying vec2 topRight;
varying vec2 bottomLeft;
varying vec2 bottomRight;

void main()
{
	vec4 accum;

	accum = texture2D( tex, topLeft );
	accum += texture2D( tex, topRight );
	accum += texture2D( tex, bottomLeft );
	accum += texture2D( tex, bottomRight );

	accum *= 0.25;

	gl_FragColor = accum;
}

