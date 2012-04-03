#version 120

uniform sampler2D tex[9];

uniform float mixOpacity;
uniform float flash;

void main()
{
	vec4 c = texture2D( tex[0], gl_TexCoord[0].st );
	float gray = mixOpacity * dot( c.rgb, vec3( .3, .59, .11 ) );
	
	// FIXME: opacity
	vec4 color = vec4( gray, gray, gray, mixOpacity );

	color += texture2D( tex[1], gl_TexCoord[0].st );
	color += texture2D( tex[2], gl_TexCoord[0].st );
	color += texture2D( tex[3], gl_TexCoord[0].st );
	color += texture2D( tex[4], gl_TexCoord[0].st );
	color += texture2D( tex[5], gl_TexCoord[0].st );
	color += texture2D( tex[6], gl_TexCoord[0].st );
	color += texture2D( tex[7], gl_TexCoord[0].st );
	color += texture2D( tex[8], gl_TexCoord[0].st );

	/*
	for ( int i = 1; i < 9; i ++)
	{
		color += mixOpacity * texture2D( tex[i], gl_TexCoord[0].st );
	}
	*/

	color += flash;

	gl_FragColor = color;
}

