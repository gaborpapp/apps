varying vec3 v;
varying vec3 N;

uniform sampler2D tex;

void main()
{
	vec3 L = normalize( gl_LightSource[ 0 ].position.xyz - v );
	vec3 E = normalize( -v );
	vec3 R = normalize( -reflect( L, N ) );

	// ambient term
	vec4 Iamb = gl_FrontMaterial.ambient;

	// diffuse term
	vec4 Idiff = gl_Color * gl_FrontMaterial.diffuse;
	Idiff *= max( dot( N, L ), 0. );
	Idiff = clamp( Idiff, 0., 1. );

	// specular term
	vec4 Ispec = gl_FrontMaterial.specular;
	Ispec *= pow( max( dot( R, E ), 0. ), gl_FrontMaterial.shininess );
	Ispec = clamp( Ispec, 0., 1. );

	vec4 col = texture2D( tex, gl_TexCoord[ 0 ].st );
	// final color
	gl_FragColor = col * ( Iamb + Idiff + Ispec );
}

