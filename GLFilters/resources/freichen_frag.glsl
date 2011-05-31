/*
based on the Frei-Chen edge detector by Daniel Rakos
http://rastergrid.com/blog/2011/01/frei-chen-edge-detector/
*/

#version 120

uniform sampler2D tex;

uniform vec2 offset[9];
uniform vec3 G[27];

void main(void)
{
	mat3 I;
	float cnv[9];
	vec3 sample;
	
	vec2 uv = gl_TexCoord[0].st;

	/* fetch the 3x3 neighbourhood and use the RGB vector's length as intensity value */
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			sample = texture2D(tex, uv + offset[i * 3 + j]).rgb;
			I[i][j] = length(sample); 
		}
	}
	
	/* calculate the convolution values for all the masks */
	for (int i = 0; i < 9; i++)
	{
		float dp3 = dot(G[i * 3], I[0]) + dot(G[i * 3 + 1], I[1]) + dot(G[i * 3 + 2], I[2]);
		cnv[i] = dp3 * dp3; 
	}

	float M = (cnv[0] + cnv[1]) + (cnv[2] + cnv[3]);
	float S = (cnv[4] + cnv[5]) + (cnv[6] + cnv[7]) + (cnv[8] + M); 
	
	gl_FragColor = vec4(sqrt(M / S));
}

