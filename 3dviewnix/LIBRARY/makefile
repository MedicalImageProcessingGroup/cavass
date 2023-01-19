#
#	makefile for 3DVIEWNIX library functions 
#   	version 1.0     11/19/90     Hsiu-Mei Hung
#	files : colormap.c, data_interf.c, dial_interf.c, image.c 
#		message.c, overlay.c, proc_interf.c, misc.c, text_button.c
#		butt_interf.c, menu_interf.c, globals.c microsleep.c
#





# compiler - (Default - cc)
# =========================
CC = gcc

# 3dviewnix variables  and paths
# ==============================
VIEWNIXINCLUDEDIR = -I$(VIEWNIX_ENV)/INCLUDE
# VIEWNIXLIBD = $(VIEWNIX_ENV)/LIBRARY/libviewnixd.so.1.1
# VIEWNIXLIBG = $(VIEWNIX_ENV)/LIBRARY/libviewnixg.so.1.1
VIEWNIXLIBD = $(VIEWNIX_ENV)/LIBRARY/libviewnixd.a
VIEWNIXLIBG = $(VIEWNIX_ENV)/LIBRARY/libviewnixg.a
VIEWNIXLIB = $(VIEWNIXLIBD) $(VIEWNIXLIBG)

# X11 library path (Default- /usr/lib)
# ====================================
# X11LIBDIR = -L/usr/lib
# X11LIBDIR = -L$(OPENWINHOME)/lib
X11LIBDIR = 

# X11 include path (Default - /usr/include)
# =========================================
# X11INCLUDEDIR = -I/usr/include
# X11INCLUDEDIR = -I$(OPENWINHOME)/include
X11INCLUDEDIR = 

INCLUDEDIRS = $(X11INCLUDEDIR) $(VIEWNIXINCLUDEDIR)

# flags and options passed in 
# ===========================
# CFLAGS = -O $(INCLUDEDIRS)
# CFLAGS = -g $(INCLUDEDIRS) -DFTP_EXPIRATION=199505
CFLAGS = -g $(INCLUDEDIRS)


# Libraries to include 
# ====================
LDFLAGS = $(X11LIBDIR) -lm -lX11




OBJD = globals.o data_interf.o proc_interf.o

OBJG = colormap.o dialog_interf.o image.o message.o \
      overlay.o windows.o menu_interf.o button_interf.o \
      text_button.o events.o help.o info.o annotation.o utils.o

all: $(VIEWNIXLIB)

$(VIEWNIX_ENV)/LIBRARY/libviewnixd.so.1.1 : $(OBJD) 
	-rm libviewnixd.a libviewnixd.so
	ld -o libviewnixd.so.1.1 $(OBJD)
	ln -s libviewnixd.so.1.1 libviewnixd.so

$(VIEWNIX_ENV)/LIBRARY/libviewnixd.a : $(OBJD)
	-rm libviewnixd.so.1.1 libviewnixd.so
	ar cr libviewnixd.a $(OBJD) 
	-ranlib libviewnixd.a


$(VIEWNIX_ENV)/LIBRARY/libviewnixg.so.1.1 : $(OBJG)
	-rm libviewnixg.a libviewnixg.so
	ld -o libviewnixg.so.1.1 $(OBJG)
	ln -s libviewnixg.so.1.1 libviewnixg.so
 
$(VIEWNIX_ENV)/LIBRARY/libviewnixg.a : $(OBJG)
	-rm libviewnixg.so.1.1 libviewnixg.so
	ar cr libviewnixg.a $(OBJG)
	-ranlib libviewnixg.a


globals.o colormap.o data_interf.o dialog_interf.o image.o message.o \
overlay.o proc_interf.o windows.o menu_interf.o button_interf.o \
text_button.o help.o info.o annotation.o utils.o\
  : Vlibrary.h \
    Vlibparam.h \
    Vlibtypedef.h \
    Vlibglobals.h \
    ../INCLUDE/Viewnix.h \
    ../INCLUDE/Vparam.h \
    ../INCLUDE/Vdataheader.h \
    ../INCLUDE/Vtypedef.h

windows.o: icon.xb

events.o :  events.c Vlibrary.h \
    Vlibparam.h \
    Vlibtypedef.h \
    Vlibglobals.h \
    ../INCLUDE/Viewnix.h \
    ../INCLUDE/Vparam.h \
    ../INCLUDE/Vdataheader.h \
    ../INCLUDE/Vtypedef.h
	$(CC) -c $(CFLAGS) events.c || $(CC) -c $(CFLAGS) -DPOLL_NOT_FOUND events.c




clean : 
	-rm -f $(OBJD) $(OBJG)




