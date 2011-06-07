#include "cinder/Xml.h"
#include "cinder/Filesystem.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Io.h"

#include "KinectReader.h"

#include "AntTweakBar.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void KinectReader::prepareSettings(Settings *settings)
{
	settings->setWindowSize(1100, 600);
}

void KinectReader::setup()
{
	string ui_files[] = {"flip", "zoom", "pan"};

	string data_path = getAppPath();
#ifdef CINDER_MAC
	data_path += "/Contents/Resources/";
#endif
	logo = loadImage(data_path + "logo.png");

	data_path += "ui/";

	for (int i = 0; i < 3; i++)
	{
		ui_panels[i][0] = loadImage(data_path + ui_files[i] + "-0.png");
		ui_panels[i][1] = loadImage(data_path + ui_files[i] + "-1.png");
	}

	load_samples();

	audio::Output::setVolume(1.0);

	Area book_area = getWindowBounds();
	pc = new PageCurl("book", book_area);

	cstate = LOADING;
	idle = false;
	page_zoom = 1.0;
	page_zoom_pos = Vec2f(.0, .0);
	cursors_movement = Vec2f(0, 0);

	// hand tracker parameters
	state_change_period = 1.5;
	pageflip_frame_thr = 10;

	zooming_speed = 3.0;
	zoom_min = 1.0;
	zoom_max = 5.0;
	zoom_frame_thr = 5;

	panning_speed = 2.0;

	idle_time = 10.0;


	// hand tracker gui
	params = params::InterfaceGl("HandTracker", Vec2i(220, 300));
	params.addParam("State change period", &state_change_period, "min=0.5 max=5.0 step=.25");
	params.addParam("Flip frame thr", &pageflip_frame_thr, "min=2 max=25 step=1");
	params.addParam("Zooming speed", &zooming_speed, "min=0.1 max=10.0 step=.1");
	params.addParam("Minimum zoom", &zoom_min, "min=1. max=10.0 step=.1");
	params.addParam("Maximum zoom", &zoom_max, "min=1. max=10.0 step=.1");
	params.addParam("Zoom frame thr", &zoom_frame_thr, "min=2 max=25 step=1");
	params.addParam("Panning speed", &panning_speed, "min=0.1 max=10.0 step=.1");
	params.addParam("Idle time", &idle_time, "min=3.0 max=50.0 step=1");

	params.addSeparator();
	params.addButton("Load config", std::bind(&KinectReader::load_config, this));
	params.addButton("Save config", std::bind(&KinectReader::save_config, this));

	// kinect tracker
	tracker.setup();

	// FIXME: setOptions is not working
	//params.setOptions("HandTracker", " iconified=true ");
	TwDefine(" HandTracker iconified=true ");
	TwDefine(" KinectTracker iconified=true position='256 16' ");
	TwDefine(" GLOBAL iconpos=bottomright iconalign=horizontal ");
	TwDefine(" TW_HELP visible=false ");

	load_config();

	last_mouse_action = -MOUSE_IDLE;

#ifndef DEBUG
	setFullScreen(true);
	hideCursor();
#endif
}

void KinectReader::shutdown()
{
	delete pc;
}

void KinectReader::resize(ResizeEvent event)
{
	Area book_area(0, 0, event.getWidth(), event.getHeight());
	int margin = static_cast<int>(event.getHeight() * .1);
	float left_panel = 4. * getWindowHeight() / 15.;
	book_area.setX1(left_panel);
	book_area.expand(-margin, -margin);
	console() << "resize " << book_area << endl;
	pc->resize(book_area);
}

void KinectReader::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
	{
		setFullScreen(!isFullScreen());
	}

	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void KinectReader::keyUp(KeyEvent event)
{
	if (event.getCode() == KeyEvent::KEY_LEFT)
		pc->prev();
	else
	if (event.getCode() == KeyEvent::KEY_RIGHT)
		pc->next();
}

