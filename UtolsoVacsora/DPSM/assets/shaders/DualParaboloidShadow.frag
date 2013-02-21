varying vec3 V; // eye space position
varying vec3 N; // normal
varying vec3 eyeVec;
 
varying vec4 Q;

uniform float shadowNearClip;
uniform float shadowFarClip;

uniform sampler2DShadow shadowTextureForward;
uniform sampler2DShadow shadowTextureBackward;

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

	// specular term
	vec4 specular = gl_LightSource[ 0 ].specular *
					gl_FrontMaterial.specular;
	specular *= pow( max( dot( E, R ), 0. ), gl_FrontMaterial.shininess );

	// dual-parabolic shadow mapping
	vec3 q = Q.xyz / Q.w;
	float ds = length( q ); // distance between the vertex and the origin

	float alpha = .5 + q.z * attenuation; // ???

	vec3 Q0 = q / ds;
	Q0.z = Q0.z + 1.;
	Q0.x = Q0.x / Q0.z;
	Q0.y = Q0.y / Q0.z;
	Q0.z = ds;// * attenuation;

	Q0.x = .5 * Q0.x + .5;
	Q0.y = .5 * Q0.y + .5;

	vec3 Q1 = q / ds;
	Q1.z = 1. - Q1.z;
	Q1.x = Q1.x / Q1.z;
	Q1.y = Q1.y / Q1.z;
	Q1.z = ds;// * attenuation;

	Q1.x = .5 * Q1.x + .5;
	Q1.y = .5 * Q1.y + .5;

	float shadow;
	if ( alpha >= .5 )
	//if ( q.z > 0. )
	{
		Q0.z = ( Q0.z - shadowNearClip ) / ( shadowFarClip - shadowNearClip ); // scale depth to [0, 1] for display
		shadow = shadow2D( shadowTextureForward, Q0.xyz ).r;
	}
	else
	{
		Q1.z = ( Q1.z - shadowNearClip ) / ( shadowFarClip - shadowNearClip ); // scale depth to [0, 1] for display
		shadow = shadow2D( shadowTextureBackward, Q1.xyz ).r;
	}

	shadow = shadow * .3;

	gl_FragColor = shadow * ( ambient + diffuse + specular ) * attenuation;
}
