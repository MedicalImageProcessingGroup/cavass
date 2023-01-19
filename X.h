
#ifndef X_H
#define X_H

typedef unsigned long XID;
typedef unsigned long Mask;
typedef unsigned long Atom;		/* Also in Xdefs.h */
typedef unsigned long VisualID;
typedef unsigned long Time;

typedef XID Window;
typedef XID Drawable;
#ifndef _XTYPEDEF_FONT
#  define _XTYPEDEF_FONT
typedef XID Font;
#endif
typedef XID Pixmap;
typedef XID Cursor;
typedef XID Colormap;
typedef XID GContext;
typedef XID KeySym;


/*****************************************************************
 * RESERVED RESOURCE AND CONSTANT DEFINITIONS
 *****************************************************************/

#ifndef None
#define None                 0L	/* universal null resource or null atom */
#endif

/***************************************************************** 
 * EVENT DEFINITIONS 
 *****************************************************************/

/* Input Event Masks. Used as event-mask window attribute and as arguments
   to Grab requests.  Not to be confused with event names.  */

#define ButtonPressMask			(1L<<2)  
#define EnterWindowMask			(1L<<4)  
#define PointerMotionMask		(1L<<6)  
#define ExposureMask			(1L<<15) 

/* Event names.  Used in "type" field in XEvent structures.  Not to be
confused with event masks above.  They start from 2 because 0 and 1
are reserved in the protocol for errors and replies. */

#define KeyPress		2
#define ButtonPress		4
#define MotionNotify		6
#define EnterNotify		7
#define Expose			12

/* button names. Used as arguments to GrabButton and as detail in ButtonPress
   and ButtonRelease events.  Not to be confused with button masks above.
   Note that 0 is already defined above as "AnyButton".  */

#define Button1			1
#define Button2			2
#define Button3			3

/* ImageFormat -- PutImage, GetImage */

#define XYBitmap		0	/* depth 1, XYFormat */
#define XYPixmap		1	/* depth == drawable depth */
#define ZPixmap			2	/* depth == drawable depth */

/* Display classes  used in opening the connection 
 * Note that the statically allocated ones are even numbered and the
 * dynamically changeable ones are odd numbered */

#define StaticGray		0
#define GrayScale		1
#define StaticColor		2
#define PseudoColor		3
#define TrueColor		4
#define DirectColor		5


/* Byte order  used in imageByteOrder and bitmapBitOrder */

#define LSBFirst		0
#define MSBFirst		1

#endif /* X_H */
