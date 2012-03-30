#version 120

uniform float kernelSize;
uniform float invKernelSize;
uniform sampler2D imageTex;
uniform sampler2D kernelTex;
uniform vec2 stepVector;
uniform float blurAmount;

void main()
{
	vec4 c = vec4( .0 );
	for ( int i = 0; i < kernelSize; i++ )
	{
		vec2 kernel = texture2D( kernelTex, vec2( i * invKernelSize, 0.5 ) ).rg;
		vec2 offset = stepVector * ( kernel.r - 0.5 ) * blurAmount;
		
		vec4 sample = texture2D( imageTex, gl_TexCoord[0].st + offset );
		c += sample * kernel.g * 0.15;
	}
	gl_FragColor = c;
}

