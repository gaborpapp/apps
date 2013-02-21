varying float z; // z * hemisphere direction
varying float depth;

void main()
{
	if ( z > 0. ) // z coordinate and hemisphere direction have the same sign
		gl_FragColor = vec4( vec3( 1. - depth ), 1. );
	else // coordinate is on the other hemisphere, discard it
		discard;
}
