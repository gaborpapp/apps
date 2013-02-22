uniform float hemisphereDirection; // 1 for forward, -1 for backward hemisphere
uniform float nearClip;
uniform float farClip;

varying float z;
varying float depth;

void main()
{
	vec4 position = gl_ModelViewMatrix * gl_Vertex;
	position /= position.w; // normalize by w
	position.z *= hemisphereDirection; // set z-values to forward or backward

	float d = length( position.xyz ); // determine the distance between the vertex and the origin
	position /= d; // divide the vertex position by the distance

	z = position.z;

	position.z += 1.; // add the reflected vector to find the normal vector ???

	position.x /= position.z;
	position.y /= position.z;

	position.z = d / 1000.;
	//position.z = ( d - nearClip ) / ( farClip - nearClip ); // scale depth
	position.w = 1.;
	depth = d;

	gl_Position = position;
	gl_FrontColor = gl_Color;
}
