/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <vector>
#include <string>
#include <sstream>

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/params/Params.h"

namespace mndl { namespace fx {

#define STRINGIFY(x) #x

// Parameter interface class
class IParameter
{
	public:
		IParameter() {}

		IParameter( const std::string &name ) :
			mName( name )
		{}

		virtual void addToParams( ci::params::InterfaceGl &params ) = 0;

	protected:
		std::string mName;
};

template< typename T >
class Parameter : public IParameter
{
	public:
		Parameter() {}

		Parameter( const std::string &name, T def ) :
			mValue( def ), mDef( def ),
			IParameter( name )
		{}

		Parameter( const std::string &name, T value, T def ) :
			mValue( value ), mDef( def ),
			IParameter( name )
		{}

		Parameter( const Parameter< T > &rhs ) // normal copy constructor
			: mValue( rhs.mValue ),
			mDef( rhs.mDef ),
			IParameter( rhs.mName )
		{}

		Parameter< T >& operator=( const Parameter< T > &rhs ) // copy assignment
		{
			if ( this != &rhs )
			{
				IParameter::mName = rhs.mName;
				mValue = rhs.mValue;
				mDef = rhs.mDef;
			}
			return *this;
		}

		operator const T&() const { return mValue; }

		virtual void addToParams( ci::params::InterfaceGl &params )
		{
			params.addParam( mName, &mValue );
		}

	protected:
		T mValue;
		T mDef;
};

template< typename T >
class ParameterNumber : public Parameter< T >
{
	public:
		ParameterNumber()
		{}

		ParameterNumber( const std::string &name, T def, T min, T max )
			: mMin( min ), mMax( max ),
			  Parameter< T >( name, def )
		{}

		ParameterNumber( const ParameterNumber &rhs ) // normal copy constructor
			: mMin( rhs.mMin ), mMax( rhs.mMax ),
			  Parameter< T >( rhs.mName, rhs.mValue, rhs.mDef )
		{}

		ParameterNumber& operator=( const ParameterNumber &rhs ) // copy assignment
		{
			if ( this != &rhs )
			{
				Parameter< T >::mName = rhs.mName;
				Parameter< T >::mValue = rhs.mValue;
				Parameter< T >::mDef = rhs.mDef;
				mMin = rhs.mMin;
				mMax = rhs.mMax;
			}
			return *this;
		}

		void addToParams( ci::params::InterfaceGl &params )
		{
			std::stringstream ss;
			T step = ( mMax - mMin ) / 256.;
			if ( step == 0 )
				step = 1;
			ss << "min=" << mMin << " max=" << mMax << " step=" << step;
			params.addParam( Parameter< T >::mName, &this->mValue, ss.str() );
		}

	private:
		T mMin;
		T mMax;
};

typedef Parameter< bool > ParameterBool;
typedef Parameter< ci::Color > ParameterColor;
typedef Parameter< ci::ColorA > ParameterColorA;
typedef ParameterNumber< float > ParameterFloat;
typedef ParameterNumber< int > ParameterInt;

class Effect
{
	public:
		size_t getParameterCount() const
		{
			return mParams->mParameters.size();
		}

		const std::string &getName() const
		{
			return mName;
		}

		void addToParams( ci::params::InterfaceGl &params )
		{
			for ( size_t i = 0; i < mParams->mParameters.size(); ++i )
				mParams->mParameters[ i ]->addToParams( params );
		}

		virtual ci::gl::Texture &process( const ci::gl::Texture &source ) = 0;

	protected:
		Effect() {}

		Effect( const std::string &name ) :
			mName( name ),
			mParams( new Params )
		{}

		struct Params
		{
			void addParameter( IParameter *prm )
			{
				mParameters.push_back( prm );
			}

			std::vector< IParameter * > mParameters;
		};
		std::shared_ptr< Params > mParams;

		std::string mName;
};

typedef std::shared_ptr< class Effect > EffectRef;

} }; // namespace mndl::fx

