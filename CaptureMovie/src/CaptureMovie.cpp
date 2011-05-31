#include <ctime>
#include <sstream>
#include <iomanip>

#include "cinder/Utilities.h"
#include "cinder/ip/Flip.h"
#include "CaptureMovie.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void CaptureMovie::setup()
{
	try
	{
		mCapture = Capture(640, 480);
		mCapture.start();
	}
	catch (...)
	{
		console() << "Failed to initialize capture" << endl;
	}

	qtime::MovieWriter::Format format;
	format.setCodec(qtime::MovieWriter::CODEC_H264);
	format.setQuality(0.5f);

	mMovieWriter = qtime::MovieWriter(getDocumentsDirectory() + "test-" +
			timeStamp() + ".mov", 640, 480, format);
}

string CaptureMovie::timeStamp()
{
	struct tm tm;
	time_t ltime;
	static int last_sec = 0;
	static int index = 0;

	time(&ltime);
	localtime_r(&ltime, &tm);
	if (last_sec != tm.tm_sec)
		index = 0;

	stringstream ss;
	ss << setfill('0') << setw(2) << tm.tm_year - 100 <<
		setw(2) << tm.tm_mon + 1 << setw(2) << tm.tm_mday <<
		setw(2) << tm.tm_hour << setw(2) << tm.tm_min <<
		setw(2) << tm.tm_sec << setw(2) << index;

	index++;
	last_sec = tm.tm_sec;

	return ss.str();
}

void CaptureMovie::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	else if (event.getChar() == ' ')
		(mCapture && mCapture.isCapturing()) ? mCapture.stop() : mCapture.start();
}

void CaptureMovie::update()
{
	if (mCapture && mCapture.checkNewFrame())
	{
		Surface8u s = mCapture.getSurface();
		ip::flipVertical(&s);
		mTexture = gl::Texture(s);
		mMovieWriter.addFrame(s);
	}
}

void CaptureMovie::draw()
{
	gl::clear(Color(0.0f, 0.0f, 0.0f));
	gl::setMatricesWindow(getWindowWidth(), getWindowHeight());

	if (mTexture)
	{
		glPushMatrix();
		gl::draw( mTexture );
		glPopMatrix();
	}
}

CINDER_APP_BASIC(CaptureMovie, RendererGl)

