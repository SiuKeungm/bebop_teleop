#include "ManualControl.h"
#include "Patroller.h"
#include "StateTracker.h"
#include "Window.h"
#include <SDL2/SDL_ttf.h>
#include <iomanip>
#include <math.h>
#include <ros/ros.h>
#include <sstream>

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 368
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 400

#define PREC(p) std::fixed << std::setprecision(p)

Window* window;

std::string Window::font_path;

GUIC* wifi;
GUIC* batt;

GUIC* lat;
GUIC* lon;
GUIC* alt;

GUIC* velx;
GUIC* vely;
GUIC* velz;

GUIC* cmdx;
GUIC* cmdy;
GUIC* cmdz;
GUIC* cmdr;

GUIC* patrol;

GUIC* labels;
GUIC* labelr;
GUIC* speed;
GUIC* speedr;
GUIC* inc;
GUIC* incr;
GUIC* dec;
GUIC* decr;

// const char* format(double num, int prec) {
// std::stringstream stream;
// if( !isnanf(num) ) stream << std::fixed << std::setprecision(prec) << num;
// else stream << "No Fix";
// return stream.str().c_str();
// }

void reset(std::stringstream& str) {
	str.str( std::string() );
	str.clear();
}

Window::Window(bool& err) {
	err = init();
}

Window::Window() {
	init();
}

Window::~Window() {
	destroy();
}

void Window::event(SDL_Event* event) {
	if(!alive) return;

	if(event->type == SDL_QUIT) {alive = false;} else if(event->type == SDL_MOUSEBUTTONDOWN) {
		if( patrol->inside(event->button.x, event->button.y) ) patrol->callCB();
		if( inc->inside(event->button.x, event->button.y) ) inc->callCB();
		if( incr->inside(event->button.x, event->button.y) ) incr->callCB();
		if( dec->inside(event->button.x, event->button.y) ) dec->callCB();
		if( decr->inside(event->button.x, event->button.y) ) decr->callCB();
	}
}

void Window::updateVideoTexture(const sensor_msgs::ImageConstPtr& img) {
	// ROS_INFO("GOT IMAGE WITH FORMAT: %s, size (%d, %d)", img->encoding.c_str(), img->width, img->height);


	bool bgr = strncmp(img->encoding.c_str(), "bgr", 3) == 0;

	int pitch = img->step;

	char* pixels;
	int texPitch;
	SDL_LockTexture(video, NULL, (void**) &pixels, &texPitch);

	if(pixels == NULL) goto END;

	int texIndex;
	int iIndex;
	int iCol;
	for(int row = 0; row < VIDEO_HEIGHT; row++) {
		iCol = 0;
		for(int col = 0; col + 3 < texPitch; col += 4) {
			if(iCol + 2 >= pitch) continue;
			texIndex = row * texPitch + col;
			iIndex = row * pitch + iCol;
			if(bgr) {
				pixels[texIndex + 0] = img->data[iIndex + 0];	// red
				pixels[texIndex + 1] = img->data[iIndex + 1];	// green
				pixels[texIndex + 2] = img->data[iIndex + 2];	// blue
			} else {
				pixels[texIndex + 0] = img->data[iIndex + 2];	// red
				pixels[texIndex + 1] = img->data[iIndex + 1];	// green
				pixels[texIndex + 2] = img->data[iIndex + 0];	// blue
			}
			pixels[texIndex + 3] = 0xFF;// alpha - doesn't matter unless blending
			iCol += 3;
		}
	}
	video_dirty = true;
END:
	SDL_UnlockTexture(video);
}

