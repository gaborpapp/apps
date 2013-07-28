#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>

#include "cinder/PolyLine.h"
#include "cinder/Utilities.h"
#include "cinder/Xml.h"
#include "cinder/Filesystem.h"

#include "AntTweakBar.h"

#include "KinectTracker.h"

#include "CinderOpenCv.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void KinectTracker::setup()
{
	int kinect_count = Kinect::getNumDevices();
	console() << "There are " << kinect_count << " Kinects connected." << std::endl;

	if (kinect_count)
		kinect = Kinect::create(Kinect::Device());

	threshold = 170;
	blur_size = 10;
	min_area = 0.04;
	max_area = 0.2;
	vflip = true;
	tilt = 0;

	debug_tracker = false;

	params = params::InterfaceGl("KinectTracker", Vec2i(200, 300));
	params.addText("status", kinect_count ? "label=`Kinect initialized`" : "label=`Kinect not found`");
	params.addSeparator();
	params.addParam("Tilt", &tilt, "min=-31.0 max=31.0 step=1.0 keyDecr={ keyIncr=}");
	params.addParam("Threshold", &threshold, "min=0.0 max=255.0 step=1.0 keyDecr=[ keyIncr=]");
	params.addParam("Blur size", &blur_size, "min=1 max=15 step=1 keyDecr=, keyIncr=.");
	params.addParam("Min area", &min_area, "min=0.0 max=1.0 step=0.005 keyDecr=q keyIncr=w");
	params.addParam("Max area", &max_area, "min=0.0 max=1.0 step=0.005 keyDecr=a keyIncr=s");
	params.addParam("VFlip", &vflip, "");

	params.addSeparator();
	params.addButton("Play video", std::bind(&KinectTracker::playVideoCB, this));
	params.addButton("Save video", std::bind(&KinectTracker::saveVideoCB, this));

	params.addSeparator();
	params.addParam("Debug", &debug_tracker, "key=SPACE");

	params.addSeparator();
	params.addButton("Load config", std::bind(&KinectTracker::load_config, this));
	params.addButton("Save config", std::bind(&KinectTracker::save_config, this));

	playing_video = false;
	kinect_available = kinect_count;
	if (!kinect_available)
		params.setOptions("Save video", "readonly=true");
	saving_video = false;

	load_config();
}

void KinectTracker::save_config()
{
	fs::path data_path = getAppPath();
#ifdef CINDER_MAC
	data_path /= "/Contents/Resources/";
#endif
	data_path /= "tracker.xml";
	XmlTree config = XmlTree::createDoc();

	// TODO: automate this
	XmlTree t("param", "");
	string parameters[] = {"threshold", "blur_size", "min_area", "max_area", "tilt"};
	float values[] = {threshold, blur_size, min_area, max_area, tilt};

	for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++)
	{
		t = XmlTree("param", "");
		t.setAttribute("name", parameters[i]);
		t.setAttribute("value", values[i]);
		config.push_back(t);
	}
	config.write(DataTargetPath::createRef(data_path));
}

void KinectTracker::load_config()
{
	fs::path data_path = getAppPath();
#ifdef CINDER_MAC
	data_path /= "/Contents/Resources/";
#endif
	data_path /= "tracker.xml";

	fs::path test_path(data_path);
	if (!fs::exists(test_path))
	{
		save_config();
		return;
	}

	XmlTree doc(loadFile(data_path));

	for (XmlTree::Iter item = doc.begin(); item != doc.end(); ++item)
	{
		string name = item->getAttribute("name").getValue();
		float v = item->getAttribute("value").getValue<float>();

		// TODO: automate this
		string parameters[] = {"threshold", "blur_size", "min_area", "max_area", "tilt"};
		float *valptrs[] = {&threshold, &blur_size, &min_area, &max_area, &tilt};

		for (int i = 0; i < sizeof(valptrs) / sizeof(valptrs[0]); i++)
		{
			if (name == parameters[i])
			{
				*(valptrs[i]) = v;
			}
		}
	}
}

void KinectTracker::playVideoCB()
{
	if (playing_video)
	{
		params.setOptions("Play video", "label=`Play video`");
		if (kinect_available)
			params.setOptions("Save video", "readonly=false");
		playing_video = !playing_video;
	}
	else
	{
		fs::path movie_path = getOpenFilePath();
		if (!movie_path.empty())
		{
			loadMovieFile(movie_path);
			params.setOptions("Play video", "label=`Use kinect`");
			params.setOptions("Save video", "readonly=true");
			playing_video = !playing_video;
		}
	}
}

void KinectTracker::loadMovieFile(const fs::path &movie_path)
{
    try
	{
        movie = qtime::MovieSurface(movie_path);
        movie.setLoop();
        movie.play();
    }
    catch (...)
	{
        console() << "Unable to load movie " << movie_path << std::endl;
        movie.reset();
    }
}

