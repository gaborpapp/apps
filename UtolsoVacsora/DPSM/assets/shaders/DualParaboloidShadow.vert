uniform mat4 shadowMatrix;

varying vec3 V; // eye space position
varying vec3 N; // normal
varying vec3 eyeVec;

varying vec4 Q;

void main()
{
	// transform vertex into eye space
	V = vec3( gl_ModelViewMatrix * gl_Vertex );
	// transform normal into eye space
	N = gl_NormalMatrix * gl_Normal;

	eyeVec = -V;

	// shadow space vertex
	Q = shadowMatrix * gl_ModelViewMatrix * gl_Vertex;

	// pstandard pass-through transformations
	gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}

