// based on the shader by
// http://artmartinsh.blogspot.com/2010/02/glsl-lens-blur-filter-with-bokeh.html

uniform sampler2D rgb;
uniform sampler2D depth;
 
const float blurclamp = 3.0; // max blur amount
uniform float aperture; // bigger values for shallower depth of field
uniform float focus;
uniform vec2 isize;
uniform float amount;

void main()
{
	vec4 depth1 = texture2D( depth, gl_TexCoord[0].xy );

	float factor = ( depth1.x - focus );

	vec2 dofblur = vec2( clamp( factor * aperture, -blurclamp, blurclamp ) ) * isize * amount;

	vec4 col = texture2D( rgb, gl_TexCoord[0].xy );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.0, 0.4 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.15, 0.37 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.29, 0.29 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.37, 0.15 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.4, 0.0 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.37, -0.15 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.29, -0.29 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.15, -0.37 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.0, -0.4 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.15, 0.37 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.29, 0.29 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.37, 0.15 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.4, 0.0 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.37, -0.15 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.29, -0.29 ) * dofblur );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.15, -0.37 ) * dofblur );
 
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.15, 0.37 ) * dofblur * 0.9 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.37, 0.15 ) * dofblur * 0.9 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.37, -0.15 ) * dofblur * 0.9 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.15, -0.37 ) * dofblur * 0.9 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.15, 0.37 ) * dofblur * 0.9 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.37, 0.15 ) * dofblur * 0.9 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.37, -0.15 ) * dofblur * 0.9 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.15, -0.37 ) * dofblur * 0.9 );

	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.29, 0.29 ) * dofblur * 0.7 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.4, 0.0 ) * dofblur * 0.7 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.29, -0.29 ) * dofblur * 0.7 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.0, -0.4 ) * dofblur * 0.7 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.29, 0.29 ) * dofblur * 0.7 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.4, 0.0 ) * dofblur * 0.7 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.29, -0.29 ) * dofblur * 0.7 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.0, 0.4 ) * dofblur * 0.7 );

	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.29, 0.29 ) * dofblur * 0.4 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.4, 0.0 ) * dofblur * 0.4 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.29, -0.29 ) * dofblur * 0.4 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.0, -0.4 ) * dofblur * 0.4 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.29, 0.29 ) * dofblur * 0.4 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.4, 0.0 ) * dofblur * 0.4 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( -0.29, -0.29 ) * dofblur * 0.4 );
	col += texture2D( rgb, gl_TexCoord[0].xy + vec2( 0.0, 0.4 ) * dofblur * 0.4 );

	gl_FragColor = col / 41.0;
	gl_FragColor.a = 1.0;
}

