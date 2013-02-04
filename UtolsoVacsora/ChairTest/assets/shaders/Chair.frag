#version 120

varying vec3 V;

void main()
{
	const float shininess = 500.0;

	vec4 ambient = gl_LightSource[0].ambient;
	vec4 diffuse = gl_LightSource[0].diffuse;
	vec4 specular = gl_LightSource[0].specular;

	// calculate flat face normal on-the-fly
	vec3 n = normalize( cross( dFdx( V ), dFdy( V ) ) );

	vec3 L = normalize(gl_LightSource[0].position.xyz - V);
	vec3 E = normalize(-V);
	vec3 R = normalize(-reflect(L,n));

	// ambient term 
	vec4 Iamb = ambient;

	// diffuse term
	vec4 Idiff = gl_Color * diffuse;
	Idiff *= max(dot(n,L), 0.0);
	Idiff = clamp(Idiff, 0.0, 1.0);

	// specular term
	vec4 Ispec = specular;
	Ispec *= pow(max(dot(R,E),0.0), shininess);
	Ispec = clamp(Ispec, 0.0, 1.0);

	gl_FragColor = Iamb + Idiff + Ispec;
}

