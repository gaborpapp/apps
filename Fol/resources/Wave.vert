uniform sampler2D txt;
uniform vec2 invSize;

varying float depth;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;

	depth = texture2D( txt, gl_TexCoord[0].st ).r;

	vec4 grad = texture2D( txt, gl_TexCoord[0].st - invSize ) -
				texture2D( txt, gl_TexCoord[0].st + invSize );

	grad.z = .01;
	grad.w = 1.;
	grad = 10. * normalize( grad );

	vec4 vertex = gl_Vertex + vec4( grad.xy, 0, 0 );
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	gl_FrontColor = gl_Color;
}

