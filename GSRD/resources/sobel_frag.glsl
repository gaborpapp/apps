/*
based on the Sobel edge detector by Daniel Rakos
http://rastergrid.com/blog/2011/01/frei-chen-edge-detector/
*/

uniform sampler2D tex;

uniform vec2 offset[9];
uniform mat3 G[2];

void main(void)
{
	mat3 I;
	float cnv[2];
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
	for (int i = 0; i < 2; i++)
	{
		float dp3 = dot(G[i][0], I[0]) + dot(G[i][1], I[1]) + dot(G[i][2], I[2]);
		cnv[i] = dp3 * dp3; 
	}

	gl_FragColor = vec4(0.5 * sqrt(cnv[0]*cnv[0]+cnv[1]*cnv[1]));
}

