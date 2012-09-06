#version 120

uniform float twist;

varying vec3 v;
varying vec3 N;

void main()
{
	vec4 worldVertex = gl_ModelViewMatrix * gl_Vertex;

	float theta = twist * .01 * worldVertex.y;
	mat3 rotny;
	rotny[ 0 ] = vec3( cos( theta ), 0., sin( theta ) );
	rotny[ 1 ] = vec3( 0., 1., 0. );
	rotny[ 2 ] = vec3( -sin( theta ), 0., cos( theta ) );
	mat4 roty = mat4( rotny );

	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * roty * gl_Vertex;

	v = vec3( gl_ModelViewMatrix * roty * gl_Vertex );
	N = normalize( gl_NormalMatrix * rotny * gl_Normal );

	gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}

