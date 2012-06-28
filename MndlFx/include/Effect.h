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

// Param interface class
class IParam
{
	public:
		IParam() {}

		IParam( const std::string &name ) :
			mName( name )
		{}

		virtual void addToParams( ci::params::InterfaceGl &params ) = 0;

	protected:
		std::string mName;
};

template< typename T >
class Param : public IParam
{
	public:
		Param() {}

		Param( const std::string &name, T def ) :
			mValue( def ), mDef( def ),
			IParam( name )
		{}

		Param( const std::string &name, T value, T def ) :
			mValue( value ), mDef( def ),
			IParam( name )
		{}

		Param( const Param< T > &rhs ) // normal copy constructor
			: mValue( rhs.mValue ),
			mDef( rhs.mDef ),
			IParam( rhs.mName )
		{}

		Param< T >& operator=( const Param< T > &rhs ) // copy assignment
		{
			if ( this != &rhs )
			{
				IParam::mName = rhs.mName;
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
class ParamNumber : public Param< T >
{
	public:
		ParamNumber()
		{}

		ParamNumber( const std::string &name, T def, T min, T max )
			: mMin( min ), mMax( max ),
			  Param< T >( name, def )
		{}

		ParamNumber( const ParamNumber &rhs ) // normal copy constructor
			: mMin( rhs.mMin ), mMax( rhs.mMax ),
			  Param< T >( rhs.mName, rhs.mValue, rhs.mDef )
		{}

		ParamNumber& operator=( const ParamNumber &rhs ) // copy assignment
		{
			if ( this != &rhs )
			{
				Param< T >::mName = rhs.mName;
				Param< T >::mValue = rhs.mValue;
				Param< T >::mDef = rhs.mDef;
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
			params.addParam( Param< T >::mName, &this->mValue, ss.str() );
		}

	private:
		T mMin;
		T mMax;
};

typedef Param< bool > Paramb;
typedef Param< ci::Color > ParamColor;
typedef Param< ci::ColorA > ParamColorA;
typedef ParamNumber< float > Paramf;
typedef ParamNumber< int > Parami;

class Effect
{
	public:
		size_t getParamCount() const
		{
			return mParams.size();
		}

		const std::string &getName() const
		{
			return mName;
		}

		void addToParams( ci::params::InterfaceGl &params )
		{
			for ( size_t i = 0; i < mParams.size(); ++i )
				mParams[ i ]->addToParams( params );
		}

		virtual ci::gl::Texture &process( const ci::gl::Texture &source ) = 0;

		///! Adds parameter to the effect
		void addParam( IParam *prm )
		{
			mParams.push_back( prm );
		}

	protected:
		Effect() {}

		Effect( const std::string &name ) :
			mName( name )
		{}

		std::string mName;
		std::vector< IParam * > mParams;
};

typedef std::shared_ptr< class Effect > EffectRef;

} }; // namespace mndl::fx

