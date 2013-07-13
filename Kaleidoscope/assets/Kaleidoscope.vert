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

uniform vec2 center;
uniform int numReflectionLines;
uniform float rotation;

varying vec3 lines[ 32 ];

#define M_PI 3.141592653589793

void main()
{
	float addA = M_PI / float( numReflectionLines );
	float a = rotation;

	for ( int i = 0; i < numReflectionLines; i++ )
	{
		vec2 v = vec2( cos( a ), sin( a ) );
		vec2 p = center + 0.3 * v;
		vec2 n = vec2( -v.y, v.x );

		lines[ i ] = vec3( n, -dot( p, n ) ); // normalized line equation
		a += addA;
	}

	gl_FrontColor = gl_Color;
	gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}

