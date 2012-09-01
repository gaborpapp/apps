#version 120
#extension GL_EXT_gpu_shader4 : enable

attribute float index;

uniform sampler2DRect blendshapes;
uniform int numBlendshapes;
uniform float blendshapeWeights[ 46 ];

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

	gl_Position = gl_ModelViewProjectionMatrix * ( gl_Vertex + vec4( blendshape, 0 ) );

	v = vec3( gl_ModelViewMatrix * gl_Vertex );
	N = normalize( gl_NormalMatrix * gl_Normal );

	gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}

