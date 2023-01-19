/*
  Copyright 1993-2008 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

 
 
 


/*******************
*  In this file:   *
*  TAB = 4 spaces  *
* (Set you editor) *
*******************/


#include <math.h>
#include <Viewnix.h>
#include <assert.h>
#include "voi.h"
#include <stdio.h>


 
#define READY 0
#define WAIT 1
 


int generate_interpolated_scene();
int abort_interpolation();
int dimension_switchboard(), method_switchboard(), distance_switchboard();
int methodx1_switchboard(), methodx2_switchboard(), methodx3_switchboard();
int methodx4_switchboard(), z_extrapolate_switchboard();
int toggle_fore_back_proc();
int redisplay_slices();




/********************/
#include "slices.c"
/********************/

XEvent event;
Window win;

extern Display *display;
extern int 	font_h, font_w;
Visual 	*visual;
GC 		gc;
SLICES sl;	/* structure containing information about the slices of the scene */
int		W, H;		/* size of usable area on the image window */
int 	img_width, img_height;
int		iw, ih,		/* size of each of the windows */
		dw, dh,
		bw, bh;
int		ifw, ifh,	/* width and height of fonts for the different windows */
		dfw, dfh,
		bfw, bfh;
GC		igc,		/* GC of the different windows */
		dgc,
		bgc;
int		dial_height;
int 	fg, bg;
int		OX, OY;		/* Origin for the drawing area */
int		HScalex, HScaley,		/* x,y location of origin of X4 axis (Horizontal axis) */
		VScalex, VScaley,		/* x,y location of origin of X3 axis (Vertical axis) */
		HTicx1, HTicx2,			/* x location of the end-points for the tics of X3 */
		HBigTicx1, HBigTicx2,	/* x locations of the end-points for the Big tics of X3 */
		VTicy1, VTicy2,	/* y locations of the end-points of tics for X4 */
		VIndexy,		/* y location of the Index label for X4 */
		VLocy,			/* y location of the Location label for X4 */
		HIndexx, 		/* x location of the Index label for X3 */
		HLocx,			/* x location of the Location label for X3 */
		HVertAxisx,		/* x location of the X3 axis (vertical axis) */
		VHoriAxisy;		/* y location of the X4 axis (horiaontal axis) */
int sd, tsd;	/* scene dimension */
BUTTON *go, *test, *test2;
BUTTON *background;	/* indicates background execution */
TEXT *host;			/* if background indicates which host */
TEXT *output;	/* output file TEXT ITEM */
TEXT *description_text;
BUTTON *execute, *description; 
TEXT **min3;	/* min for each X3 sample */
TEXT **max3;	/* max for each X3 sample */
TEXT *min4;	/* min for each X4 sample */
TEXT *max4; 	/* max fpr each X4 sample */
TEXT *x1,	/* SAMPLING in each DIRECTION */
	*x2,
	*x3,
	*x4;
int abort_flag = 0;	/* abortion flag */

int	method[4];	/* indicates the interpolation method in each dimension */
int current_dimension,	/* indicates the values shown on each of the switches */
	current_method,
	current_distance; 	/* Shape-Based interp. distance map calculation method */
int z_extrapolate_flag;
float original_res;	/* original resolution of pixel */
char	filename[300];	/* INPUT filename */


ViewnixHeader	vh;
Img_Tree 		*tree;
FILE *fpin, *fpout;

typedef int (*FUNC_PTR)();


GC iigc, idgc, ibgc;	/* GCs */

int num_cmds;
PanelCmdInfo cmds[30];
FUNC_PTR cmd_func[30];

ViewnixColor color[NUM_OF_RESERVED_COLORCELLS];	/* Reserved colors structure */


int isotropic_data_flag=0;	/* indicates if output is isotropic and City Block (binary) */


char filenames[MAX_DEFAULT_FILES][MAX_DEFAULT_CHAR];    /* Used on the SAVE output switch */


sleep_proc()
{
	sleep(3);
}

/*-----------------------------------------------------------------------------------*/
check_button_occurance()
{
    int i,j;
    XEvent xevent;
    int found;
 
    /* check for the existance of any events on the queue */
    i = XEventsQueued(display, QueuedAfterFlush);
    found = 0;
    /* check all events found on the queue */
    if( i > 0 )
    {
        for(j=0; j<i && found == 0; j++)
        {
           VNextEvent( &xevent);
 
           if(xevent.type == ButtonPress)
            found = 1;
        }
 
    }
 
 
    /* Return the button that was pressed */
    if(found == 1)
        return(xevent.xbutton.button);
    else
        return(0);
 
}
 

/*-----------------------------------------------------------------------------------*/
/***************************************************************************
 *    Modified: 2/7/95 exposure events selected by Dewey Odhner
 ***************************************************************************/
