varying vec3 V; // eye space position
varying vec3 N; // normal
varying vec3 eyeVec;
 
varying vec4 Q;

uniform float shadowNearClip;
uniform float shadowFarClip;

uniform sampler2D shadowTextureForward;
uniform sampler2D shadowTextureBackward;

const float SHADOW_EPSILON = .001;

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
	// distance between the vertex and the origin is the z-coordinate in
	// the parabolic space
	float z = length( q );

	vec3 Q0 = normalize( q );
	// results in bigger fov in the parabolic texture lookup,
	// which removes the seam at the border of the maps
	const float SEAM_EPSILON = 0.01;
	if ( q.z > 0. )
		Q0.z = Q0.z + 1. + SEAM_EPSILON;
	else
		Q0.z = 1. + SEAM_EPSILON - Q0.z;
	Q0.xy /= Q0.z;
	Q0.xy = .5 * Q0.xy + .5;

	float shadowZ;
	if ( q.z > 0. )
	{
		shadowZ = texture2D( shadowTextureForward, Q0.xy ).b;
	}
	else
	{
		shadowZ = texture2D( shadowTextureBackward, Q0.xy ).b;
	}

	float shadowCoeff = 1.;
	if ( shadowZ + SHADOW_EPSILON < z )
	{
		shadowCoeff = .4;
	}

	gl_FragColor = shadowCoeff * ( ambient + diffuse + specular ) * attenuation;
}
