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
#include "voi.h"
#include <string.h>

/* GLOBAL VARIABLES */
Display *display ; /* a pointer to the Display structure */
long screen ; /* screen identification */
Window img, dial, butt ; /* window identifications */

Scene_Info *scene;
int num_scenes=0;
int error;
int num_files=0;


FileInfo *files;


/*-------------------------------------------------------------------------*/
clear_file_structure(files)
FileInfo *files;
{
   int i;

  if(files != NULL)
  {
	for(i=0; i<num_files; i++)
	{
	   	free(files[i].min);
	   	free(files[i].max);
	   	free(files[i].incr);
	}
	free(files);
  }
}	


/*-------------------------------------------------------------------------*/
get_file_info()
{
  char *exten;
  int len,i;
  char group[5], element[5];
  char name[300];
  

  /* Get the files chosen by the INPUT call*/
  if(VGetInputFiles(&files, &num_files, SINGLE_FILE, (IM0 | BIM | MV0) ) != 0)
	return(-1);


  num_scenes = num_files;

  if(num_files == 0)
  {
	clear_file_structure(files);
	return(0);
  }

  /* Allocate space for scene structure */
  if(scene == NULL)
  {
     if ((scene=(Scene_Info *)malloc(sizeof(Scene_Info)))==NULL) 
     {
    	fprintf(stderr,"Could not build scene data structure\n");
     	clear_file_structure(files);
    	exit(-1);
     }
  }

  /* copy filename read from the COLOR.COM file into the structure */
  strcpy(scene[0].file_name, files[0].filename);

  if ((scene[0].fp=fopen(scene[0].file_name,"rb"))==NULL) 
  {
     fprintf(stderr,"Could not open %s\n",scene[0].file_name);
     fprintf(stderr,"Ignoring file\n");
     clear_file_structure(files);
     return(-1);
  }


  VReadHeader(scene[0].fp,&scene[0].vh, group, element);
  /*BuildTree(&scene[0],NULL,NULL,NULL);*/
  VGetHeaderLength(scene[0].fp,&len);

  clear_file_structure(files);
  return(num_files);

}
/*-------------------------------------------------------------------------*/
  
main(argc,argv)
int argc;
char *argv[];
{
  char msg[80],*exten; 
  unsigned int OPTION,OLD_OPTION;
  int len,i;
  char group[5], element[5];


  /* Connect to X Server and setup 3DVIEWNIX */
  error=VSetup(argc,argv,0) ;
  handle_error(argv[0],"VOpenServer",error,-1) ;

  /* Set the defualt visual class of the X server for 3DVIEWNIX */
  error=0 ;
  handle_error(argv[0],"VSetVisualClass",error,-1) ;

  /* Get the DISPLAY for X function XNextEvent */
  VGetServerScreenInformation(&display,&screen) ;

  /* Create IMAGE, DIALOG and BUTTON windows */
  error=0 ;
  handle_error(argv[0],"VCreateWindows",error,-1) ;

  /* Get the Window IDs for event handling */
  /* img, dial and butt are global variables */
  VGetWindows(&img,&dial,&butt) ;

  /* Create and Initial;ize colormap to identity with overlay(s) specified */
  /* by function VCreateWindows */
  error=VCreateColormap() ;
  handle_error(argv[0],"VCreateWindows",error,-1) ;


  /* select event(s) for image, dialog, and button windows */
  error=VSelectEvents(img,ButtonPressMask | ExposureMask | OwnerGrabButtonMask) ;
  handle_error(argv[0],"VSelectEvents",error,-1) ;
  error=VSelectEvents(dial,ButtonPressMask | ExposureMask | OwnerGrabButtonMask) ;
  handle_error(argv[0],"VSelectEvents",error,-1) ;
  error=VSelectEvents(butt,ButtonPressMask | ExposureMask | OwnerGrabButtonMask) ;
  handle_error(argv[0],"VSelectEvents",error,-1) ;



  error = VDisplayDialogMessage("LEFT/MIDDLE button to select from menu");
  handle_error("VDisplayDialogMessage","preprocess",error,-1);
  error = VDisplayButtonAction("","","");
  handle_error("VDisplayButtonAction","main",error,0);

      error= VRemoveMenu();
      handle_error("VRemoveMenu","interpolate",error,-1);
      VDisplayDialogMessage("INTERPOLATE ...");
      i = get_file_info();
      if(i <= 0)
      {
      	/*if(i<0) VDisplayDialogMessage("Error in VGetInputFiles(). Can't read GLOBAL.COM !");*/
      	if(i<=0) VDisplayDialogMessage("No file was chosen !");
		XBell(display, 0);
      	VRedisplayMenu();
  		exit(0);
	  }
      setup_image_window(scene[0].file_name);
      error = VRedisplayMenu();
      handle_error("VRedisplayMenu","interpolate",error,-1);

  	exit(0);

}

/*------------------------------------------------------------------------*/

handle_error(process,function,error,opt)
char *process ;
char *function ;
int error ;
int opt ; 
{
        char msg[100] ;
 
	if( error != 0)
	{
           VDecodeError(process,function,error,msg) ;
           printf(msg);
           if (opt == -1) exit(-1) ;
	}
}
 