SDL_Rect r = {0, WINDOW_HEIGHT - VIDEO_HEIGHT, VIDEO_WIDTH, VIDEO_HEIGHT};
void Window::update() {
	// do update
	if( !ok() ) return;

	if(video_dirty) {
		// center image in pane

		SDL_RenderCopy(ren, video, NULL, &r);

		// SDL_RenderCopy(ren, video, NULL, NULL);

		video_dirty = false;
	}

	// update

	std::stringstream str;
	bool gpsfix = stats->hasFix();
	str << "Wifi: l" << ( abs( stats->getWifiStrength() ) > 75 ? "+      " : ( abs( stats->getWifiStrength() ) > 50 ? "++    " : (abs( stats->getWifiStrength() ) > 20 ? "+++  " : "++++") ) ) << "l";
	wifi->setText(str.str(), ren);
	wifi->render(ren);

	reset(str);
	str << "BAT: " << stats->getBattery() << "%";
	batt->setText(str.str(), ren);
	batt->render(ren);

	reset(str);
	str << "LAT: " << PREC(5);
	if(gpsfix) str << stats->getLatitude() << "*";
	else str << "No Fix";
	lat->setText(str.str(), ren);
	lat->render(ren);

	reset(str);
	str << "LON: " << PREC(5);
	if(gpsfix) str << stats->getLongitude() << "*";
	else str << "No Fix";
	lon->setText(str.str(), ren);
	lon->render(ren);

	reset(str);
	str << "ALT: " << PREC(2);
	if(gpsfix) str << stats->getAltitude() << " m";
	else str << "No Fix";
	alt->setText(str.str(), ren);
	alt->render(ren);


	reset(str);
	str << "XVEL: " << PREC(2) << stats->getXVelocity() << " m/s";
	velx->setText(str.str(), ren);
	velx->render(ren);

	reset(str);
	str << "YVEL: " << PREC(2) << stats->getYVelocity() << " m/s";
	vely->setText(str.str(), ren);
	vely->render(ren);

	reset(str);
	str << "ZVEL: " << PREC(2) << stats->getZVelocity() << " m/s";
	velz->setText(str.str(), ren);
	velz->render(ren);

	reset(str);
	str << "CMDX: " << PREC(2) << control->getLast()->linear.x;
	cmdx->setText(str.str(), ren);
	cmdx->render(ren);

	reset(str);
	str << "CMDY: " << PREC(2) << control->getLast()->linear.y;
	cmdy->setText(str.str(), ren);
	cmdy->render(ren);

	reset(str);
	str << "CMDZ: " << PREC(2) << control->getLast()->linear.z;
	cmdz->setText(str.str(), ren);
	cmdz->render(ren);

	reset(str);
	str << "CMDR: " << PREC(2) << control->getLast()->angular.z;
	cmdr->setText(str.str(), ren);
	cmdr->render(ren);

	reset(str);
	str << PREC(2) << control->getSpeed();
	speed->setText(str.str(), ren);
	speed->render(ren);

	reset(str);
	str << PREC(2) << control->getRotSpeed();
	speedr->setText(str.str(), ren);
	speedr->render(ren);

	inc->render(ren);
	dec->render(ren);
	incr->render(ren);
	decr->render(ren);

	labels->render(ren);
	labelr->render(ren);

	patrol->render(ren);


	// FUCK THIS THING. SOLID 3 hours GOOONNNNEEE because it was in the if
	SDL_RenderPresent(ren);
}

bool Window::ok() {
	return alive;
}

void destGUI() {
	delete cmdx;
	delete cmdy;
	delete cmdz;
	delete cmdr;
	delete velx;
	delete vely;
	delete velz;
	delete batt;
	delete wifi;
	delete lat;
	delete lon;
	delete alt;
	delete patrol;
}

void Window::destroy() {
	alive = false;

	TTF_CloseFont(font);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	destGUI();
	TTF_Quit();
	SDL_Quit();
}

