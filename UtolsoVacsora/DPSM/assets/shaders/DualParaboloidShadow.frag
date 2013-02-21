varying vec3 V; // eye space position
varying vec3 N; // normal
varying vec3 eyeVec;
 
varying vec4 Q;

void main()
{
	vec3 L = gl_LightSource[ 0 ].position.xyz - V;
	vec3 l = normalize( L );

	vec3 n = normalize( N );
	vec3 E = normalize( eyeVec );
	vec3 R = reflect( -l, n );

	// attenuation
	float d = length( L );
	float attenuation = 1. / ( gl_LightSource[ 0 ].constantAttenuation +
							   gl_LightSource[ 0 ].linearAttenuation * d +
							   gl_LightSource[ 0 ].quadraticAttenuation * d * d );
	// ambient term
	vec4 ambient = gl_LightSource[ 0 ].ambient *
				   gl_FrontMaterial.ambient;

	// diffuse term
	vec4 diffuse = gl_LightSource[ 0 ].diffuse * 
				   gl_FrontMaterial.diffuse;
	diffuse *= max( dot( n, l ), 0. );
	diffuse = clamp( diffuse, 0., 1. );

	// specular term
	vec4 specular = gl_LightSource[ 0 ].specular *
					gl_FrontMaterial.specular;
	specular *= pow( max( dot( E, R ), 0. ), gl_FrontMaterial.shininess );
	specular = clamp( specular, 0., 1. );

	gl_FragColor = ( ambient + diffuse + specular ) * attenuation;
}
