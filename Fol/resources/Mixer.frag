#version 120

uniform sampler2D tex[9];

void main()
{
    vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D( tex[0], uv );

    color += texture2D( tex[1], uv );
    color += texture2D( tex[2], uv );
    color += texture2D( tex[3], uv );
    color += texture2D( tex[4], uv );
    color += texture2D( tex[5], uv );
    color += texture2D( tex[6], uv );
    color += texture2D( tex[7], uv );
    color += texture2D( tex[8], uv );

    gl_FragColor = color;
}