void Window::makeGUI() {
	batt = new GUIC(font, 4, 4, -1, 24);

	int i;
	TTF_SizeText(font, "BAT: 000%", &i, NULL);
	wifi = new GUIC(font, i + 8, 4, -1, 24);

	lat = new GUIC(font, VIDEO_WIDTH + 4, 4, -1, 24);
	lon = new GUIC(font, VIDEO_WIDTH + 4, 4 + 24 + 4, -1, 24);
	alt = new GUIC(font, VIDEO_WIDTH + 4, 4 + 48 + 8, -1, 24);

	cmdx = new GUIC(font, VIDEO_WIDTH + 4, WINDOW_HEIGHT - 28 * 8, -1, 24);
	cmdy = new GUIC(font, VIDEO_WIDTH + 4, WINDOW_HEIGHT - 28 * 7, -1, 24);
	cmdz = new GUIC(font, VIDEO_WIDTH + 4, WINDOW_HEIGHT - 28 * 6, -1, 24);
	cmdr = new GUIC(font, VIDEO_WIDTH + 4, WINDOW_HEIGHT - 28 * 5, -1, 24);

	velx = new GUIC(font, VIDEO_WIDTH + 4, WINDOW_HEIGHT - 28 * 3, -1, 24);
	vely = new GUIC(font, VIDEO_WIDTH + 4, WINDOW_HEIGHT - 28 * 2, -1, 24);
	velz = new GUIC(font, VIDEO_WIDTH + 4, WINDOW_HEIGHT - 28, -1, 24);

	#define H 24
	int W;
	TTF_SizeText(font, "1.00", &W, NULL);
	#define X VIDEO_WIDTH - 400
	#define X2 VIDEO_WIDTH - 200
	#define Y 4
	labels = new GUIC(font, X, Y, -1, H);
	speed = new GUIC(font, X + 70, Y, W, H);
	inc = new GUIC(font, X + 72 + W, Y, H, H);
	dec = new GUIC(font, X + 68 - H, Y, H, H);

	labelr = new GUIC(font, X2, Y, -1, H);
	speedr = new GUIC(font, X2 + 70, Y, W, H);
	incr = new GUIC(font, X2 + 72 + W, Y, H, H);
	decr = new GUIC(font, X2 + 68 - H, Y, H, H);

	// ensure you set colors before text
	inc->setBG(100, 130, 100);
	incr->setBG(100, 130, 100);
	dec->setBG(130, 100, 100);
	decr->setBG(130, 100, 100);
	labels->setText("Spd:", ren, GUIC::RESIZE_X);
	labelr->setText("Rot:", ren, GUIC::RESIZE_X);
	inc->setText(">", ren, GUIC::RESIZE_NONE);
	incr->setText(">", ren, GUIC::RESIZE_NONE);
	dec->setText("<", ren, GUIC::RESIZE_NONE);
	decr->setText("<", ren, GUIC::RESIZE_NONE);
	speed->setText("1.00", ren, GUIC::RESIZE_NONE);
	speedr->setText("1.00", ren, GUIC::RESIZE_NONE);

	inc->setCallback([] (GUIC * g) {control->incSpeed();});
	incr->setCallback([] (GUIC * g) {control->incRotSpeed();});
	dec->setCallback([] (GUIC * g) {control->decSpeed();});
	decr->setCallback([] (GUIC * g) {control->decRotSpeed();});

	patrol = new GUIC(font, VIDEO_WIDTH + 4, WINDOW_HEIGHT - 28 * 11, -1, 24);
	patrol->setBG(100, 100, 100);
	patrol->setText(" start patrol ", ren);
	patrol->setCallback(
		[this](GUIC * g) {
			if(g->getText()[3] == 'o') {
				g->setBG(100, 100, 100);
				g->setText(" start patrol ", ren, 0);
				patroller->stop();
			} else {
				g->setBG(170, 70, 70);
				g->setText(" stop patrol ", ren, 0);
				patroller->start(2, 0.25, 0.08);
			}
		});
}

bool Window::init() {
	alive = false;
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		ROS_ERROR( "SDL INIT FAIL: %s\n", SDL_GetError() );
		return true;
	}

	alive = true;
	if(TTF_Init() < 0) {
		ROS_ERROR( "TTF INIT FAIL: %s\n", TTF_GetError() );
		return true;
	}

	font = TTF_OpenFont(font_path.c_str(), 20);
	if(font == NULL) ROS_ERROR( "TTF FONT LOAD FAIL: %s\n", TTF_GetError() );

	// return true;
	SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL, &win, &ren);
	if(win == NULL || ren == NULL) {
		ROS_ERROR( "SDL CREATE WINDOW FAIL: %s\n", SDL_GetError() );
		return true;
	}

	video = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, VIDEO_WIDTH, VIDEO_HEIGHT);
	if(video == NULL) {
		ROS_ERROR( "SDL CREATE TEXTURE FAIL: %s\n", SDL_GetError() );
		return true;
	}
	registerEventListener(this);
	SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
	SDL_RenderClear(ren);
	SDL_RenderPresent(ren);

	makeGUI();
	return false;
}
