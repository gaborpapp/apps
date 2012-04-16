#version 120

uniform float time;

uniform bool enableTvLines;
uniform bool enableVignetting;
uniform float randFreqMultiplier;

float rand(vec2 co)
{
	return fract( sin( dot( co.xy + vec2( time, time ),
							vec2( 12.9898, 78.233 ) ) ) * 0.437585453 * randFreqMultiplier );
}

void main()
{
	vec2 uv = gl_TexCoord[0].st;

	float d = distance( uv, vec2( .5, .5 ) ) * 1.5;
	float gray = smoothstep( 1., 0., d );
	gray = .7 * gray + .3 * .5 * ( ( 2. * rand( gl_FragCoord.xy ) ) - 1. );
	
	vec4 color = vec4( gray, gray, gray, 1. );

	if ( enableVignetting )
		color *= .5 + .5 * 16. * uv.x * uv.y * ( 1.0 - uv.x ) * ( 1.0 - uv.y );

	if ( enableTvLines )
		color *= .9 + .1 * sin( 10.0 * time + uv.y * 1000.0 );

	gl_FragColor = color;
}