setup_image_window(file)
char *file;
{
	int quit;
	int i, j;
	int	cy;		/* current y location (for general use) */
	int posx, posy;		/* position of the slices (in pixels on the screen */
	int	size;			/* size of the base of the slice icon in pixels */
	char element[5], group[5];
   	unsigned long dum;
	char text[200];
	char text2[100];
	int D3, D4;		/* length axis along x3 and x4 dimensions */







	/* The Keyboard is GRABBED inside the EVENT HANDLING Function */
	VSelectEvents(img,ButtonPressMask|PointerMotionMask|EnterWindowMask
        |OwnerGrabButtonMask|KeyPressMask|KeyReleaseMask|ExposureMask);
	VSelectEvents(dial,ButtonPressMask|PointerMotionMask|EnterWindowMask
        |OwnerGrabButtonMask|KeyPressMask|KeyReleaseMask|ExposureMask);
	VSelectEvents(butt,ButtonPressMask|PointerMotionMask|EnterWindowMask
        |OwnerGrabButtonMask|KeyPressMask|KeyReleaseMask|ExposureMask);

	/* Clear entire image window */
	VClearWindow(img,0,0,0,0);


	 /* Get copies of GCs */
    VGetWindowGC(img, &iigc);
    VGetWindowGC(dial, &idgc);
    VGetWindowGC(butt, &ibgc);
 


	/* Get information about the IMAGE window */
	VGetWindowInformation(img,
					&dum,&dum,
					&iw,&ih,	/* image window size */
					&ifw, &ifh,		/* font size */
					&fg,&bg);		/* back. and foreg. color cells */

	/* Get information about the DIALOG window */
	VGetWindowInformation(dial,
					&dum,&dum,
					&dw,&dh,	/* image window size */
					&dfw, &dfh,		/* font size */
					&fg,&bg);		/* back. and foreg. color cells */

	/* Get information about the BUTTON window */
	VGetWindowInformation(butt,
					&dum,&dum,
					&bw,&bh,	/* image window size */
					&bfw, &bfh,		/* font size */
					&fg,&bg);		/* back. and foreg. color cells */

	/* Get the reserved colors Structure */
	VGetReservedColors(color);

	/* copy the filename into a local global variable for later use (when interpolating) */
	strcpy(filename, file);

	/* Get the visual class of the display */
	/*VGetVisual(&visual);*/
    	XSetFunction(display, iigc, GXcopy);



	/* Read the input file */

	/* Open input file */
    if((fpin = fopen(file,"rb")) == NULL)
    {
	VDisplayDialogMessage("ERROR: CAN'T OPEN INPUT FILE !");
	XBell(display, 0);
	VRedisplayMenu();
        return(-1);
    }

	/* Read the header */
	VReadHeader(fpin,&vh, group, element);

	compute_slices(&vh, &sl);

	sd = sl.sd;
	tsd = sd;


	H = ih - dh;
	W = iw;

	size = 90;	    /* size of the base of the slice icon (in pixels) */
	D3 = (int) (0.7*H); /* height of the drawing area (in the image window) for the slice icons */
	D4 = (int) (0.7*W); /* widht "    "    "      "            "           "    "    "     "*/

	/* Check SIZE of slice against width of screen */
	if( (2*size*sl.volumes) > D4)
		size = D4 / (2*sl.volumes);
	if(  (D3/sl.max_slices) > (size/2) )
		D3 = (size * sl.max_slices / 3) - 4*ifh;

	/* spacing along X3 can't be smaller than 3 pixels */

	/* if step in X4 is "greater" than 2*'size', make X4 = 2*size  */


	/* ORIGIN of DRAWING AREA */
	OX = 5*ifw;
	OY = 4*ifh;

	/* Origin for HORIZONTAL AXIS (X4) */
	HScalex = OX + 6*ifw + size/2;
	HScaley = OY + ifh - size/3 ;

	/* Origin for VERTICAL AXIS (X3) */
	VScalex = OX;
	VScaley = OY + 3*ifh;

	/* End-Points for TICS on VERTICAL AXIS (X3) */
	HTicx1 = OX - ifw/2;
	HTicx2 = OX + ifw/2;

	/* End-Points for Larger TICS on VERTICAL AXIS (X3) */
	HBigTicx1 = OX - ifw;
	HBigTicx2 = OX + ifw;

	/* Horizontal Location of LABELS on VERTICAL AXIS (X3) */
	HIndexx = OX - 4.5*ifw;
	HLocx   = OX + 3*ifw/2;

	/* End-Points for TICS on HORIZONTAL AXIS (X3) */
	VTicy1 = HScaley - ifh/2;
	VTicy2 = HScaley + ifh/2;

	/* Vertical Location of LABELS on HRIZONTAL AXIS (X4) */
	VIndexy = VTicy1 - 3*ifh/2;
	VLocy   = VTicy2 + ifh/2;



	/* Allocate space for the TEXt_ITEM IDs */
	min3 = (TEXT **) malloc( sl.volumes*sizeof(TEXT *) );
	max3 = (TEXT **) malloc( sl.volumes*sizeof(TEXT *) );
	

	/* DRAW SLICE ICONS FROM BOTTOM to TOP */
	for(j=0; j<sl.volumes; j++)
	{
		if(j == 0)
			posx = OX + 6*ifw;
		else
			posx = OX + 6*ifw + (int)((sl.location4[j]-sl.location4[0]) * D4 /
					(sl.location4[sl.volumes-1]-sl.location4[0]) );
		posy = VScaley + D3;

		/* Draw HORIZONTAL LABELS */
		if(sl.volumes > 1)
		{
			sprintf(text, "%.2f", sl.location4[j]);
			VDisplayImageMessage(img, text, 1, posx + size/2 - ifw/2, VLocy);
			sprintf(text, "%d", j+1);
			VDisplayImageMessage(img, text, 1, posx + size/2 - ifw/2, VIndexy);
		}


		/* Traverse the VOLUME in reverse order (from bottom to top) */
		for(i = sl.slices[j]-1; i>=0; i--)
		{

			/* If just one slice */
			if(sl.slices[j] == 1) 
				posy = VScaley;
			else
				posy = VScaley + (int)((sl.location3[j][i]-sl.Min_location3) * ((float)D3) / sl.Fov3);
					
			cy = posy;

			/* Draw VERTICAL TICS */
			if(sd > 3)
				XDrawLine(display, img, iigc, posx+size/2, VTicy1, posx+size/2, VTicy2);

			/* Draw horizontal tics only for the FIRST volume */
			if(j == 0)
			{
				/* Draw HORIZONTAL TICS  */
				XDrawLine(display, img, iigc,  HTicx1, cy, HTicx2, cy);

			}

			/* Draw VERTICAL LABELS (if applicable) */
			if(j == 0  &&  ( i%5 == 0  /*||  sD3 > 1.2*ifh*/)   )
			{
				/* Slice LOCATION (-3 fonts) */
				sprintf(text,"%.1f", sl.location3[j][i]);
				VDisplayImageMessage(img, text, 1, HLocx, cy - ifh/2 );

				/* Slice INDEX  (-9 fonts) */
				sprintf(text,"%d", i+1);
				VDisplayImageMessage(img, text, 1,  HIndexx, cy - ifh/2);

				/* Draw BIG HORIZONTAL TIC (from -6 to -3 fonts) */
				XDrawLine(display, img, iigc, HBigTicx1, cy, HBigTicx2, cy);

			}


			/* Draw SLICE ICON */
			draw_slice(iigc, posx, posy, size, 1);

		}

		/* Draw the LABELS on the BASE of the pile (min,max for each Volume) */
		/*posx = OX + 6*ifw ;*/
		cy = VScaley + D3+ifh;
		sprintf(text, "%d", 1);
		VDisplayTablet(img, posx+5, cy , 10, "min:", text, 0, &(min3[j]));
		sprintf(text, "%d", sl.slices[j]);
		VDisplayTablet(img, posx+5, cy+3*ifh/2+4 , 10,"max:", text, 0, &(max3[j]));
	}

	/* Draw AXIS */
	XDrawLine(display,img,iigc, VScalex, VScaley, VScalex, VScaley+D3);
	if(sd == 4)
		XDrawLine(display,img,iigc, HScalex, HScaley, HScalex + D4, HScaley);

	/* Draw Min,Max LABELS for X3 and X4 */
	/* X3 */
	sprintf(text, "%d", sl.slices[0]);
	VDisplayImageMessage(img, text, 1, HIndexx, VScaley+D3+ifh);
	sprintf(text,"%.2f", sl.location3[0][ sl.slices[0]-1 ]);
	VDisplayImageMessage(img, text, 1, HLocx, VScaley+D3+ifh);

	/* X4 */
	if(sd == 4)
	{
		sprintf(text, "%d", 1);
		VDisplayTablet(img,   HScalex+D4+ifw, HScaley-2*ifh, 10, "min:", text, 0, &min4);
		sprintf(text, "%d", sl.volumes);
		VDisplayTablet(img,  HScalex+D4+ifw, HScaley-ifh/2, 10, "max:", text, 0, &max4);
	}



	/* TEXT ITEMS on the DIALOG Window */
	sprintf(text,"X1 - (%.3f) : ", vh.scn.xypixsz[0]);
	sprintf(text2,"%.5f", vh.scn.xypixsz[0]);
	VDisplayTablet(dial, 5*dfw, 3*dfh, 30, text, text2, 0, &x1);
	sprintf(text,"X2 - (%.3f) : ", vh.scn.xypixsz[1]);
	sprintf(text2,"%.5f", vh.scn.xypixsz[1]);
	VDisplayTablet(dial, 5*dfw, 5*dfh, 30, text, text2, 0, &x2);
	sprintf(text,"X3 - (%.3f/%.3f) : ",  vh.scn.xypixsz[0], sl.Max_spacing3);
	sprintf(text2,"%.5f",  vh.scn.xypixsz[0]);
	VDisplayTablet(dial, 5*dfw, 7*dfh, 30, text, text2, 0, &x3);
	if(sd == 4)
	{
		sprintf(text,"X4 - (%.3f/%.3f) : ", sl.min_spacing4, sl.max_spacing4);
		sprintf(text2,"%.5f", sl.min_spacing4);
		VDisplayTablet(dial, 5*dfw, 9*dfh, 30, text, text2, 0, &x4);
	}
	original_res =  vh.scn.xypixsz[0];

	/* TEXT ITEM on the BUTTON Window */

	VDisplayRunModeCommand();

	/* HOST */
	/*VDisplayTablet(butt, 15*bfw, bh-7*bfh, 40, "Host : ", "local", 0, &host);*/
 


	/* OUTPUT */
	memset((char *) filenames, '\0', sizeof(filenames));
    	strcpy(filenames[0] , "intrpl-tmp");
    	VDisplayOutputFilePrompt(NULL, filenames, generate_interpolated_scene);
 

	/* SAVE SCREEN */
    	VDisplaySaveScreenCommand();
 

	for(i=0; i<4; i++) method[i] = 0;


	VSetTabletCaret("|");

	set_INTERPOLATE_button_panel();

	VDisplayButtonAction("SELECT","","QUIT");

	/* while(check_button_occurance(RIGHT_BUTTON) == 0);*/
  	/******************************************************/
  	/*            EVENT HANDLER           				*/
  	/******************************************************/
	/*XGrabKeyboard(display, img, True, GrabModeAsync, GrabModeAsync, CurrentTime);*/
	quit = FALSE;
  	while (quit == FALSE)
  	{
    		VNextEvent(&event);
 
    		win=event.xany.window;
 
		/* check for MOUSE and KEYBOARD events related to TEXT ITEMS */
        	quit = HandleInterpolateEvent(&event);

		/* NEW BUTTON WINDOW ITEMS */
        	if(win == butt)
            		VCheckEventsInButtonWindow(&event, redisplay_slices);
		else
		/* Check for TEXT-ITEMs and BUTTONs */
		{
			VCheckTabletEvent(&event);
			/*VCheckButtonItemEvent(&event);*/
		}



		/* check for Button Panel events */
		if(win == dial)
			quit = HandleButtonEvent(&event);
 
  	} /* endwhile !quit */
	/*XUngrabKeyboard(display, CurrentTime);*/
  	/******************************************************/
  	/*            END EVENT HANDLER       				*/
  	/******************************************************/
 



	VDeleteTabletList();
	VDeleteButtonItemList();

	/* Free GCs */
    	XFreeGC(display, iigc);
    	XFreeGC(display, idgc);
    	XFreeGC(display, ibgc);
 

	/* REMOVE NEW BUTTON PANEL ITEMS */
	VDeleteRunModeCommand();
    	VDeleteSaveSreenCommand();
    	VDeleteOutputFilePrompt();
 


	/* Clear BUTTONs Area */
	VDeletePanel();

	VClearWindow(img,0,0,0,0);
	VClearWindow(dial,0,0,0,0);

	VDisplayButtonAction("SELECT","","");

	return(1);

}

