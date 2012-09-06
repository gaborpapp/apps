#version 120
#extension GL_EXT_gpu_shader4 : enable

attribute float index;

uniform sampler2DRect blendshapes;
uniform int numBlendshapes;
uniform float blendshapeWeights[ 46 ];

uniform float twist;

varying vec3 v;
varying vec3 N;

void main()
{
	vec2 uv = vec2( ( ( int( index ) & 31 ) * numBlendshapes ) + .5,
					( int( index ) / 32 ) + .5 );
	vec3 blendshape = vec3( 0, 0, 0 );
	for ( int i = 0; i < numBlendshapes; i++ )
	{
		blendshape += blendshapeWeights[ i ] * texture2DRect( blendshapes, uv ).xyz;
		uv += vec2( 1, 0 );
	}

	vec4 vertex = gl_Vertex + vec4( blendshape, 0 );
	vec4 worldVertex = gl_ModelViewMatrix * vertex;
	float theta = twist * .01 * worldVertex.y;
	mat3 rotny;
	rotny[ 0 ] = vec3( cos( theta ), 0., sin( theta ) );
	rotny[ 1 ] = vec3( 0., 1., 0. );
	rotny[ 2 ] = vec3( -sin( theta ), 0., cos( theta ) );
	mat4 roty = mat4( rotny );

	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * roty * vertex;

	v = vec3( gl_ModelViewMatrix * roty * vertex );
	N = normalize( gl_NormalMatrix * rotny * gl_Normal );

	gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}