void KinectReader::mouseDown(MouseEvent event)
{
	last_mouse_action = getElapsedSeconds();
}

void KinectReader::mouseUp(MouseEvent event)
{
	last_mouse_action = getElapsedSeconds();
}

void KinectReader::mouseMove(MouseEvent event)
{
	last_mouse_action = getElapsedSeconds();
}

void KinectReader::mouseDrag(MouseEvent event)
{
	last_mouse_action = getElapsedSeconds();
}

void KinectReader::update()
{
	tracker.update();
}

void KinectReader::load_samples()
{
	string data_path = getAppPath();
#ifdef CINDER_MAC
	data_path += "/Contents/Resources/";
#endif
	data_path += "ui-sfx/";

	samples.clear();

	fs::path p(data_path);
	for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it) && (it->path().extension().string() == ".mp3"))
		{
#ifdef DEBUG
			console() << "   " << it->path().filename() << endl;
#endif
			samples.push_back(audio::load(data_path + it->path().filename().string()));
		}
	}
}

bool KinectReader::cursors_fixed()
{
	bool r = false;
	static vector<KinectTracker::Blob> last_blobs(tracker.blobs);
	cursors_movement = Vec2f(0, 0);

	if (last_blobs.size() != tracker.blobs.size())
	{
		goto finish;
	}

	for (int i = 0; i < tracker.blobs.size(); i++)
	{
		cursors_movement += tracker.blobs[i].centroid - last_blobs[i].centroid;
	}
	if (!tracker.blobs.empty())
		cursors_movement /= tracker.blobs.size();

	for (int i = 0; i < tracker.blobs.size(); i++)
	{
		// TODO: add distance threshold parameter
		if (tracker.blobs[i].centroid.distance(last_blobs[i].centroid) > 20)
		{
			goto finish;
		}
	}
	r = true;

finish:
	last_blobs.clear();
	last_blobs = tracker.blobs;
	return r;
}

int KinectReader::detect_state_change()
{
	static float last_cur0_time = 0;
	static float last_cur1_time = 0;
	static float last_cur2_time = 0;
	static unsigned last_cur_count = 0;
	static int last_state = FLIPPING;
	int current_state = last_state;

	float ctime = getElapsedSeconds();
	unsigned nblobs = tracker.blobs.size();

	bool cur_fixed = cursors_fixed();

	idle = (nblobs == 0);

	if (nblobs == 0)
	{
		if (last_cur_count != 0)
		{
			last_cur0_time = ctime;
		}

		if (cur_fixed)
		{
			if (ctime - last_cur0_time > state_change_period)
			{
				current_state = IDLE;
			}
		}
		else
		{
			last_cur1_time = ctime;
		}
	}
	else
	if (nblobs == 1)
	{
		if (last_cur_count != 1)
		{
			last_cur1_time = ctime;
		}

		if (cur_fixed)
		{
			if (ctime - last_cur1_time > state_change_period)
			{
				current_state = (page_zoom > zoom_min) ? PANNING : FLIPPING;
			}
		}
		else
		{
			last_cur1_time = ctime;
		}
	}
	else
	if (nblobs == 2)
	{
		if (last_cur_count != 2)
		{
			last_cur2_time = ctime;
		}

		if (cur_fixed)
		{
			if (ctime - last_cur2_time > state_change_period)
			{
				current_state = ZOOMING;
			}
		}
		else
		{
			last_cur2_time = ctime;
		}
	}

	if (last_state != current_state)
	{
		if (samples.size() > current_state)
		{
			audio::Output::play(samples[current_state]);
		}
		else
		if (!samples.empty())
		{
			audio::Output::play(samples[0]);
		}
	}

	last_state = current_state;
	last_cur_count = nblobs;

	return current_state;
}