/*-----------------------------------------------------------------------------------*/
redisplay_slices()
{
	int i, j;
	int sd;
	int	cy;		/* current y location (for general use) */
	int posx, posy;		/* position of the slices (in pixels on the screen */
	int	size;			/* size of the base of the slice icon in pixels */
	char text[100];
	int D3, D4;		/* length axis along x3 and x4 dimensions */


	sd = sl.sd;

	H = ih - dh;
	W = iw;

	size = 90;	    /* size of the base of the slice icon (in pixels) */
	D3 = (int) (0.7*H); /* height of the drawing area (in the image window) for the slice icons */
	D4 = (int) (0.7*W); /* widht "    "    "      "            "           "    "    "     "*/

	/* Check SIZE of slice against width of screen */
	if( (2*size*sl.volumes) > D4)
		size = D4 / (2*sl.volumes);
	if(  (D3/sl.max_slices) > (size/2) )
		D3 = (size * sl.max_slices / 3) - 4*ifh;

	/* spacing along X3 can't be smaller than 3 pixels */

	/* if step in X4 is "greater" than 2*'size', make X4 = 2*size  */


	/* ORIGIN of DRAWING AREA */
	OX = 5*ifw;
	OY = 4*ifh;

	/* Origin for HORIZONTAL AXIS (X4) */
	HScalex = OX + 6*ifw + size/2;
	HScaley = OY + ifh - size/3 ;

	/* Origin for VERTICAL AXIS (X3) */
	VScalex = OX;
	VScaley = OY + 3*ifh;

	/* End-Points for TICS on VERTICAL AXIS (X3) */
	HTicx1 = OX - ifw/2;
	HTicx2 = OX + ifw/2;

	/* End-Points for Larger TICS on VERTICAL AXIS (X3) */
	HBigTicx1 = OX - ifw;
	HBigTicx2 = OX + ifw;

	/* Horizontal Location of LABELS on VERTICAL AXIS (X3) */
	HIndexx = OX - 4.5*ifw;
	HLocx   = OX + 3*ifw/2;

	/* End-Points for TICS on HORIZONTAL AXIS (X3) */
	VTicy1 = HScaley - ifh/2;
	VTicy2 = HScaley + ifh/2;

	/* Vertical Location of LABELS on HRIZONTAL AXIS (X4) */
	VIndexy = VTicy1 - 3*ifh/2;
	VLocy   = VTicy2 + ifh/2;



	/* DRAW SLICE ICONS FROM BOTTOM to TOP */
	for(j=0; j<sl.volumes; j++)
	{
		if(j == 0)
			posx = OX + 6*ifw;
		else
			posx = OX + 6*ifw + (int)((sl.location4[j]-sl.location4[0]) * D4 /
					(sl.location4[sl.volumes-1]-sl.location4[0]) );
		posy = VScaley + D3;

		/* Draw HORIZONTAL LABELS */
		if(sl.volumes > 1)
		{
			sprintf(text, "%.2f", sl.location4[j]);
			VDisplayImageMessage(img, text, 1, posx + size/2 - ifw/2, VLocy);
			sprintf(text, "%d", j+1);
			VDisplayImageMessage(img, text, 1, posx + size/2 - ifw/2, VIndexy);
		}


		/* Traverse the VOLUME in reverse order (from bottom to top) */
		for(i = sl.slices[j]-1; i>=0; i--)
		{
			/* If just one slice */
			if(sl.slices[j] == 1) 
				posy = VScaley;
			else
				posy = VScaley + (int)((sl.location3[j][i]-sl.Min_location3) * ((float)D3) / sl.Fov3);
					
			cy = posy;

			/* Draw VERTICAL TICS */
			if(sd > 3)
				XDrawLine(display, img, iigc, posx+size/2, VTicy1, posx+size/2, VTicy2);

			/* Draw horizontal tics only for the FIRST volume */
			if(j == 0)
			{
				/* Draw HORIZONTAL TICS  */
				XDrawLine(display, img, iigc,  HTicx1, cy, HTicx2, cy);

			}

			/* Draw VERTICAL LABELS (if applicable) */
			if(j == 0  &&  ( i%5 == 0  /*||  sD3 > 1.2*ifh*/)   )
			{
				/* Slice LOCATION (-3 fonts) */
				sprintf(text,"%.1f", sl.location3[j][i]);
				VDisplayImageMessage(img, text, 1, HLocx, cy - ifh/2 );

				/* Slice INDEX  (-9 fonts) */
				sprintf(text,"%d", i+1);
				VDisplayImageMessage(img, text, 1,  HIndexx, cy - ifh/2);

				/* Draw BIG HORIZONTAL TIC (from -6 to -3 fonts) */
				XDrawLine(display, img, iigc, HBigTicx1, cy, HBigTicx2, cy);

			}


			/* Draw SLICE ICON */
			draw_slice(iigc, posx, posy, size, 1);

		}

		/* Draw the LABELS on the BASE of the pile (min,max for each Volume) */
		/*posx = OX + 6*ifw ;*/
		cy = VScaley + D3+ifh;
		v_DisplayText(min3[j]);
		v_DisplayText(max3[j]);
	}

	/* Draw AXIS */
	XDrawLine(display,img,iigc, VScalex, VScaley, VScalex, VScaley+D3);
	if(sd == 4)
		XDrawLine(display,img,iigc, HScalex, HScaley, HScalex + D4, HScaley);

	/* Draw Min,Max LABELS for X3 and X4 */
	/* X3 */
	sprintf(text, "%d", sl.slices[0]);
	VDisplayImageMessage(img, text, 1, HIndexx, VScaley+D3+ifh);
	sprintf(text,"%.2f", sl.location3[0][ sl.slices[0]-1 ]);
	VDisplayImageMessage(img, text, 1, HLocx, VScaley+D3+ifh);

	/* X4 */
	if(sd == 4)
	{
		v_DisplayText(min4);
		v_DisplayText(max4);
	}

}
/*-----------------------------------------------------------------------------------*/
abort_interpolation()
{
	abort_flag = 1;
}