void KinectTracker::saveVideoCB()
{
	if (saving_video)
	{
		params.setOptions("Save video", "label=`Save video`");
		movie_writer.finish();
		params.setOptions("Play video", "readonly=false");
	}
	else
	{
		params.setOptions("Save video", "label=`Finish saving`");
		params.setOptions("Play video", "readonly=true");

		qtime::MovieWriter::Format format;
		format.setCodec(qtime::MovieWriter::CODEC_H264);
		format.setQuality(0.5f);
		format.setDefaultDuration(1./25.);

		movie_writer = qtime::MovieWriter(getDocumentsDirectory() / fs::path( "kinect_depth-" +
				timeStamp() + ".mov"), 640, 480, format);
	}
	saving_video = !saving_video;
}

string KinectTracker::timeStamp()
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

void KinectTracker::update()
{
	if (playing_video && movie)
	{
		ImageSourceRef is = movie.getSurface();
		depth_surface = is;
		depth_texture = is;
	}
	else
	if (kinect_available && kinect->checkNewDepthFrame())
	{
		if (static_cast<int>(kinect->getTilt()) != static_cast<int>(tilt))
			kinect->setTilt(tilt);

		ImageSourceRef is = kinect->getDepthImage();
		depth_surface = is;
		depth_texture = is;

		// mirror
		if (vflip)
		{
			uint8_t *data = depth_surface.getData();
			int32_t width = depth_surface.getWidth();
			int32_t height = depth_surface.getHeight();
			int32_t row_bytes = depth_surface.getRowBytes();
			uint8_t pixel_inc = depth_surface.getPixelInc();

			unsigned offset = 0;
			unsigned end_offset;
			for (int y = 0; y < height; y++)
			{
				end_offset = offset + (width - 1) * pixel_inc;
				for (int x = 0; x < width / 2; x++)
				{
					unsigned xo = x * pixel_inc;
					uint8_t d;
					for (int j = 0; j < pixel_inc; j++)
					{
						d = data[offset + xo + j];
						data[offset + xo + j] = data[end_offset - xo + j];
						data[end_offset - xo + j] = d;
					}
				}
				offset += row_bytes;
			}
		}
	}

	if (depth_surface)
	{
		// opencv
		cv::Mat input(toOcv(Channel8u(depth_surface)));
		cv::Mat blurred, thresholded;

		cv::blur(input, blurred, cv::Size(blur_size, blur_size));
		cv::threshold(blurred, thresholded, threshold, 255, CV_THRESH_BINARY);

		cv_blurred = gl::Texture(fromOcv(blurred));
		cv_thresholded = gl::Texture(fromOcv(thresholded));

		vector< vector<cv::Point> > contours;
		cv::findContours(thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		float min_area_limit = 640 * 480 * min_area;
		float max_area_limit = 640 * 480 * max_area;

		blobs.clear();
		for (vector<vector<cv::Point> >::iterator cit = contours.begin(); cit < contours.end(); ++cit)
		{
			Blob b;
			cv::Mat pmat = cv::Mat(*cit);
			cv::Rect cv_rect = cv::boundingRect(pmat);
			b.bbox = Rectf(cv_rect.x, cv_rect.y, cv_rect.x + cv_rect.width, cv_rect.y + cv_rect.height);
			float area = b.bbox.calcArea();
			if ((min_area_limit <= area) && (area < max_area_limit))
			{
				cv::Moments m = cv::moments(pmat);
				b.centroid = Vec2f(m.m10 / m.m00, m.m01 / m.m00);
				blobs.push_back(b);
			}
		}

		if (saving_video)
			movie_writer.addFrame(depth_surface);
	}
}

void KinectTracker::draw(Vec2f tr /* (0, 0) */, float opacity /* = 1.0 */)
{
	if (!debug_tracker)
		return;

	gl::setMatricesWindow(getWindowWidth(), getWindowHeight());

	gl::pushModelView();
	gl::translate(tr);

	gl::color(ColorA(1.0f, 1.0f, 1.0f, opacity));
	if (depth_texture)
	{
		gl::draw(depth_texture);
	}
	if (cv_blurred)
	{
		gl::draw(cv_blurred, Rectf(640, 0, 640 + 320, 240));
	}

	if (cv_thresholded)
	{
		gl::draw(cv_thresholded, Rectf(640, 0, 640 + 320, 240) + Vec2f(0, 240));
	}

	for (vector<Blob>::const_iterator i = blobs.begin(); i < blobs.end(); ++i)
	{
		PolyLine<Vec2f> p;
		Rectf bbox = (*i).bbox * .5 + Vec2f(640, 240);
		p.push_back(bbox.getUpperLeft());
		p.push_back(bbox.getUpperRight());
		p.push_back(bbox.getLowerRight());
		p.push_back(bbox.getLowerLeft());
		p.setClosed();

		gl::color(ColorA(1.0f, 0.0f, 0.0f, opacity));
		gl::draw(p);
		gl::drawSolidCircle(((*i).centroid) * .5 + Vec2f(640, 240), 2);
	}
	gl::color(Color(1.0f, 1.0f, 1.0f));
	gl::popModelView();
}

