#version 120

uniform vec3 effectCenter;
uniform float effectRadius;
uniform float effectMaxRadius;

varying vec3 V;

void main()
{
	gl_FrontColor = gl_Color;
	gl_TexCoord[ 0 ] = gl_MultiTexCoord0;

	vec3 pos = gl_Vertex.xyz / gl_Vertex.w;

	vec3 d = pos - effectCenter;
	float l = length( d );
	l = clamp( l, effectRadius, effectMaxRadius );
	d = normalize( d ) * l;
	float u = ( l - effectRadius ) / ( effectMaxRadius - effectRadius );
	vec3 newPos = effectCenter + d * sin( 3.1415926 * u );
	newPos = mix( newPos, pos, u );

	// transform vertex into eyespace
	V = ( gl_ModelViewMatrix * vec4( newPos, 1 ) ).xyz;

/*
	vec3 d = pos - effectCenter;
	float l = length( d );
	vec3 newPos = effectCenter + ( d * ( effectRadius * effectRadius ) / ( l * l ) );
	float u = clamp( l, 0, effectMaxRadius ) / effectMaxRadius;
	newPos = mix( newPos, pos, u );
	*/
	gl_Position = gl_ModelViewProjectionMatrix * vec4( newPos, 1. );
}