/*-----------------------------------------------------------------------------------*/
/**************************************************************************
 *    Modified: 2/10/95 check if output same as input file by Dewey Odhner
 *    Modified: 3/13/96 shape_interp_cb not called by Dewey Odhner
 *    Modified: 9/6/02 z-extrapolation enabled by Dewey Odhner
 *    Modified: 3/4/03 initial, final conversions fixed by Dewey Odhner
 **************************************************************************/
generate_interpolated_scene()
{
	char text[1000], text2[400];
	int i;
	float aa, bb;
	float initial, final;
	char t0[200],
		t1[30],
		t2[30],
		t3[30],
		t4[30],
		t5[30],
		t6[30],
		t7[30],
		t8[30];
	float xx1, xx2, xx3;

	char output_file[300];
	int back_flag;

	FILE *fparg;
	int arg_file_flag=0; /* indicate if argument file will be used (in case of too may arguments) */


	/* Get BACKGROUND mode  (0=OFF  1=ON) */
	/*VGetButtonItemMode(background, &back_flag);*/
	VGetRunModeValue(&back_flag);


	/* Get CURRENT_DIRECTORY */
	/*strcpy( current_directory, (char *)getenv("PWD"));*/

	/* Get OUTPUT FIle Name */
	/*VGetTabletValue(output, t0);*/
	VGetSaveFilename(t0);

	/* BINARY OUTPUT */
	if(  sl.bits == 1)
		sprintf(output_file, "%s.BIM", t0);
	else
	/* GREY OUTPUT */
		sprintf(output_file, "%s.IM0", t0);


	/* If input and output filenames are the same */
	if(strcmp(filename,output_file) == 0)
	{
		VDisplayDialogMessage("INPUT AND OUTPUT FILES CAN'T HAVE THE SAME NAME. CHANGE OUTPUT NAME.");
		XBell(display, 0);
		XFlush(display);
		sleep(5);
		VDisplayDialogMessage("");
    	VDisplayButtonAction("SELECT","","QUIT");
    	VSelectCursor(ALL_WINDOWS, DEFAULT_CURSOR);
    	XFlush(display);
		return;
	}

	if(sl.sd==3)
	{
		VGetTabletValue( x1,  t1);
		VGetTabletValue( x2 , t2);
		VGetTabletValue( x3 , t3);
		if(strlen(t1) == 0) strcpy(t1, "0");
		if(strlen(t2) == 0) strcpy(t2, "0");
		if(strlen(t3) == 0) strcpy(t3, "0");
		sscanf(t1, "%f", &xx1);
		sscanf(t2, "%f", &xx2);
		sscanf(t3, "%f", &xx3);
		VGetTabletValue( min3[0], t4);
		VGetTabletValue( max3[0], t5);
		sscanf(t4, "%f", &initial);
		sscanf(t5, "%f", &final);
		if (z_extrapolate_flag)
		{
			initial = .5;
			final = sl.slices[0]+.5;
		}
		sprintf(text,"ndinterpolate  %s %s  %d   %s %s %s   %d    %d %d %d    %f %f    ", 
				filename, output_file, 
				back_flag, 
				t1, t2, t3,
				current_distance, 
				method[0], method[1], method[2],
				initial-1, final-1);
		sprintf(t6,"%.3f",  vh.scn.xypixsz[0]);

/* removed 3/13/96:
		/* CHECK IF OUTPUT IS ISOTROPIC 3D *\/
		if( sl.bits == 1 &&				/* binary scene *\/
			current_distance == 0 && 	/* distance method is CITY BLOCK *\/
			method[2] == 1  &&			/* interp. method along Oz is LINEAR *\/
			xx1 == original_res &&  	/* output Ox sampling is equal to input Ox sampling *\/
			xx1 == xx2 && 				/* sampling scheme along all 3 directions is the same *\/
			xx1 == xx3 && 
			xx2 == xx3)
			sprintf(text, "shape_interp_cb %s %s %d", filename, output_file, back_flag);
*/
	}
	else
	if(sl.sd==4)
	{
		VGetTabletValue( x1,  t1);
		VGetTabletValue( x2 , t2);
		VGetTabletValue( x3 , t3);
		VGetTabletValue( x4 , t4);
		if(strlen(t1) == 0) strcpy(t1, "0");
		if(strlen(t2) == 0) strcpy(t2, "0");
		if(strlen(t3) == 0) strcpy(t3, "0");
		if(strlen(t4) == 0) strcpy(t4, "0");
		VGetTabletValue( min4 , t5 );
		VGetTabletValue( max4  , t6);
		sscanf(t5, "%f", &initial);
		sscanf(t6, "%f", &final);
		sprintf(text,"ndinterpolate  %s %s   %d   %s %s %s %s   %d    %d %d %d %d    %.0f %.0f    ", 
				filename, output_file, 
				back_flag, 
				t1, t2, t3, t4,
				current_distance, 
				method[0], method[1], method[2], method[3],
				initial-1, final-1);

		for(i=initial-1; i<final; i++)
		{
			VGetTabletValue(min3[i], t7);
			VGetTabletValue(max3[i], t8);
			sscanf(t7, "%f", &aa);
			sscanf(t8, "%f", &bb);
			if (z_extrapolate_flag)
			{
				aa = .5;
				bb = sl.slices[i]+.5;
			}
			sprintf(text2,"%f %f  ", aa-1, bb-1 );
			strcat(text, text2);
		}

		/* If more than 20 volumes, then use the ARGUMENT FILE */
		if(sl.volumes > 20)
		{
			arg_file_flag = 1;
			sprintf(text,"ndinterpolate  %s %s args", filename, output_file);
			if( (fparg = fopen("args", "wb")) == NULL)
			{
				VDisplayDialogMessage("CAN'T OPEN ARGUMENT FILE. FILE SYSTEM MAY BE FULL. SORRY, CAN'T CONTINUE.");
				XBell(display, 0);
				sleep(5);
				VDisplayButtonAction("SELECT","","QUIT");
				XFlush(display);
				VSelectCursor(ALL_WINDOWS, DEFAULT_CURSOR);
				VDisplayStatus(READY);
				return;
			}
			fprintf(fparg, "%d\n", back_flag);
			fprintf(fparg, "%s\n", t1);
			fprintf(fparg, "%s\n", t2);
			fprintf(fparg, "%s\n", t3);
			fprintf(fparg, "%s\n", t4);
			fprintf(fparg, "%d\n", current_distance);
			fprintf(fparg, "%d\n", method[0]);
			fprintf(fparg, "%d\n", method[1]);
			fprintf(fparg, "%d\n", method[2]);
			fprintf(fparg, "%d\n", method[3]);
			fprintf(fparg, "%.0f\n", initial-1);
			fprintf(fparg, "%.0f\n", final-1);
			for(i=initial-1; i<final; i++)
        	{
				VGetTabletValue(min3[i], t7);
				VGetTabletValue(max3[i], t8);
				sscanf(t7, "%f", &aa);
				sscanf(t8, "%f", &bb);
				if (z_extrapolate_flag)
				{
					aa = .5;
					bb = sl.slices[i]+.5;
				}
				fprintf(fparg, "%f\n", aa-1);
				fprintf(fparg, "%f\n", bb-1);
			}
			fclose(fparg);
			
		}
		
	}

	VDisplayDialogMessage("Interpolating. Wait ...");
	VSelectCursor(ALL_WINDOWS, XC_watch);
	VDisplayStatus(WAIT);
	XFlush(display);

	printf("[%s]\n", text);

	i = VCallProcess(text, back_flag, "local", "", "Interpolate");
	if(i==400)
	{
		VDisplayDialogMessage("CAN'T EXECUTE INTERPOLATION PROCESS !");
		XBell(display, 0);
		XFlush(display);
		sleep(3);
		VDisplayDialogMessage("");
	}
	else
	if(i==401)
	{
		VDisplayDialogMessage("INTERPOLATION WAS ABORTED.");
		XBell(display, 0);
		XFlush(display);
	}

	/* If argument file was used, delete it */
	if(arg_file_flag == 1)
		unlink("args");

	VDisplayButtonAction("SELECT","","QUIT");
	XFlush(display);
	VSelectCursor(ALL_WINDOWS, DEFAULT_CURSOR);
	VDisplayStatus(READY);
	
}

