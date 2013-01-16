uniform sampler2D tDepth;

uniform float zNear;
uniform float zFar;

void main()
{
	vec4 depth = texture2D( tDepth, gl_TexCoord[ 0 ].st );
	float linDepth = ( 2. * zNear ) / ( zFar + zNear - depth.x * ( zFar - zNear ) );

	gl_FragColor = vec4( linDepth, linDepth, linDepth, 1. );
}