void KinectReader::controller()
{
	cstate = detect_state_change();

	unsigned nblobs = tracker.blobs.size();

	if (nblobs == 0)
		return;

	if (cstate == FLIPPING)
	{
		// number of frames the cursor is going in the direction
		static int cursor_going_left = 0;
		static int cursor_going_right = 0;
		static int last_cursor_x = 320; // x position of the cursor in the last frame

		Vec2f centroid(0, 0);
		for (int i = 0; i < nblobs; i++)
		{
			centroid += tracker.blobs[i].centroid;
		}
		centroid /= nblobs;

		if (centroid.x < last_cursor_x)
		{
			cursor_going_left++;
			cursor_going_right = 0;
		}
		else if (centroid.x > last_cursor_x)
		{
			cursor_going_left = 0;
			cursor_going_right++;
		}

		if (cursor_going_left > pageflip_frame_thr)
		{
			pc->next();
			cursor_going_left = 0;
		}
		if (cursor_going_right > pageflip_frame_thr)
		{
			pc->prev();
			cursor_going_right = 0;
		}

		last_cursor_x = centroid.x;
	}
	else
	if (cstate == ZOOMING)
	{
		static int cursor_zooming_in = 0; // number of frames the cursor is zooming in the direction
		static int cursor_zooming_out = 0;
		static float last_cursor_d = 0; // distance of the cursors in the last frame

		float mind = 0; // minimum distance between the blobs

		if (nblobs > 1)
		{
			mind = 9999;
			for (int i = 0; i < nblobs; i++)
			{
				for (int j = 0; j < nblobs; j++)
				{
					if (j == i)
						continue;

					float d = tracker.blobs[j].centroid.distance(tracker.blobs[i].centroid);
					if (d < mind)
						mind = d;
				}
			}
		}

		if (mind < last_cursor_d)
		{
			cursor_zooming_in++;
			cursor_zooming_out = 0;
		}
		else
		if (mind > last_cursor_d)
		{
			cursor_zooming_in = 0;
			cursor_zooming_out++;
		}

		if ((cursor_zooming_in > zoom_frame_thr) ||
			(cursor_zooming_out > zoom_frame_thr))
		{
			float dzoom = zooming_speed * (mind - last_cursor_d) / 640.;
			page_zoom = lerp(page_zoom, page_zoom + dzoom, .95);
		}

		last_cursor_d = mind;
	}
	else
	if (cstate == PANNING)
	{
		Vec2f mov = cursors_movement / Vec2f(640, 480) * panning_speed;
		page_zoom_pos = page_zoom_pos.lerp(.5, page_zoom_pos - mov);
		if (page_zoom_pos.x < -1.0)
			page_zoom_pos.x = -1.0;
		else
		if (page_zoom_pos.x > 1.0)
			page_zoom_pos.x = 1.0;

		if (page_zoom_pos.y < -1.0)
			page_zoom_pos.y = -1.0;
		else
		if (page_zoom_pos.y > 1.0)
			page_zoom_pos.y = 1.0;
		//console() << "zoom pos: " << page_zoom_pos << endl;
	}

	// correct page-zoom and zoom-pos
	if (page_zoom <= zoom_min)
	{
		page_zoom = zoom_min;
		page_zoom_pos = Vec2f(.0, .0);
	}
	else
	if (page_zoom >= zoom_max)
	{
		page_zoom = zoom_max;
	}
}

void KinectReader::save_config()
{
	string data_path = getAppPath();
#ifdef CINDER_MAC
	data_path += "/Contents/Resources/";
#endif
	data_path += "reader.xml";
	XmlTree config = XmlTree::createDoc();

	// TODO: automate this
	XmlTree t("param", "");
	string parameters[] = {"state_change_period", "pageflip_frame_thr", "zooming_speed",
		"zoom_min", "zoom_max", "zoom_frame_thr", "panning_speed", "idle_time"};
	float values[] = {state_change_period, pageflip_frame_thr, zooming_speed,
		zoom_min, zoom_max, zoom_frame_thr, panning_speed, idle_time};

	for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++)
	{
		t = XmlTree("param", "");
		t.setAttribute("name", parameters[i]);
		t.setAttribute("value", values[i]);
		config.push_back(t);
	}
	config.write(DataTargetPath::createRef(data_path));
}