/*-----------------------------------------------------------------------------------*/
HandleInterpolateEvent(event)
XEvent *event;
{

	int type,
		button;


	type = event->type;
    button = event->xbutton.button;


	/* If RIGHT-BUTTON */
	if( type == ButtonPress  &&  button == RIGHT_BUTTON)
		return(TRUE);


	return(FALSE);
}

/*-----------------------------------------------------------------------------------*/
set_INTERPOLATE_button_panel()
{
	int group = 0;
	num_cmds = 0;


    /*. . . . . . . . . . . . . . . . . . . . . . . . . */
    /* GROUP 0 */


	/* METHOD SWITCH */ 
    cmds[num_cmds].group= group;
    cmds[num_cmds].type = 1;
    strcpy(cmds[num_cmds].cmd,"x1");
	cmds[num_cmds].num_of_switches = 3;
	cmds[num_cmds].switches = (Char30 *) malloc(sizeof(Char30)*cmds[num_cmds].num_of_switches);
	strcpy(cmds[num_cmds].switches[0], "N.N.");
	strcpy(cmds[num_cmds].switches[1], "LINR");
	strcpy(cmds[num_cmds].switches[2], "CUB");
    cmd_func[num_cmds] = methodx1_switchboard;
    num_cmds++;

    cmds[num_cmds].group= group;
    cmds[num_cmds].type = 1;
    strcpy(cmds[num_cmds].cmd,"x2");
	cmds[num_cmds].num_of_switches = 3;
	cmds[num_cmds].switches = (Char30 *) malloc(sizeof(Char30)*cmds[num_cmds].num_of_switches);
	strcpy(cmds[num_cmds].switches[0], "N.N.");
	strcpy(cmds[num_cmds].switches[1], "LINR");
	strcpy(cmds[num_cmds].switches[2], "CUB");
    cmd_func[num_cmds] = methodx2_switchboard;
    num_cmds++;
 
    cmds[num_cmds].group= group;
    cmds[num_cmds].type = 1;
    strcpy(cmds[num_cmds].cmd,"x3");
	cmds[num_cmds].num_of_switches = 3;
	cmds[num_cmds].switches = (Char30 *) malloc(sizeof(Char30)*cmds[num_cmds].num_of_switches);
	strcpy(cmds[num_cmds].switches[0], "N.N.");
	strcpy(cmds[num_cmds].switches[1], "LINR");
	strcpy(cmds[num_cmds].switches[2], "CUB");
    cmd_func[num_cmds] = methodx3_switchboard;
    num_cmds++;
 
	if(sd == 4)
	{
    cmds[num_cmds].group= group;
    cmds[num_cmds].type = 1;
    strcpy(cmds[num_cmds].cmd,"x4");
	cmds[num_cmds].num_of_switches = 3;
	cmds[num_cmds].switches = (Char30 *) malloc(sizeof(Char30)*cmds[num_cmds].num_of_switches);
	strcpy(cmds[num_cmds].switches[0], "N.N.");
	strcpy(cmds[num_cmds].switches[1], "LINR");
	strcpy(cmds[num_cmds].switches[2], "CUB");
    cmd_func[num_cmds] = methodx4_switchboard;
    num_cmds++;
	}

	/* DISTANCE METH0D SWITCH (For BINARY scenes only) */
	if(  vh.scn.bit_fields[1] - vh.scn.bit_fields[0] == 0)
	{
    	cmds[num_cmds].group= group;
    	cmds[num_cmds].type = 1;
    	strcpy(cmds[num_cmds].cmd,"DIST");
		cmds[num_cmds].num_of_switches = 2;
		cmds[num_cmds].switches = (Char30 *) malloc(sizeof(Char30)*cmds[num_cmds].num_of_switches);
		strcpy(cmds[num_cmds].switches[0], "CITY");
		strcpy(cmds[num_cmds].switches[1], "CHAM");
    	cmd_func[num_cmds] = distance_switchboard;
    	num_cmds++;
	}

	/* Z_EXTRAPOLATE SWITCH (For BINARY scenes only) */
	if(  vh.scn.bit_fields[1] - vh.scn.bit_fields[0] == 0)
	{
    	cmds[num_cmds].group= group;
    	cmds[num_cmds].type = 1;
    	strcpy(cmds[num_cmds].cmd,"EXTRAP");
		cmds[num_cmds].num_of_switches = 2;
		cmds[num_cmds].switches = (Char30 *) malloc(sizeof(Char30)*cmds[num_cmds].num_of_switches);
		strcpy(cmds[num_cmds].switches[0], "NO");
		strcpy(cmds[num_cmds].switches[1], "YES");
    	cmd_func[num_cmds] = z_extrapolate_switchboard;
    	num_cmds++;
	}

	VDisplayPanel(num_cmds, cmds);

}


