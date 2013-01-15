#version 110

uniform sampler2DShadow	shadowTexture; // shadow depth texture
uniform sampler2D diffuseTexture; // diffuse color map
uniform float shadowStrength;

varying vec3 V;
varying vec3 N;
varying vec4 Q;

void main()
{	
	const vec4 gamma = vec4( 1.0 / 2.2 ); 
	const float shininess = 10.0;

	vec4 ambient = gl_LightSource[0].ambient;
	vec4 diffuse = gl_LightSource[0].diffuse;
	vec4 specular = gl_LightSource[0].specular;

	vec3 L = normalize(gl_LightSource[0].position.xyz - V);
	vec3 E = normalize(-V);
	vec3 R = normalize(-reflect(L, N));
	
	// shadow term
	float shadow = 1.0 - shadowStrength + shadowStrength * shadow2D( shadowTexture, 0.5 * (Q.xyz / Q.w + 1.0) ).r;

	// ambient term 
	vec4 Iamb = ambient;

	// diffuse term
	vec4 Idiff = texture2D( diffuseTexture, gl_TexCoord[0].st) * diffuse;
	Idiff *= max(dot(N,L), 0.0);
	Idiff = clamp(Idiff, 0.0, 1.0) * shadow;

	// specular term
	vec4 Ispec = specular;
	Ispec *= pow(max(dot(R,E),0.0), shininess);
	Ispec = clamp(Ispec, 0.0, 1.0) * shadow;

	// final color 
	gl_FragColor = pow(Iamb + Idiff + Ispec, gamma);
}
