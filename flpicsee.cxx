// flPicSee  -- A FLTK-based picture viewer 
#define appver "0.8.8"	// Last update 2009-05-06

/* -------------------------------------------------------
   Copyright 2009 Michael A. Losh
   
    "flPicSee" (previously known as "fl_picsee" and "FLTK - Picture See")
    is free software: you can redistribute it and/or modify it under the 
	terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    flPicSee is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with flPicSee.  If not, see <http://www.gnu.org/licenses/>.
-----------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_BMP_Image.H>
#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_XBM_Image.H>
#include <FL/Fl_XPM_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Tooltip.H>

enum {NATIVE_ZOOM = 0, WHOLE_IMG_ZOOM = 1, FIT_ZOOM = 2};

class Fl_Pic_Window : public Fl_Window {
protected:
    int zoommode;
	int img_h, img_w;	// original size of image
	int ful_h, ful_w;	// size of image so that the full image is visible without scrolling in current window
	int fit_h, fit_w;	// size of image so it fills current window with scrolling in only one dimension
	int scn_h, scn_w;	// screen size (X-Windows screen dimension)
	int wnd_h, wnd_w;   // current window size
	int xoffset, yoffset;
	int zoompct;		// scaling "zoom" as a percentage (100 = 100%)
			
	float img_hwratio, wnd_hwratio, scn_hwratio;

	Fl_Image * im_orig_p; 
	Fl_Image * im_full_p;
	Fl_Image * im_fit_p;
	
	
	char wintitle[256];
	char tiptitle[256];
	char imgpath[256];
	char imgname[256];
	char scalestr[8];
	char buf[256];
	char *p;
	char ext[8];
	int len;
		
	Fl_Scroll* scroll_p; //scroll(0, 0, wnd_w, wnd_h);
	Fl_Box*    pic_box_p;

	void scale_image(void) 
	{
		switch(zoommode) {
			case NATIVE_ZOOM:
				pic_box_p->resize((w() - img_w) / 2, (h() - img_h) / 2, img_w, img_h);
				pic_box_p->image(im_orig_p);  
				zoompct = 100;
				break;
			case WHOLE_IMG_ZOOM:
				pic_box_p->resize((w() - ful_w) / 2, (h() - ful_h) / 2, ful_w, ful_h);
				pic_box_p->image(im_full_p);
				zoompct = (ful_w * 100) / img_w;
				break;
			case FIT_ZOOM:
				pic_box_p->resize((w() - fit_w) / 2, (h() - fit_h) / 2, fit_w, fit_h);
				pic_box_p->image(im_fit_p);
				zoompct = (fit_w *100)/ img_w;
				break;
		}
		sprintf(scalestr, "%4d%%", zoompct);
		sprintf(tiptitle, "%s %s/%s", scalestr, imgpath, imgname, appver);
		tooltip(tiptitle);
		sprintf(wintitle, "%s %s - flPicSee %s (%s/%s)", scalestr, imgname, appver, imgpath, imgname);
		label(wintitle);

		redraw();
	}
	
	void resize(int x, int y, int w, int h)	{
		wnd_w = w;
		wnd_h = h;
		Fl_Group::resize(x, y, w, h);
		
		wnd_hwratio = (float)wnd_h / (float)wnd_w;
		xoffset = 0;
		yoffset = 0;

		if (img_hwratio > wnd_hwratio) {
			ful_h = h;
			ful_w = (int)((float)ful_h / img_hwratio);
			fit_w = w - 32;  // allow room for scrollbar
			fit_h = (int)((float)fit_w * img_hwratio);
		}
		else {
			ful_w = w;
			ful_h = (int)((float)ful_w * img_hwratio);
			fit_h = h - 32; // allow room for scrollbar
			fit_w = (int)((float)fit_h / img_hwratio);
		} 
		if (im_full_p) {
			delete im_full_p;
			im_full_p = im_orig_p->copy(ful_w, ful_h);
		} 
		if (im_fit_p) {
			delete im_fit_p;
			im_fit_p = im_orig_p->copy(fit_w, fit_h);
		} 
		scale_image();
	}
	
    int handle(int e) {
		char s[200];
		int ret;
		switch ( e ) {
			case FL_RELEASE:
				if ( Fl::event_button() == FL_LEFT_MOUSE ) {
					zoommode = (zoommode + 1) % 3;
					scale_image();					
					redraw(); 
					return(1);
				}
			case FL_SHOW:
				return 1;
			default:
				ret = Fl_Window::handle(e);
				break;

		}
        return(ret);
    }
	
public:
	Fl_Pic_Window(const char *fullfilename=0);
	~Fl_Pic_Window() {
		if 	(im_orig_p) delete im_orig_p; 
		if 	(im_full_p) delete im_full_p; 
		if 	(im_fit_p) delete im_fit_p; 
		delete scroll_p; 
		delete pic_box_p;

	}
};

Fl_Pic_Window::Fl_Pic_Window(const char *fullfilename) : Fl_Window(Fl::w(), Fl::h()) 

{
	im_orig_p = NULL; 
	im_full_p = NULL;
	im_fit_p  = NULL;
	zoommode = 0;
	char *fn_p;
	
	bzero(wintitle, 256);
	bzero(tiptitle, 256);
	bzero(imgpath, 256);
	bzero(imgname, 256);
	bzero(scalestr, 8);

	
	scn_w = Fl::w();
	scn_h = Fl::h() - 32;  // Ammount claimed by window frame/title

	strcpy(scalestr, " 100%");
	
	strncpy(imgpath, fullfilename, 255);
	len = strlen(fullfilename);
	if (len > 4) {
		fn_p = imgpath + len - 1;
		while (*fn_p != '/'&& fn_p > imgpath) {
			fn_p--;
		}
		if (*fn_p == '/') { 
			*fn_p = '\0';
			fn_p++;
		}
		strncpy(imgname, fn_p, 255);
	}
	
	ext[0] = toupper(fullfilename[len - 4]);
	ext[1] = toupper(fullfilename[len - 3]);
	ext[2] = toupper(fullfilename[len - 2]);
	ext[3] = toupper(fullfilename[len - 1]);
	ext[4] = 0;
	
	printf("File type appears to be '%s'\n", ext);
	
	if (!strcmp(ext, ".JPG") || !strcmp(ext, "JPEG")) {
		im_orig_p = new Fl_JPEG_Image(fullfilename); 
	}
	else if (!strcmp(ext, ".PNG")) {
		im_orig_p = new Fl_PNG_Image(fullfilename);
	}
	else if (!strcmp(ext, ".BMP")) {
		im_orig_p = new Fl_BMP_Image(fullfilename); 
	}
	else if (!strcmp(ext, ".GIF")) {
		im_orig_p = new Fl_GIF_Image(fullfilename); 
	}
	else if (!strcmp(ext, ".XBM")) {
		im_orig_p = new Fl_XBM_Image(fullfilename); 
	}
	else if (!strcmp(ext, ".XPM")) {
		im_orig_p = new Fl_XPM_Image(fullfilename); 
	}
	else {
		fl_alert("File '%s' is not a known image type.  flPicSee will exit.\n", fullfilename);
		exit(-1);
	}
	img_h = im_orig_p->h(); 
	img_w = im_orig_p->w(); 

	img_hwratio = (float)img_h / (float)img_w;
	wnd_hwratio = (float)wnd_h / (float)wnd_w;
	scn_hwratio = (float)scn_h / (float)scn_w;
	xoffset = 0;
	yoffset = 0;

	if ((img_h < scn_h) && (img_w < scn_w)) {
		zoommode = 0; // native 1:1 scaling
		wnd_h = img_h;
		wnd_w = img_w;
		ful_h = img_h;
		ful_w = img_w;
		fit_h = img_h;
		fit_w = img_w;
		if (wnd_h < 90) {
			wnd_h = 90;
			xoffset = (wnd_w - img_w) / 2;
		}
		if (wnd_w < 120) {
			wnd_w = 120;
			yoffset = (wnd_h - img_h) / 2;
		}
	}
	else {
		zoommode = 1; // view whole image scaling
		if (img_hwratio > scn_hwratio) {
			ful_h = scn_h;
			ful_w = (int)((float)ful_h / img_hwratio);
			fit_w = scn_w - 32;  // allow room for scrollbar
			fit_h = (int)((float)fit_w * img_hwratio);
		}
		else {
			ful_w = scn_h;
			ful_h = (int)((float)ful_w * img_hwratio);
			fit_h = scn_h - 32; // allow room for scrollbar
			fit_w = (int)((float)ful_h / img_hwratio);
		}
		wnd_h = ful_h;
		wnd_w = ful_w;
	}
	im_full_p = im_orig_p->copy(ful_w, ful_h);
	im_fit_p = im_orig_p->copy(fit_w, fit_h);
	w(wnd_w);
	h(wnd_h);
	
	scroll_p = new Fl_Scroll(0, 0, wnd_w, wnd_h);
	scroll_p->box(FL_NO_BOX);
	scroll_p->begin();
	pic_box_p = new Fl_Box(xoffset, yoffset, ful_w, ful_h);

	scroll_p->end();
	end();
	resizable(pic_box_p); 
	scale_image();
}

int main(int argc, char** argv) 
{
	char filename[256];
	Fl_Pic_Window* wnd_p;
	if (argc > 1) {
		strncpy(filename, argv[1], 255);
	}
	else {
		Fl_File_Chooser* fcd_p = new Fl_File_Chooser("~", 
					"JPEG Files (*.jpg)\tPNG Files (*.png)\tGIF Files (*.gif)\tBMP Files (*.bmp)"
					"\tXBM FIles (*.xbm)\tXPM Files (*.xpm)\tAll Files (*)", 0, "Select Image File");
		fcd_p->textsize(12);
		fcd_p->show();
		Fl::run();
		strncpy(filename, fcd_p->value(), 255);
	}
    fl_register_images();
	wnd_p = new Fl_Pic_Window(filename);  
	wnd_p->show();
	return(Fl::run());
} 