/*-----------------------------------------------------------------------------------*/
/***************************************************************************
 *    Modified: 3/12/96 correct switch number sought by Dewey Odhner
 ***************************************************************************/
methodx1_switchboard(value)
char *value;
{
	int i;
	int sn;

	for (sn=0; sn<num_cmds; sn++)
		if (strcmp(cmds[sn].cmd, "x1") == 0)
			break;
	assert(sn < num_cmds);
	current_dimension = 0;

	for(i=0; i<cmds[sn].num_of_switches; i++)
	{
		if( !strncmp(value, cmds[sn].switches[i], 3) )
			current_method = i;
	}
	
	method[current_dimension] = current_method;
}
/***************************************************************************
 *    Modified: 3/12/96 correct switch number sought by Dewey Odhner
 ***************************************************************************/
methodx2_switchboard(value)
char *value;
{
	int i;
	int sn;

	for (sn=0; sn<num_cmds; sn++)
		if (strcmp(cmds[sn].cmd, "x2") == 0)
			break;
	assert(sn < num_cmds);
	current_dimension = 1;
	for(i=0; i<cmds[sn].num_of_switches; i++)
	{
		if( !strncmp(value, cmds[sn].switches[i], 3) )
			current_method = i;
	}
	
	method[current_dimension] = current_method;
}
/***************************************************************************
 *    Modified: 3/12/96 correct switch number sought by Dewey Odhner
 ***************************************************************************/
