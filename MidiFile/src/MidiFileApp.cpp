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

#include <boost/format.hpp>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Io.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/Utilities.h"

#include "MidiFileIn.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MidiFileApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled = false;

		typedef std::shared_ptr< stk::MidiFileIn > MidiFileInRef;
		MidiFileInRef mMidiFileRef;
		audio::SourceRef mAudioSource;
		audio::TrackRef mAudioTrack;

		void midiFileThread( unsigned int track );
		bool mMidiFileThreadShouldQuit;
		std::shared_ptr< std::thread > mMidiThreadRef;
		std::mutex mMidiMutex;
		string mMidiNoteString;

		enum {
			MIDI_SYSTEM = 0x0F,
			MIDI_NOTE_OFF = 0x08,
			MIDI_NOTE_ON = 0x09,
			MIDI_CONTROLLER = 0x0b,
			MIDI_PROGRAM_CHANGE = 0x0c,
			MIDI_PITCH_BEND = 0x0e
		};

		void handleMidi( const std::vector< unsigned char > &event );
};

void MidiFileApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void MidiFileApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addParam( "Vertical sync", &mVerticalSyncEnabled );

	try
	{
		mMidiFileRef = MidiFileInRef( new stk::MidiFileIn( getAssetPath( "mennyeihangok.mid" ).string() ) );
	}
	catch ( stk::StkError &exc )
	{
		app::console() << exc.getMessage() << endl;
	}

	if ( mMidiFileRef )
	{
		app::console() << "format " << mMidiFileRef->getFileFormat() << endl;
		app::console() << "tracks " << mMidiFileRef->getNumberOfTracks() << endl;
		app::console() << "seconds/ticks " << mMidiFileRef->getTickSeconds() << endl;
	}

	try
	{
		mAudioSource = audio::load( loadAsset( "mennyeihangok.mp3" ) );
		mAudioTrack = audio::Output::addTrack( mAudioSource, false );

	}
	catch ( const AssetLoadExc &exc )
	{
		app::console() << exc.what() << endl;
	}

	mParams.addButton( "Start midi", [ & ]()
			{
				if ( !mMidiFileRef )
					return;

				if ( mMidiThreadRef )
				{
					mMidiFileThreadShouldQuit = true;
					mMidiThreadRef->join();
					mMidiThreadRef.reset();
				}
				if ( mAudioTrack && mAudioTrack->isPlaying() )
					mAudioTrack->stop();

				mMidiFileThreadShouldQuit = false;
				mMidiThreadRef = std::shared_ptr< std::thread >( new std::thread(
						std::bind( &MidiFileApp::midiFileThread, this, 0 ) ) );
				if ( mAudioTrack )
				{
				    mAudioTrack->setTime( 0. );
				    mAudioTrack->play();
				}
			} );
}

void MidiFileApp::midiFileThread( unsigned int track )
{
	mMidiFileRef->rewindTrack( track );

    vector< unsigned char > event;
    unsigned long ticks = mMidiFileRef->getNextMidiEvent( &event, track );
    while ( !mMidiFileThreadShouldQuit && event.size() )
	{
		// pause for the MIDI event delta time
		ci::sleep( ticks * mMidiFileRef->getTickSeconds() * 1000 );

		handleMidi( event );

		// get a new event
		ticks = mMidiFileRef->getNextMidiEvent( &event, track );
	}
}

void MidiFileApp::handleMidi( const std::vector< unsigned char > &event )
{
	int status = event[ 0 ] >> 4;
	int ch = event[ 0 ] & 0xf;

	switch ( status )
	{
		case MIDI_NOTE_ON:
			if ( event.size() == 3 )
			{
				std::lock_guard< std::mutex > lock( mMidiMutex );
				if ( event[ 2 ] == 0 )
					mMidiNoteString = "";
				else
					mMidiNoteString = ( boost::format( "ch:%d note:%d vel:%d" ) % ch % int( event[ 1 ] ) % int( event[ 2 ] ) ).str();
			}
			break;
		case MIDI_NOTE_OFF:
		{
			std::lock_guard< std::mutex > lock( mMidiMutex );
			mMidiNoteString = "";
		}

		default:
			break;
	}
}

void MidiFileApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );
}

void MidiFileApp::draw()
{
	gl::clear();
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );
	gl::drawStringCentered( mMidiNoteString, getWindowCenter() );

	mParams.draw();
}

void MidiFileApp::shutdown()
{
	if ( mMidiThreadRef )
	{
		mMidiFileThreadShouldQuit = true;
		mMidiThreadRef->join();
	}
}

void MidiFileApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			mParams.show( !mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			break;

		case KeyEvent::KEY_v:
			 mVerticalSyncEnabled = !mVerticalSyncEnabled;
			 break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( MidiFileApp, RendererGl )

