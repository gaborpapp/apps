uniform sampler2D txt;
uniform sampler2D ptxt;
uniform sampler2D dispX;
uniform sampler2D dispY;

uniform float feed;

void main()
{
	vec2 uv = gl_TexCoord[ 0 ].st;
	vec2 puv = gl_TexCoord[ 0 ].st;

	puv.y += texture2D( dispX, vec2( puv.x, .5 ) ).r;
	puv.x += texture2D( dispY, vec2( puv.y, .5 ) ).r;

	gl_FragColor = mix( texture2D( txt, uv ), texture2D( ptxt, puv ), feed );
}