methodx3_switchboard(value)
char *value;
{
	int i;
	int sn;

	for (sn=0; sn<num_cmds; sn++)
		if (strcmp(cmds[sn].cmd, "x3") == 0)
			break;
	assert(sn < num_cmds);
	current_dimension = 2;
	for(i=0; i<cmds[sn].num_of_switches; i++)
	{
		if( !strncmp(value, cmds[sn].switches[i], 3) )
			current_method = i;
	}
	
	method[current_dimension] = current_method;
}
/***************************************************************************
 *    Modified: 3/12/96 correct switch number sought by Dewey Odhner
 ***************************************************************************/
methodx4_switchboard(value)
char *value;
{
	int i;
	int sn;

	for (sn=0; sn<num_cmds; sn++)
		if (strcmp(cmds[sn].cmd, "x4") == 0)
			break;
	assert(sn < num_cmds);
	current_dimension = 3;
	for(i=0; i<cmds[sn].num_of_switches; i++)
	{
		if( !strncmp(value, cmds[sn].switches[i], 3) )
			current_method = i;
	}
	
	method[current_dimension] = current_method;
}




/*-----------------------------------------------------------------------------------*/
/***************************************************************************
 *    Modified: 3/12/96 correct switch number sought by Dewey Odhner
 ***************************************************************************/
