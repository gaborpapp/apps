uniform sampler2D txt;
uniform sampler2D ptxt;

uniform float dispXAmplitude;
uniform float dispXOffset;
uniform float dispXAddPerPixel;

uniform float dispYAmplitude;
uniform float dispYOffset;
uniform float dispYAddPerPixel;

uniform float feed;
uniform float time;
uniform float noiseSpeed;
uniform float noiseScale;
uniform float noiseDisp;
uniform float noiseTwirl;

#define M_PI 3.14159

// noise & fbm by iq (https://www.shadertoy.com/view/XdfGRn)
mat3 m = mat3( 0.00,  0.80,  0.60,
              -0.80,  0.36, -0.48,
              -0.60, -0.48,  0.64 );

float hash( float n )
{
    return fract(sin(n)*43758.5453);
}

float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f*f*(3.0-2.0*f);

    float n = p.x + p.y*57.0 + 113.0*p.z;

    float res = mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                        mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
                    mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                        mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
    return res;
}

float fbm( vec3 p )
{
    float f = 0.0;

    f += 0.5000*noise( p ); p = m*p*2.02;
    f += 0.2500*noise( p ); p = m*p*2.03;
    f += 0.1250*noise( p ); p = m*p*2.01;
    f += 0.0625*noise( p );

    return f/0.9375;
}

void main()
{
	vec2 uv = gl_TexCoord[ 0 ].st;
	vec2 puv = gl_TexCoord[ 0 ].st;

	puv.y += dispYAmplitude * sin( dispYAddPerPixel * puv.x + dispYOffset );
	puv.x += dispXAmplitude * sin( dispXAddPerPixel * puv.y + dispXOffset );

	float n = noiseTwirl * M_PI * fbm( noiseScale * vec3( puv, noiseSpeed * time ) );
	puv += noiseDisp * vec2( cos( n ), sin( n ) );

	gl_FragColor = mix( texture2D( txt, uv ), texture2D( ptxt, puv ), feed );
}

