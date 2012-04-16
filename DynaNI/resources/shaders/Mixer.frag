#version 120

uniform sampler2D tex[9];

uniform float mixOpacity;
uniform float flash;

uniform float randSeed;
uniform float noiseStrength;
uniform float randFreqMultiplier;

uniform bool enableTvLines;
uniform bool enableVignetting;

float rand(vec2 co)
{
	return fract( sin( dot( co.xy + vec2( randSeed, randSeed ),
							vec2( 12.9898, 78.233 ) ) ) * 0.437585453 * randFreqMultiplier );
}

void main()
{
	vec2 uv = gl_TexCoord[0].st;

	vec4 c = texture2D( tex[0], uv );
	float gray = mixOpacity * dot( c.rgb, vec3( .3, .59, .11 ) );
	gray +=  noiseStrength * ( ( 2. * rand( gl_FragCoord.xy ) ) - 1. );
	
	vec4 color = vec4( gray, gray, gray, mixOpacity );

	if ( enableVignetting )
		color *= .5 + .5 * 16. * uv.x * uv.y * ( 1.0 - uv.x ) * ( 1.0 - uv.y );

	if ( enableTvLines )
		color *= .9 + .1 * sin( 10.0 * randSeed + uv.y * 1000.0 );

	color += texture2D( tex[1], uv );
	color += texture2D( tex[2], uv );
	color += texture2D( tex[3], uv );
	color += texture2D( tex[4], uv );
	color += texture2D( tex[5], uv );
	color += texture2D( tex[6], uv );
	color += texture2D( tex[7], uv );
	color += texture2D( tex[8], uv );

	color += flash;

	gl_FragColor = color;
}