distance_switchboard(value)
char *value;
{
	int i;
	int sn;

	for (sn=0; sn<num_cmds; sn++)
		if (strcmp(cmds[sn].cmd, "DIST") == 0)
			break;
	assert(sn < num_cmds);
	for(i=0; i<cmds[sn].num_of_switches; i++)
	{
		if( !strcmp(value, cmds[sn].switches[i]) )
			current_distance = i;  /* 0=city block, 1=chamfer */
	}
	
}

/*****************************************************************************
 * FUNCTION: z_extrapolate_switchboard
 * DESCRIPTION: Sets the variable z_extrapolate_flag according to parameter
 *    value.
 * PARAMETERS:
 *    value: "YES" or "NO"
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 9/6/02 by Dewey Odhner
 *
 *****************************************************************************/
z_extrapolate_switchboard(value)
char *value;
{
	z_extrapolate_flag = strcmp(value, "YES")==0;
}


/*-----------------------------------------------------------------------------------*/
toggle_fore_back_proc()
{

	int state;

	VGetButtonItemMode(background, &state);

	/* If Background is ON, Turn Host Text ON */
	if(state == 1)
	{
/*
		VSetTabletValue(host, "local");
		VDisplayText(host);
*/
		VSetButtonItemLabel(background, "Background");
	}
	/* If Background is OFF, Turn Host Text OFF */
	else
	{
/*
		VSetTabletValue(host, "");
		VRedisplayTablet(host);
*/
		VSetButtonItemLabel(background, "Foreground");
	}
	
}


/*-----------------------------------------------------------------------------------*/
HandleButtonEvent(event)
XEvent *event;
{
	 int i,error;
  	char sel_cmd[80],sel_swt[80];
 
 
	switch (event->type)
  	{
  		/* GETTING INTO THE WINDOW */
  		case EnterNotify:
    		error = VDisplayButtonAction("SELECT","","QUIT");
    		if (error) handle_error("VDisplayButtonAction","HandleEvent_dial",error,0);
    		break;
 
  		/* BUTTON PRESS */
  		case ButtonPress:
			/* call function that based on Event, gives back Function to call */
    		if (!VCheckPanelEvent(event,sel_cmd,sel_swt))
    		{
				/* Check WHICH BUTTON was pressed */
      			for(i=0;i<num_cmds;i++)
				{
					/* if BUTTON was pressed */
    				if (!strcmp(sel_cmd,cmds[i].cmd))
    				{
						/* If PLAIN BUTTON */
      					if (cmds[i].type==0)
      					{
        					if (cmd_func[i] != NULL)  (cmd_func[i])();
      					}
      					else 
						/* If SWITCH */
						if (cmd_func[i] != NULL) 
						{
							(cmd_func[i])(sel_swt);
						}
      					break;
    				}
				}
    		}
    		break;
  	}
 
  return(FALSE);
 
}


/*-----------------------------------------------------------------------------------*/
/* Modified: 3/12/97 parameter gc declared by Dewey Odhner. */
draw_slice(gc, x, y, size, type)
GC gc;
int 	x, y, 	/* location of the lower-left corner of slice */
		size, 	/* size of the base of the slice in pixels */
		type;	/* 0=45deg.angle,  1=~ 30deg. angle */
{
	int x1,x2,x3,x4;
	int y1,y2,y3,y4;
	XPoint p[4];

	p[0].x = x1=x; 
	p[0].y = y1=y;
	p[1].x = x2 = x+size; 
	p[1].y = y2 = y;
	if(type == 0)
	{
		p[2].x = x3 = x2+size/2+1;
		p[2].y = y3 = y-size/2-1;
	}
	else
	{
		p[2].x = x3 = x2+size/2+1;
		p[2].y = y3 = y-size/3-1;
	}
	p[3].x = x4 = x3-size;
	p[3].y = y4 = y3;

	/* Set FOREGROUND Color */
	XSetForeground(display, gc, color[0].pixel);

	/* Draw the outside polygon */
	XFillPolygon(display, img, gc, p, 4, Convex, CoordModeOrigin);



	/* specify inside of polygon */
	if(type == 0)
	{
		p[0].x = x1+2;
		p[0].y = y1-1;
		p[1].x = x2;
		p[1].y = y2-1;
		p[2].x = x3-2;
		p[2].y = y3+1;
		p[3].x = x4;
		p[3].y = y4+1;
	}
	else
	{
		p[0].x = x1+3;
		p[0].y = y1-1;
		p[1].x = x2;
		p[1].y = y2-1;
		p[2].x = x3-3;
		p[2].y = y3+1;
		p[3].x = x4;
		p[3].y = y4+1;
	}
	



	/* Set FOREGROUND Color */
	XSetForeground(display, gc, color[2].pixel);

	/* INVERT the INSIDE of the Polygon */
	XFillPolygon(display, img, gc, p, 4, Convex, CoordModeOrigin);




	/* Set FOREGROUND Color */
	XSetForeground(display, gc, color[0].pixel);

}
