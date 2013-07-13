/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

uniform sampler2D txt;

uniform int numReflectionLines;
varying vec3 lines[ 32 ];

//#define DEBUG 1

void main()
{
	vec2 uv = gl_TexCoord[ 0 ].st;
	vec2 p = uv;

	for ( int i = 0; i < numReflectionLines; i++ )
	{
		float d = dot( vec3( p, 1. ), lines[ i ] ); // distance from line

	#ifdef DEBUG
		if ( abs( d ) < 0.001 )
		{
			gl_FragColor  = vec4( 0, 1, 0, 1 );
			return;
		}
		else
	#endif
		if ( d < 0. )
		{
			p -= 2. * d * lines[ i ].xy; // mirror
		}
	}
	gl_FragColor = texture2D( txt, fract( p ) );
}