void KinectReader::load_config()
{
	string data_path = getAppPath();
#ifdef CINDER_MAC
	data_path += "/Contents/Resources/";
#endif
	data_path += "reader.xml";

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
		string parameters[] = {"state_change_period", "pageflip_frame_thr", "zooming_speed",
			"zoom_min", "zoom_max", "zoom_frame_thr", "panning_speed", "idle_time"};
		float *valptrs[] = {&state_change_period, &pageflip_frame_thr, &zooming_speed,
			&zoom_min, &zoom_max, &zoom_frame_thr, &panning_speed, &idle_time};

		for (int i = 0; i < sizeof(valptrs) / sizeof(valptrs[0]); i++)
		{
			if (name == parameters[i])
			{
				*(valptrs[i]) = v;
			}
		}
	}
}

void KinectReader::draw_book()
{
	pc->set_zoom(page_zoom);
	pc->set_zoom_pos(page_zoom_pos);
	pc->draw();

	gl::disableDepthRead();

	float h = getWindowHeight() / 15.;
	float starth = 3 * h;
	h *= 4;

	// thresholded camera image
	{
		gl::Texture thr_img = tracker.get_thr_image();
		Rectf rect(0, 0, h, starth);
		if (thr_img)
			gl::draw(thr_img, thr_img.getBounds(), rect);
	}

	// ui panels
	for (int i = 0; i < 3; i++)
	{
		gl::Texture t = ui_panels[i][i == (cstate - 2)];
		Area area = t.getBounds();
		Rectf rect(0, starth + i * h, h, starth + (i + 1) * h);

		gl::draw(t, area, rect);
	}
}

void KinectReader::draw_logo()
{
	float logo_aspect_ratio = logo.getWidth() / static_cast<float>(logo.getHeight());
	Area logo_area = getWindowBounds();
	float window_aspect_ratio = getWindowAspectRatio();

	if (window_aspect_ratio > logo_aspect_ratio)
	{
		int margin = (getWindowWidth() - getWindowHeight() * logo_aspect_ratio) / 2;
		logo_area.expand(-margin, 0);
	}
	else
	{
		int margin = (getWindowHeight() - getWindowWidth() / logo_aspect_ratio) / 2;
		logo_area.expand(0, -margin);
	}

	gl::draw(logo, Rectf(logo_area));
}

void KinectReader::draw_loading(float progress)
{
	float w = getWindowWidth();
	float h = getWindowHeight();
	float h2 = h / 2.;
	float s = h / 20;

	Rectf rect(Vec2f(0, h2 - s), Vec2f(progress * w, h2 + s));
	gl::color(Color::white());
	gl::drawSolidRect(rect);
}

void KinectReader::draw()
{
	static float last_action_time = -idle_time;
	float ctime = getElapsedSeconds();

	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow(getWindowSize());

	if (cstate == LOADING)
	{
		float f;
		if (pc->preload(&f))
			cstate = IDLE;
		console() << "loading " << f * 100 << endl;
		draw_loading(f);
		return;
	}

	controller();

	if ((ctime - last_action_time) > idle_time)
	{
		draw_logo();
		page_zoom = zoom_min;
		page_zoom_pos = Vec2f(0, 0);
	}
	else
	{
		draw_book();
	}

	if (!idle) // (cstate > IDLE)
		last_action_time = ctime;

	float h = 4 * getWindowHeight() / 15.;
	tracker.draw(Vec2f(h, 0), .9);

	// gui
	bool show_gui;
	if (ctime - last_mouse_action > MOUSE_IDLE)
	{
		show_gui = false;
	}
	else
	{
		show_gui = true;
	}

	if (show_gui)
	{
		showCursor();
		params::InterfaceGl::draw();
	}
	else
	{
		if (isFullScreen())
			hideCursor();
	}
}

CINDER_APP_BASIC(KinectReader, RendererGl)

