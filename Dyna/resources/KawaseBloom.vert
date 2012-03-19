#version 120

uniform vec2 pixelSize;
uniform int iteration;

varying vec2 topLeft;
varying vec2 topRight;
varying vec2 bottomLeft;
varying vec2 bottomRight;

void main()
{
	gl_FrontColor = gl_Color;
	vec2 st = gl_MultiTexCoord0.st;

	vec2 halfPixelSize = pixelSize / 2.;
	vec2 dst = pixelSize * iteration + halfPixelSize;

	topLeft = vec2( st.s - dst.s, st.t + dst.t );
	topRight = vec2( st.s + dst.s, st.t + dst.t );
	bottomLeft = vec2( st.s - dst.s, st.t - dst.t );
	bottomRight = vec2( st.s + dst.s, st.t - dst.t );

	gl_Position = ftransform();
}

