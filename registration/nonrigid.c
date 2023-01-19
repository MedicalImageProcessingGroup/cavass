/*
  Copyright 1993-2011, 2017 Medical Image Processing Group
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

//======================================================================
/**
 * \file   nonrigid.c
 * \brief  B-spline based nonrigid registration method implementation
 * \author Xiaofen Zheng, Ph.D.
 * \notes: Currently this nonrigid registration assumes floating image and 
 *   fixed image have the same dimension and voxel size, but it surely is 
 *   extendable easily by making same changes on this code.
 */
//======================================================================  
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include  "Etime.h"
#include  "Viewnix.h"
#include "slices.c"
#include  <time.h>
#ifndef __WXWINCE__  
#include <time.h>  
#endif 

#define DEFAULT_ITERATION 100
#define DEFAULT_STEPSIZE 100
#define DEFAULT_LEVELNUM 7
#define DEFAULT_RBINNUM 32
#define DEFAULT_TBINNUM 32
#define DEFAULT_CONTROLSPACE 15
#define DEFAULT_SUBSAMPLENUM 2048

int VCloseData ( FILE* fp );
int VDeleteBackgroundProcessInformation ( void );
int VGetHeaderLength ( FILE* fp, int* hdrlen );
int VPackByteToBit ( unsigned char* idata, int nbytes, unsigned char* odata );
int VPrintFatalError ( const char function[], const char msg[], int value );
int VReadData ( char* data, int size, int items, FILE* fp, int* items_read );
int VReadHeader ( FILE* fp, ViewnixHeader* vh, char group[5], char element[5] );
int VSeekData ( FILE* fp, long offset );
int VWriteData ( char* data, int size, int items, FILE* fp, int* items_written );
int VWriteHeader ( FILE *fp, ViewnixHeader *vh, char group[5], char element[5] );

float LOG2;

/*-------------------------------------------------------------------------*/
/* Read the file header and its length */
int get_file_info(file, vh, len)
ViewnixHeader *vh;
char *file; /* filename */
int *len;   /* length of header */
{
    char group[5], element[5];
    FILE *fp;
    int error;
 
    /* open file */
    if ((fp=fopen(file,"rb"))==NULL)
    {
        fprintf(stderr,"ERROR: Can't open file [%s] !\n", file);
        VDeleteBackgroundProcessInformation();
        exit(0);
    }
 
 
    error = VReadHeader(fp, vh, group, element);
    if( error>0 && error < 106)
    {
        fprintf(stderr,"ERROR #%d: Inconsistent 3DVIEWNIX header on file [%s] !\n", error, file);
        fprintf(stderr,"group=%s,  element=%s\n", group, element);
        VDeleteBackgroundProcessInformation();
        exit(0);
    }
 
    error = VGetHeaderLength(fp, len);
    if( error>0 )
    {
        fprintf(stderr,"ERROR #%d: Can't get header length on file [%s] !\n", error, file);
        VDeleteBackgroundProcessInformation();
        exit(0);
    }
 
    fclose(fp);
 
    return(0);
 
}


/*---------------------------------------------------------------------------------------*/
/* Bspline: Free the memory associated with the 2D array, given number of rows
 */
void free2dfloat(p, rows)
float **p;
int rows;
{
	int i;

	for (i = 0; i < rows; i++) {
		/* Free row #i */
		free(p[i]);
	}
	/* Free array of pointers */    
	free(p);
}
/*---------------------------------------------------------------------------------------*/
/* Free the memory associated with the 3D array, given numbers of slices and rows
 */
void free3dfloat(p, slices, rows)
float*** p;
int slices, rows;
{
	int k, j;
	for (k = 0; k < slices; k++)
	{
		for (j = 0; j < rows; j++)
			free(p[k][j]);
		free(p[k]);
	}
	free(p);
}

/*---------------------------------------------------------------------------------------*/
/* Free the memory associated with the 5D array, given the size of d5, d4, d3, d2
 */
void free5dfloat(p, d5, d4, slices, rows)
float***** p;
int d5, d4, slices, rows;
{
	int i5, i4, k, j;
	for(i5 = 0; i5 < d5; i5++)
	{
		for(i4 = 0; i4 < d4; i4++)
		{
			for (k = 0; k < slices; k++)
			{
				for (j = 0; j < rows; j++)
					free(p[i5][i4][k][j]);
				free(p[i5][i4][k]);
			}
			free(p[i5][i4]);
		}
		free(p[i5]);
	}
	free(p);
}

/*---------------------------------------------------------------------------------------*/
/* B-spline related functions: B0, B3, dB3.
 */
float B0(xv)
	float xv;
{
	float absx, x=(float)xv;
	absx = x<0? -x: x;
	if (absx > 0.5)
		return 0;
	else
		return 1;
}

float B3(xv)
	float xv;
{
	float absx, x2, xtmp, x=(float)xv;
	absx = x<0? -x: x;
	if (absx < 1)
	{
		x2 =(float)(x*x);
		return (float)((4.0+3.0*x2*absx)/6.0-x2);
	}
	else
	{
		if (absx < 2)
		{
			xtmp = (float)(2.0-absx);
			return (float)(xtmp*xtmp*xtmp/6.0);			
		}
		else
			return 0;
	}	
}

float dB3(xv)
	float xv;
{
	float absx, x2, x=(float)xv;
	absx = x<0? -x: x;
	if (absx <= 1)
		return (float)((1.5*absx-2.0)*x);
	else
	{
		if (absx < 2)
		{
			x2 = (float)(x*x);
			if(x>1)
				return (float)(2.0*x-x2/2.0-2.0);
			else
				return (float)(x2*0.5+2.0*x+2.0);
		}
		else
			return 0;
	}
}
/*****************************************************************************
 * FUNCTION: calci
 * DESCRIPTION: Calculate B-spline coefficients of a 3-D volume
 * PARAMETERS:
 *    xdim, ydim, zdim: the dimensions of the image
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    bcoeff: store  the input float image and the result 3-D B-spline coefficients
 * EXIT CONDITIONS:
 *****************************************************************************/
void calci3(bcoeff, xdim, ydim, zdim)
        float* bcoeff;
        int xdim, ydim, zdim;
{
        int i, j, k, l, llength, ll, kk, jj, ji, kj;
        float z1, z1tmp, tmpsum, z2tmp;
        float *cplusx, *cplusy, *cplusz, *cminusx, *cminusy, *cminusz;

        z1 = sqrtf(3)-2;
        z1tmp = 6*z1/(z1*z1-1);
        cplusx = (float *)malloc(xdim * sizeof(float));
        cminusx = (float *)malloc(xdim * sizeof(float));
        llength = xdim * ydim;
        ll = 2*xdim-2;
        z2tmp = 1 - powf(z1, (float)ll);
	for(k=0; k<zdim; k++) {
		kk = k*llength;
		for(j=0; j<ydim; j++) {
			kj = kk + j*xdim;
		        tmpsum = bcoeff[kj];
		        for(l=1; l<xdim; l++)
		                tmpsum = tmpsum +bcoeff[kj+l]*powf(z1, (float)l);
		        for(l=0; l<xdim-1; l++)
		                tmpsum = tmpsum +bcoeff[kj+l]*powf(z1, (float)(2*xdim-2-l));
		        cplusx[0] = tmpsum/z2tmp;
		        for(l=1; l<xdim; l++)
		                cplusx[l] = bcoeff[kj+l] + z1*cplusx[l-1];
		        cminusx[xdim-1] = cplusx[xdim-1];
		        for(l=xdim-2; l>=0; l--)
		                cminusx[l] = bcoeff[kj+l] + z1*cminusx[l+1];
		        for(i=0; i<xdim; i++)
		        {
		                bcoeff[kj+i] = z1tmp*(cplusx[i]+cminusx[i]-bcoeff[kj+i]);
		        }

		}
	}
        free(cplusx);free(cminusx);
        cplusy = (float *)malloc(ydim * sizeof(float));
        cminusy = (float *)malloc(ydim * sizeof(float));
        ll = 2*ydim-2;
        z2tmp = 1 - powf(z1, (float)ll);
	for(k=0; k<zdim; k++) {
		kk = k*llength;
		for(i=0; i<xdim; i++)
		{
		        tmpsum = bcoeff[kk+i];
		        for(l=1; l<ydim; l++)
		                tmpsum = tmpsum + bcoeff[kk+l*xdim+i]* powf(z1, (float)l);
		        for(l=0; l<ydim-1; l++)
		                tmpsum = tmpsum + bcoeff[kk+l*xdim+i]* powf(z1, (float)(2*ydim-2-l));
		        cplusy[0] = tmpsum/z2tmp;
		        for(l=1; l<ydim; l++)
		                cplusy[l] = bcoeff[kk+l*xdim+i] + z1*cplusy[l-1];
		        cminusy[ydim-1] = cplusy[ydim-1];
		        for(l=ydim-2; l>=0; l--)
		                cminusy[l] = bcoeff[kk+l*xdim+i] + z1*cminusy[l+1];
		        for(j=0; j<ydim; j++)
		        {
		                bcoeff[kk+j*xdim+i] = z1tmp*(cplusy[j]+cminusy[j]-bcoeff[kk+j*xdim+i]);
		        }
		}
	}
        free(cplusy); free(cminusy);
        cplusz = (float *)malloc(zdim * sizeof(float));
        cminusz = (float *)malloc(zdim * sizeof(float));
        ll = 2*zdim-2;
        z2tmp = 1 - powf(z1, (float)ll);
	for(j=0; j<ydim; j++) {
		jj = j*xdim;
		for(i=0; i<xdim; i++)
		{
			ji = jj + i;
		        tmpsum = bcoeff[ji];
		        for(l=1; l<zdim; l++)
		                tmpsum = tmpsum + bcoeff[l*llength+ji]*powf(z1, (float)l);
		        for(l=0; l<zdim-1; l++)
		                tmpsum = tmpsum + bcoeff[l*llength+ji]*powf(z1, (float)(2*zdim-2-l));
		        cplusz[0] = tmpsum/z2tmp;
		        for(l=1; l<zdim; l++)
		                cplusz[l] = bcoeff[l*llength+ji] + z1*cplusz[l-1];
		        cminusz[zdim-1] = cplusz[zdim-1];
		        for(l=zdim-2; l>=0; l--)
		                cminusz[l] = bcoeff[l*llength+ji] + z1*cminusz[l+1];
		        for(k=0; k<zdim; k++)
		        {
		                bcoeff[k*llength+ji] = z1tmp*(cplusz[k]+cminusz[k]-bcoeff[k*llength+ji]);
		        }
		}
	}
        free(cplusz); free(cminusz);
}

/*************************************************************************
 * FUNCTION: bspline_3d16
 * DESCRIPTION: Applies B-spline deformation to a voxel
 * PARAMETERS:
 *    tmp3dctrl: 3D control point mesh
 *    oldx,oldy,oldz: the input voxel position
 *    spacex, spacey, spacez: cordinates spacing of control mesh
 *    cnumx, cnumy, cnumz: control point resolution
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    newx,newy,newz: store the deformed position
 * EXIT CONDITIONS:
 *****************************************************************************/
void bspline_3d16_1d(tmp3dctrl, spacex,spacey,spacez, cnumx, cnumy, cnumz,oldx,oldy,oldz,newx,newy,newz)
	float* tmp3dctrl;
	float spacex, spacey, spacez;
	int cnumx, cnumy, cnumz, oldx, oldy, oldz;
	float *newx;
	float *newy;
	float *newz;
{
	int i, j, k, ljx, rjx, ljy, rjy, ljz, rjz, kk, kj, ipos;
	float dx, dy, dz, dj;
	
	ljx = (int)floor(oldx/spacex)-1; if (ljx < 0) ljx = 0;
	rjx = (int)ceil(oldx/spacex)+2; if (rjx > cnumx) rjx = cnumx;
	ljy = (int)floor(oldy/spacey)-1; if (ljy < 0) ljy = 0;
	rjy = (int)ceil(oldy/spacey)+2; if (rjy > cnumy) rjy = cnumy;
	ljz = (int)floor(oldz/spacez)-1; if (ljz < 0) ljz = 0;
	rjz = (int)ceil(oldz/spacez)+2;  if (rjz > cnumz) rjz = cnumz;
	dx = 0; dy = 0; dz = 0;
	for(k=ljz; k<rjz; k++)
	{
		kk = k*cnumy;
		for(j=ljy; j<rjy; j++)
		{
			kj = (kk + j)*cnumx;
			for(i=ljx; i<rjx; i++)
			{
				dj = B3(oldx/spacex-i)*B3(oldy/spacey-j)*B3(oldz/spacez-k);
				ipos = (kj + i)*3;		dx = dx + dj*tmp3dctrl[ipos];
				ipos++; dy = dy + dj*tmp3dctrl[ipos];
				ipos++; dz = dz + dj*tmp3dctrl[ipos];
			}
		}
	}
	*newx = oldx + dx;
	*newy = oldy + dy;
	*newz = oldz + dz;

}



/*****************************************************************************
 * FUNCTION: bspline_3d16interop
 * DESCRIPTION: Applies B-spline deformation and interpolation to a 3d image
 * PARAMETERS:
 *    tmp3dctrl: 3D control point mesh
 *    bcoeff: the input data
 *    spacex, spacey, spacez: cordinates spacing of control mesh
 *    xdim, ydim, zdim: the dimensions of the original test image, which is also the memory size
 *    nxdim, nydim, nzdim: the dimensions of the image on current level
 *    cnumx, cnumy, cnumz: control point resolution
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    out: store the deformed 3-D volume
 * EXIT CONDITIONS:
 *****************************************************************************/
void bsplineimage(tmp3dctrl, in, out, spacex, spacey, spacez, xdim,ydim,zdim, cnumx, cnumy, cnumz)
        float *tmp3dctrl, *in;
        unsigned short *out;
        float spacex, spacey, spacez;
        int xdim, ydim, zdim, cnumx, cnumy, cnumz;
{
        int i, j, k, ljx, rjx, ljy, rjy, ljz, rjz, kz, jy, ix, lz, rz, ly, ry, lx, rx, oldx, oldy, oldz, kz0, jy0, ix0, hlength, cxy, ipos, c3, kk, kj2, vpos;
        float dx, dy, dz, dj, newx, newy, newz, ci, fsum;

        hlength = xdim * ydim; c3 = cnumx*3; cxy = c3 *cnumy; vpos = -1;
        for(oldz = 0; oldz<zdim; oldz++)
        {
                for(oldy= 0; oldy<ydim; oldy++)
                {
                        for (oldx=0; oldx<xdim; oldx++)
                        {
			vpos++;
                        ljx = (int)floor(oldx/spacex)-1; if (ljx < 0) ljx = 0;
                        rjx = (int)ceil(oldx/spacex)+2; if (rjx > cnumx) rjx = cnumx;
                        ljy = (int)floor(oldy/spacey)-1; if (ljy < 0) ljy = 0;
                        rjy = (int)ceil(oldy/spacey)+2; if (rjy > cnumy) rjy = cnumy;
                        ljz = (int)floor(oldz/spacez)-1; if (ljz < 0) ljz = 0;
                        rjz = (int)ceil(oldz/spacez)+2;  if (rjz > cnumz) rjz = cnumz;
                        dx = 0; dy = 0; dz = 0;
                        for(k=ljz; k<rjz; k++)
                        {
				kk = k*cxy;
                                for(j=ljy; j<rjy; j++)
                                {
					kj2 = kk+j*c3;
                                        for(i=ljx; i<rjx; i++)
                                        {
                                                dj = B3(oldx/spacex-i)*B3(oldy/spacey-j)*B3(oldz/spacez-k);
						ipos = kj2+i*3;
                                                dx = dx + dj*tmp3dctrl[ipos];
                                                ipos++; dy = dy + dj*tmp3dctrl[ipos];
                                                ipos++; dz = dz + dj*tmp3dctrl[ipos];
                                        }
                                }
                        }
                        newx = oldx + dx;
                        newy = oldy + dy;
                        newz = oldz + dz;
                        //got new position. 
			//to interpolate the intensity, first calculate the neigbors and use mirror for the image border
                        lz = (int)floor(newz)-1;
                        rz = (int)ceil(newz)+2;
                        ly = (int)floor(newy)-1;
                        ry = (int)ceil(newy)+2;
                        lx = (int)floor(newx)-1;
                        rx = (int)ceil(newx)+2;
                        fsum = 0;
                        for(kz=lz; kz<rz; kz++)
                        {
                                kz0 = kz; if (kz<0) kz0 = abs(kz); if(kz>=zdim) kz0 = zdim*2 - 2 - kz;
				kk = kz0*hlength;
                                for(jy=ly; jy<ry; jy++)
                                {
                                        jy0 = jy; if (jy<0) jy0 = abs(jy); if(jy>=ydim) jy0 = ydim*2 - 2 - jy;
					kj2 = kk+jy0*xdim;
                                        for(ix=lx; ix<rx; ix++)
                                        {
                                                ix0 = ix; if (ix<0) ix0 = abs(ix); if(ix>=xdim) ix0 = xdim*2 - 2 - ix;
                                                ci = in[kj2+ix0];
                                                fsum = ci*B3(newx-ix)*B3(newy-jy)*B3(newz-kz)+ fsum;
                                        }
                                }
                        }
                        if(fsum<0) out[vpos]=0; else out[vpos] = (unsigned short)(fsum+0.5);
                        }
                }
        }
}

void bsplineimagedf(tmp3dctrl, in, out, spacex, spacey, spacez, xdim,ydim,zdim, cnumx, cnumy, cnumz, outdf, vx, vy, vz)
        float *tmp3dctrl, *in;
        unsigned short *out;
        float spacex, spacey, spacez;
        int xdim, ydim, zdim, cnumx, cnumy, cnumz;
	short *outdf;
	float vx, vy, vz;
{
        int i, j, k, ljx, rjx, ljy, rjy, ljz, rjz, kz, jy, ix, lz, rz, ly, ry, lx, rx, oldx, oldy, oldz, kz0, jy0, ix0, hlength, cxy, ipos, c3, kk, kj2, vpos, dfpos;
        float dx, dy, dz, dj, newx, newy, newz, ci, fsum, vdf;

        hlength = xdim * ydim;		c3 = cnumx*3; cxy = c3 *cnumy;
		vpos = -1;
		vdf = (float)(4.0*vz/vx);
        for(oldz = 0; oldz<zdim; oldz++)  for(oldy= 0; oldy<ydim; oldy++)   for (oldx=0; oldx<xdim; oldx++) {
		vpos++; dfpos = vpos*3;
                ljx = (int)floor(oldx/spacex)-1; if (ljx < 0) ljx = 0;
                rjx = (int)ceil(oldx/spacex)+2; if (rjx > cnumx) rjx = cnumx;
                ljy = (int)floor(oldy/spacey)-1; if (ljy < 0) ljy = 0;
                rjy = (int)ceil(oldy/spacey)+2; if (rjy > cnumy) rjy = cnumy;
                ljz = (int)floor(oldz/spacez)-1; if (ljz < 0) ljz = 0;
                rjz = (int)ceil(oldz/spacez)+2;  if (rjz > cnumz) rjz = cnumz;
                dx = 0; dy = 0; dz = 0;
                for(k=ljz; k<rjz; k++) {
			kk = k*cxy;
                        for(j=ljy; j<rjy; j++) {
				kj2 = kk+j*c3;
                                for(i=ljx; i<rjx; i++)     {
                                        dj = B3(oldx/spacex-i)*B3(oldy/spacey-j)*B3(oldz/spacez-k);
					ipos = kj2+i*3;
                                        dx = dx + dj*tmp3dctrl[ipos];
                                        ipos++; dy = dy + dj*tmp3dctrl[ipos];
                                        ipos++; dz = dz + dj*tmp3dctrl[ipos];
                                }
                        }
                }
                newx = oldx + dx;
                newy = oldy + dy;
                newz = oldz + dz;
		if(dx>0) outdf[dfpos] = (short)(dx*4+0.5); else outdf[dfpos] = (short)(dx*4-0.5);
		if(dy>0) outdf[dfpos+1] = (short)(dy*4+0.5); else outdf[dfpos+1] = (short)(dy*4-0.5);
		if(dz>0) outdf[dfpos+2] = (short)(dz*vdf+0.5); else outdf[dfpos+2] = (short)(dz*vdf-0.5); 
                        lz = (int)floor(newz)-1;
                        rz = (int)ceil(newz)+2;
                        ly = (int)floor(newy)-1;
                        ry = (int)ceil(newy)+2;
                        lx = (int)floor(newx)-1;
                        rx = (int)ceil(newx)+2;
                        fsum = 0;
                        for(kz=lz; kz<rz; kz++)
                        {
                                kz0 = kz; if (kz<0) kz0 = abs(kz); if(kz>=zdim) kz0 = zdim*2 - 2 - kz;
				kk = kz0*hlength;
                                for(jy=ly; jy<ry; jy++)
                                {
                                        jy0 = jy; if (jy<0) jy0 = abs(jy); if(jy>=ydim) jy0 = ydim*2 - 2 - jy;
					kj2 = kk+jy0*xdim;
                                        for(ix=lx; ix<rx; ix++)
                                        {
                                                ix0 = ix; if (ix<0) ix0 = abs(ix); if(ix>=xdim) ix0 = xdim*2 - 2 - ix;
                                                ci = in[kj2+ix0];
                                                fsum = ci*B3(newx-ix)*B3(newy-jy)*B3(newz-kz)+ fsum;
                                        }
                                }
                        }
                        if(fsum<0) out[vpos]=0; else out[vpos] = (unsigned short)(fsum+0.5);
        }
}


/*****************************************************************************
 * FUNCTION: pyramid3d
 * DESCRIPTION: Calculate B-spline pyramid of 3d image
 * PARAMETERS:
 *    g: B-spline template
 *    ng: the size of the template
 *    xdim, ydim, zdim: the dimensions of the input image
 * SIDE EFFECTS: mirror the last cell if the size is odd
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    out: store the 3-D image and the resized output
 * EXIT CONDITIONS:
 *****************************************************************************/
void pyramid3dff(in, out1, g, ng, xdim, ydim, zdim)
	float *in, *out1;
	float *g;
	short ng;
	int xdim, ydim, zdim;//image size before reduce
{
	float ***out;
	int nxdim, nydim, nzdim;//image size after reduce
	int ir, jc, ks;
	float *tmp;
	int n, kn, k, j, i, i1, i2, kk, kj;

	out = (float ***)malloc(zdim * sizeof(float**));
	for (k = 0; k < zdim; k++)
	{
		out[k] = (float **) malloc(ydim * sizeof(float *));
		for (j = 0; j < ydim; j++)
			out[k][j] = (float *)malloc(xdim * sizeof(float));
	}
	for (k = 0; k < zdim; k++)
	{
		kk = k * ydim * xdim;
		for (j = 0; j < ydim; j++)
		{
			kj = kk + j*xdim;
			for(i = 0; i<xdim; i++)
			{
				out[k][j][i] = in[kj+i];
			}
		}
	}
	for (ks = 0; ks< zdim; ks++) for (jc=0; jc<ydim; jc++)//reduce by rows
	{
		nxdim = xdim/2;
		n = nxdim*2;
		kn = 2*n;
	
		/* --- Allocated memory for a temporary buffer --- */
		tmp = (float *)malloc(n*sizeof(float));
		if ( tmp == (float *)NULL) {
			printf("Out of memory in reduce_centered!");
			return;
		}
	
		/* --- Apply the symmetric filter to all the coefficients --- */
		for (k=0; k<n; k++) {
			tmp[k] = out[ks][jc][k]*g[0];
			for (i=1; i<ng; i++) {
				i1 = k-i;
				i2 = k+i;
				if (i1 < 0) {
					i1 = (2*n-1-i1) % kn;
					if (i1 >= n) 
						i1 = kn-i1-1;
				}
				if (i2 > (n-1)) {
					i2 = i2 % kn;
					if (i2 >= n) i2 = kn-i2-1;
				}
				tmp[k] = tmp[k] + g[i]*(out[ks][jc][i1]+out[ks][jc][i2]);
			}
		}
	
		/* --- Now apply the Haar and perform downsampling --- */
		for(kk=0; kk<nxdim; kk++) {
			k = 2*kk;
			out[ks][jc][kk] = (tmp[k] + tmp[k+1])/(float)2.;
		}

		/* --- Free allocated memory --- */
		free(tmp);
	}

	for (ks = 0; ks< zdim; ks++) for (ir=0; ir<nxdim; ir++)//reduce by cols
	{
		nydim = ydim/2;
		n = nydim*2;
		kn = 2*n;
	
		/* --- Allocated memory for a temporary buffer --- */
		tmp = (float *)malloc(n*sizeof(float));
		if ( tmp == (float *)NULL) {
			printf("Out of memory in reduce_centered!");
			return;
		}
	
		/* --- Apply the symmetric filter to all the coefficients --- */
		for (k=0; k<n; k++) {
			tmp[k] = out[ks][k][ir]*g[0];
			for (i=1; i<ng; i++) {
				i1 = k-i;
				i2 = k+i;
				if (i1 < 0) {
					i1 = (2*n-1-i1) % kn;
					if (i1 >= n) 
						i1 = kn-i1-1;
				}
				if (i2 > (n-1)) {
					i2 = i2 % kn;
					if (i2 >= n) i2 = kn-i2-1;
				}
				tmp[k] = tmp[k] + g[i]*(out[ks][i1][ir]+out[ks][i2][ir]);
			}
		}
	
		/* --- Now apply the Haar and perform downsampling --- */
		for(kk=0; kk<nydim; kk++) {
			k = 2*kk;
			out[ks][kk][ir] = (tmp[k] + tmp[k+1])/(float)2.;
		}

		/* --- Free allocated memory --- */
		free(tmp);
	}

	for (jc=0; jc<nydim; jc++) for (ir=0; ir<nxdim; ir++)//reduce by slices
	{
		nzdim = zdim/2;
		n = nzdim*2;
		kn = 2*n;
	
		/* --- Allocated memory for a temporary buffer --- */
		tmp = (float *)malloc(n*sizeof(float));
		if ( tmp == (float *)NULL) {
			printf("Out of memory in reduce_centered!");
			return;
		}
	
		/* --- Apply the symmetric filter to all the coefficients --- */
		for (k=0; k<n; k++) {
			tmp[k] = out[k][jc][ir]*g[0];
			for (i=1; i<ng; i++) {
				i1 = k-i;
				i2 = k+i;
				if (i1 < 0) {
					i1 = (2*n-1-i1) % kn;
					if (i1 >= n) 
						i1 = kn-i1-1;
				}
				if (i2 > (n-1)) {
					i2 = i2 % kn;
					if (i2 >= n) i2 = kn-i2-1;
				}
				tmp[k] = tmp[k] + g[i]*(out[i1][jc][ir]+out[i2][jc][ir]);
			}
		}
	
		/* --- Now apply the Haar and perform downsampling --- */
		for(kk=0; kk<nzdim; kk++) {
			k = 2*kk;
			out[kk][jc][ir] = (tmp[k] + tmp[k+1])/(float)2.;
		}

		/* --- Free allocated memory --- */
		free(tmp);
	}

	for (k = 0; k < nzdim; k++)
	{
		kk = k * nydim * nxdim;
		for (j = 0; j < nydim; j++)
		{
			kj = kk + j*nxdim;
			for(i = 0; i<nxdim; i++)
			{
				out1[kj+i] = out[k][j][i];
			}
		}
	}
	free3dfloat(out,zdim, ydim);

}
void pyramid3dif(in, out1, g, ng, xdim, ydim, zdim)
	unsigned short *in;
	float *out1;
	float *g;
	short ng;
	int xdim, ydim, zdim;//image size before reduce
{
	float ***out;
	int nxdim, nydim, nzdim;//image size after reduce
	int ir, jc, ks;
	float *tmp;
	int n, kn, k, j, i, i1, i2, kk, kj;

	out = (float ***)malloc(zdim * sizeof(float**));
	for (k = 0; k < zdim; k++)
	{
		out[k] = (float **) malloc(ydim * sizeof(float *));
		for (j = 0; j < ydim; j++)
			out[k][j] = (float *)malloc(xdim * sizeof(float));
	}
	for (k = 0; k < zdim; k++)
	{
		kk = k * ydim * xdim;
		for (j = 0; j < ydim; j++)
		{
			kj = kk + j*xdim;
			for(i = 0; i<xdim; i++)
			{
				out[k][j][i] = (float)in[kj+i];
			}
		}
	}
	for (ks = 0; ks< zdim; ks++) for (jc=0; jc<ydim; jc++)//reduce by rows
	{
		nxdim = xdim/2;
		n = nxdim*2;
		kn = 2*n;
	
		/* --- Allocated memory for a temporary buffer --- */
		tmp = (float *)malloc(n*sizeof(float));
		if ( tmp == (float *)NULL) {
			printf("Out of memory in reduce_centered!");
			return;
		}
	
		/* --- Apply the symmetric filter to all the coefficients --- */
		for (k=0; k<n; k++) {
			tmp[k] = out[ks][jc][k]*g[0];
			for (i=1; i<ng; i++) {
				i1 = k-i;
				i2 = k+i;
				if (i1 < 0) {
					i1 = (2*n-1-i1) % kn;
					if (i1 >= n) 
						i1 = kn-i1-1;
				}
				if (i2 > (n-1)) {
					i2 = i2 % kn;
					if (i2 >= n) i2 = kn-i2-1;
				}
				tmp[k] = tmp[k] + g[i]*(out[ks][jc][i1]+out[ks][jc][i2]);
			}
		}
	
		/* --- Now apply the Haar and perform downsampling --- */
		for(kk=0; kk<nxdim; kk++) {
			k = 2*kk;
			out[ks][jc][kk] = (tmp[k] + tmp[k+1])/(float)2.;
		}

		/* --- Free allocated memory --- */
		free(tmp);
	}

	for (ks = 0; ks< zdim; ks++) for (ir=0; ir<nxdim; ir++)//reduce by cols
	{
		nydim = ydim/2;
		n = nydim*2;
		kn = 2*n;
	
		/* --- Allocated memory for a temporary buffer --- */
		tmp = (float *)malloc(n*sizeof(float));
		if ( tmp == (float *)NULL) {
			printf("Out of memory in reduce_centered!");
			return;
		}
	
		/* --- Apply the symmetric filter to all the coefficients --- */
		for (k=0; k<n; k++) {
			tmp[k] = out[ks][k][ir]*g[0];
			for (i=1; i<ng; i++) {
				i1 = k-i;
				i2 = k+i;
				if (i1 < 0) {
					i1 = (2*n-1-i1) % kn;
					if (i1 >= n) 
						i1 = kn-i1-1;
				}
				if (i2 > (n-1)) {
					i2 = i2 % kn;
					if (i2 >= n) i2 = kn-i2-1;
				}
				tmp[k] = tmp[k] + g[i]*(out[ks][i1][ir]+out[ks][i2][ir]);
			}
		}
	
		/* --- Now apply the Haar and perform downsampling --- */
		for(kk=0; kk<nydim; kk++) {
			k = 2*kk;
			out[ks][kk][ir] = (tmp[k] + tmp[k+1])/(float)2.;
		}

		/* --- Free allocated memory --- */
		free(tmp);
	}

	for (jc=0; jc<nydim; jc++) for (ir=0; ir<nxdim; ir++)//reduce by slices
	{
		nzdim = zdim/2;
		n = nzdim*2;
		kn = 2*n;
	
		/* --- Allocated memory for a temporary buffer --- */
		tmp = (float *)malloc(n*sizeof(float));
		if ( tmp == (float *)NULL) {
			printf("Out of memory in reduce_centered!");
			return;
		}
	
		/* --- Apply the symmetric filter to all the coefficients --- */
		for (k=0; k<n; k++) {
			tmp[k] = out[k][jc][ir]*g[0];
			for (i=1; i<ng; i++) {
				i1 = k-i;
				i2 = k+i;
				if (i1 < 0) {
					i1 = (2*n-1-i1) % kn;
					if (i1 >= n) 
						i1 = kn-i1-1;
				}
				if (i2 > (n-1)) {
					i2 = i2 % kn;
					if (i2 >= n) i2 = kn-i2-1;
				}
				tmp[k] = tmp[k] + g[i]*(out[i1][jc][ir]+out[i2][jc][ir]);
			}
		}
	
		/* --- Now apply the Haar and perform downsampling --- */
		for(kk=0; kk<nzdim; kk++) {
			k = 2*kk;
			out[kk][jc][ir] = (tmp[k] + tmp[k+1])/(float)2.;
		}

		/* --- Free allocated memory --- */
		free(tmp);
	}

	for (k = 0; k < nzdim; k++)
	{
		kk = k * nydim * nxdim;
		for (j = 0; j < nydim; j++)
		{
			kj = kk + j*nxdim;
			for(i = 0; i<nxdim; i++)
			{
				out1[kj+i] = out[k][j][i];
			}
		}
	}
	free3dfloat(out,zdim, ydim);

}


/*****************************************************************************
 * paper "Modeling liver motion and deformation during the respiratory cycle using intensity-based free-form registration of gated MR images"
 * FUNCTION: multicontrol
 * DESCRIPTION: refine the control mesh into the next finer level
 * PARAMETERS:
 *    tmp3dctrl: 3D control point mesh
 *    oldcnumx, oldcnumy, oldcnumz: old control point resolution
 *    cnumx, cnumy, cnumz: finer control point resolution
 * SIDE EFFECTS: None
 * RETURN VALUE:
 *    tmp3dctrl: input and output 3D control point mesh
 * EXIT CONDITIONS:
 *****************************************************************************/
void multicontrol1d(oldctrl, newctrl, oldcnumx, oldcnumy, oldcnumz, cnumx, cnumy, cnumz)
	float* oldctrl, *newctrl;
	int oldcnumx, oldcnumy, oldcnumz, cnumx, cnumy, cnumz;
{
	float ***h3dctrl, ***tmp3dctrl;
	int i, j, k, i2, j2, k2, x6, y2, z2, llength, cx3;
	float v000, v001, v010, v011, v100, v101, v110, v111;

	cx3 = oldcnumx * 3;
	tmp3dctrl = (float ***)malloc(oldcnumz * sizeof(float**));
	for (k = 0; k < oldcnumz; k++)
	{
		tmp3dctrl[k] = (float **) malloc(oldcnumy * sizeof(float *));
		for (j = 0; j < oldcnumy; j++)
			tmp3dctrl[k][j] = (float *)malloc(cx3 * sizeof(float));
	}
	llength = cx3 * oldcnumy;
	for (k = 0; k < oldcnumz; k++)
	{
		z2 = k*llength;
		for(j = 0; j < oldcnumy; j++)
		{
			j2 = z2 + j* cx3;
			for (i = 0; i < cx3; i++)
			{
				tmp3dctrl[k][j][i] = oldctrl[j2+i];
			}
		}
	}

	z2 = oldcnumz * 2;
	y2 = oldcnumy * 2;
	x6 = oldcnumx * 6;
	h3dctrl = (float ***)malloc(z2 * sizeof(float**));
	for (k = 0; k < z2; k++)
	{
		h3dctrl[k] = (float **) malloc(y2 * sizeof(float *));
		for (j = 0; j < y2; j++)
			h3dctrl[k][j] = (float *)malloc(x6 * sizeof(float));
	}
	for (k = 0; k < z2; k++)
	{
		for(j = 0; j < y2; j++)
		{
			for (i = 0; i < x6; i++)
			{
				h3dctrl[k][j][i] = 0;
			}
		}
	}
	//reset control points from coarse level to the adjacent fine level
	//section 1.1
	for (i=0; i<3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[0][0][i]; v001 = tmp3dctrl[0][0][i+3];
		v010 = tmp3dctrl[0][1][i]; v011 = tmp3dctrl[0][1][i+3];
		v100 = tmp3dctrl[1][0][i]; v101 = tmp3dctrl[1][0][i+3];
		v110 = tmp3dctrl[1][1][i]; v111 = tmp3dctrl[1][1][i+3];
		h3dctrl[1][1][i2+3] = (v000 + v001 + v010 + v011 + v100 + v101 + v110 + v111)/8; //#8
		h3dctrl[0][1][i2+3] = (v100 + v101 + v110 + v111 + 6*(v000 + v001 + v010 + v011))/28;//#7
		h3dctrl[1][0][i2+3] = (v010 + v011 + v110 + v111 + 6 * (v000 + v001 + v100 + v101))/28;//#6
		h3dctrl[1][1][i2] = (v001 + v011 + v101 + v111+ 6*(v000 + v010 + v100 + v110))/28;//#5
		h3dctrl[0][0][i2+3] = (v110 + v111+ 6*(v010 + v011+v100 + v101) + 36*(v000 + v001))/98;//#4
		h3dctrl[0][1][i2] = (v101+v111 +6*(v100+v110+v001+v011) + 36*(v000 + v010))/98;//#3
		h3dctrl[1][0][i2] = (tmp3dctrl[0][1][i+3] +v111+6*(tmp3dctrl[0][1][i]+v110+tmp3dctrl[0][0][i+3]+v101)+36*(v100+v000))/98; //#2
		h3dctrl[0][0][i2] = (v111 +6*(v011+v101+v110)+36*(v100+v010+v001)+216*v000)/343;//1
	}
	for (i = 3; i < oldcnumx*3-3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[0][0][i]; v001 = tmp3dctrl[0][0][i+3];
		v010 = tmp3dctrl[0][1][i]; v011 = tmp3dctrl[0][1][i+3];
		v100 = tmp3dctrl[1][0][i]; v101 = tmp3dctrl[1][0][i+3];
		v110 = tmp3dctrl[1][1][i]; v111 = tmp3dctrl[1][1][i+3];
		h3dctrl[1][1][i2+3] = (v000 + v001 + v010 + v011 + v100 + v101 + v110 + v111)/8; //#8
		h3dctrl[0][1][i2+3] = (v100 + v101 + v110 + v111 + 6*(v000 + v001 + v010 + v011))/28;//#7
		h3dctrl[1][0][i2+3] = (v010 + v011 + v110 + v111 + 6 * (v000 + v001 + v100 + v101))/28;//#6
		h3dctrl[1][1][i2] = ( tmp3dctrl[0][0][i-3] + tmp3dctrl[0][1][i-3] + tmp3dctrl[1][0][i-3] + tmp3dctrl[1][1][i-3] + v001 + v011 + v101 + v111+ 6*(v000 + v010 + v100 + v110))/32;//#5
		h3dctrl[0][0][i2+3] = (v110 + v111+ 6*(v010 + v011+v100 + v101) + 36*(v000 + v001))/98;//#4
		h3dctrl[0][1][i2] = (tmp3dctrl[1][0][i-3]+tmp3dctrl[1][1][i-3]+v101+v111 +6*(v100+v110+tmp3dctrl[0][0][i-3]+tmp3dctrl[0][1][i-3]+v001+v011) + 36*(v000 + v010))/112;//#3
		h3dctrl[1][0][i2] = (tmp3dctrl[0][1][i-3]+tmp3dctrl[0][1][i+3]+tmp3dctrl[1][1][i-3] +v111+6*(tmp3dctrl[0][1][i]+v110+tmp3dctrl[0][0][i-3]+tmp3dctrl[0][0][i+3]+tmp3dctrl[1][0][i-3]+v101)+36*(v100+v000))/112; //#2
		h3dctrl[0][0][i2] = (tmp3dctrl[1][1][i-3]+v111 +6*(tmp3dctrl[0][1][i-3]+v011+tmp3dctrl[1][0][i-3]+v101+v110)+36*(v100+tmp3dctrl[0][0][i-3]+v010+v001)+216*v000)/392;//1
	}
	for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[0][0][i];
		v010 = tmp3dctrl[0][1][i];
		v100 = tmp3dctrl[1][0][i];
		v110 = tmp3dctrl[1][1][i];
		h3dctrl[1][1][i2] = ( tmp3dctrl[0][0][i-3] + tmp3dctrl[0][1][i-3] + tmp3dctrl[1][0][i-3] + tmp3dctrl[1][1][i-3] + 6*(v000 + v010 + v100 + v110))/28;//#5
		h3dctrl[0][1][i2] = (tmp3dctrl[1][0][i-3]+tmp3dctrl[1][1][i-3] +6*(v100+v110+tmp3dctrl[0][0][i-3]+tmp3dctrl[0][1][i-3]) + 36*(v000 + v010))/98;//#3
		h3dctrl[1][0][i2] = (tmp3dctrl[0][1][i-3]+tmp3dctrl[1][1][i-3] +6*(tmp3dctrl[0][1][i]+v110+tmp3dctrl[0][0][i-3]+tmp3dctrl[1][0][i-3])+36*(v100+v000))/98; //#2
		h3dctrl[0][0][i2] = (tmp3dctrl[1][1][i-3]+6*(tmp3dctrl[0][1][i-3]+tmp3dctrl[1][0][i-3]+v110)+36*(v100+tmp3dctrl[0][0][i-3]+v010)+216*v000)/343;//1
	}
	//section 1.2
	for(j = 1; j < oldcnumy-1; j++)
	{
		j2 = j*2;
		for (i=0; i<3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[0][j][i]; v001 = tmp3dctrl[0][j][i+3];
			v010 = tmp3dctrl[0][j+1][i]; v011 = tmp3dctrl[0][j+1][i+3];
			v100 = tmp3dctrl[1][j][i]; v101 = tmp3dctrl[1][j][i+3];
			v110 = tmp3dctrl[1][j+1][i]; v111 = tmp3dctrl[1][j+1][i+3];
			h3dctrl[1][j2+1][i2+3] = (v000 + v001 + v010 + v011 + v100 + v101 + v110 + v111)/8; //#8
			h3dctrl[0][j2+1][i2+3] = (v100 + v101 + v110 + v111 + 6*(v000 + v001 + v010 + v011))/28;//#7
			h3dctrl[1][j2][i2+3] = (tmp3dctrl[0][j-1][i] + tmp3dctrl[0][j-1][i+3] + tmp3dctrl[1][j-1][i] + tmp3dctrl[1][j-1][i+3] + v010 + v011 + v110 + v111 + 6 * (v000 + v001 + v100 + v101))/32;//#6
			h3dctrl[1][j2+1][i2] = (v001 + v011 + v101 + v111+ 6*(v000 + v010 + v100 + v110))/28;//#5
			h3dctrl[0][j2][i2+3] = (tmp3dctrl[1][j-1][i]+tmp3dctrl[1][j-1][i+3]+ v110 + v111+ 6*(tmp3dctrl[0][j-1][i] + tmp3dctrl[0][j-1][i+3] + v010 + v011+ v100 + v101) + 36*(v000 + v001))/112;//#4
			h3dctrl[0][j2+1][i2] = (v101+v111 +6*(v100+v110+v001+v011) + 36*(v000 + v010))/98;//#3
			h3dctrl[1][j2][i2] = (tmp3dctrl[0][j-1][i+3]+tmp3dctrl[0][j+1][i+3]+tmp3dctrl[1][j-1][i+3]+v111+6*(tmp3dctrl[0][j-1][i]+tmp3dctrl[0][j+1][i]+tmp3dctrl[1][j-1][i]+v110+tmp3dctrl[0][j][i+3]+v101)+36*(v100+v000))/112; //#2
			h3dctrl[0][j2][i2] = (tmp3dctrl[1][j-1][i+3]+v111 +6*(tmp3dctrl[0][j-1][i+3]+v011+tmp3dctrl[1][j-1][i]+v101+v110)+36*(v100+tmp3dctrl[0][j-1][i]+v010+v001)+216*v000)/392;//1
		}
		for (i = 3; i < oldcnumx*3-3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[0][j][i]; v001 = tmp3dctrl[0][j][i+3];
			v010 = tmp3dctrl[0][j+1][i]; v011 = tmp3dctrl[0][j+1][i+3];
			v100 = tmp3dctrl[1][j][i]; v101 = tmp3dctrl[1][j][i+3];
			v110 = tmp3dctrl[1][j+1][i]; v111 = tmp3dctrl[1][j+1][i+3];
			h3dctrl[1][j2+1][i2+3] = (v000 + v001 + v010 + v011 + v100 + v101 + v110 + v111)/8; //#8
			h3dctrl[0][j2+1][i2+3] = (v100 + v101 + v110 + v111 + 6*(v000 + v001 + v010 + v011))/28;//#7
			h3dctrl[1][j2][i2+3] = (tmp3dctrl[0][j-1][i] + tmp3dctrl[0][j-1][i+3] + tmp3dctrl[1][j-1][i] + tmp3dctrl[1][j-1][i+3] + v010 + v011 + v110 + v111 + 6 * (v000 + v001 + v100 + v101))/32;//#6
			h3dctrl[1][j2+1][i2] = (tmp3dctrl[0][j][i-3] + tmp3dctrl[0][j+1][i-3] + tmp3dctrl[1][j][i-3] + tmp3dctrl[1][j+1][i-3] + v001 + v011 + v101 + v111+ 6*(v000 + v010 + v100 + v110))/32;//#5
			h3dctrl[0][j2][i2+3] = (tmp3dctrl[1][j-1][i]+tmp3dctrl[1][j-1][i+3]+ v110 + v111+ 6*(tmp3dctrl[0][j-1][i] + tmp3dctrl[0][j-1][i+3] + v010 + v011+ v100 + v101) + 36*(v000 + v001))/112;//#4
			h3dctrl[0][j2+1][i2] = (tmp3dctrl[1][j][i-3]+tmp3dctrl[1][j+1][i-3]+v101+v111 +6*(v100+v110+tmp3dctrl[0][j][i-3]+tmp3dctrl[0][j+1][i-3]+v001+v011) + 36*(v000 + v010))/112;//#3
			h3dctrl[1][j2][i2] = (tmp3dctrl[0][j-1][i-3]+tmp3dctrl[0][j-1][i+3]+tmp3dctrl[0][j+1][i-3]+tmp3dctrl[0][j+1][i+3]+tmp3dctrl[1][j-1][i-3]+tmp3dctrl[1][j-1][i+3]+tmp3dctrl[1][j+1][i-3] +v111+6*(tmp3dctrl[0][j-1][i]+tmp3dctrl[0][j+1][i]+tmp3dctrl[1][j-1][i]+v110+tmp3dctrl[0][j][i-3]+tmp3dctrl[0][j][i+3]+tmp3dctrl[1][j][i-3]+v101)+36*(v100+v000))/128; //#2
			h3dctrl[0][j2][i2] = (tmp3dctrl[1][j-1][i-3]+tmp3dctrl[1][j-1][i+3]+tmp3dctrl[1][j+1][i-3]+v111 +6*(tmp3dctrl[0][j-1][i-3]+tmp3dctrl[0][j-1][i+3]+tmp3dctrl[0][j+1][i-3]+v011+tmp3dctrl[1][j-1][i]+tmp3dctrl[1][j][i-3]+v101+v110)+36*(v100+tmp3dctrl[0][j-1][i]+tmp3dctrl[0][j][i-3]+v010+v001)+216*v000)/448;//1
		}
		for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[0][j][i]; 
			v010 = tmp3dctrl[0][j+1][i];
			v100 = tmp3dctrl[1][j][i];
			v110 = tmp3dctrl[1][j+1][i]; 
			h3dctrl[1][j2+1][i2] = (tmp3dctrl[0][j][i-3] + tmp3dctrl[0][j+1][i-3] + tmp3dctrl[1][j][i-3] + tmp3dctrl[1][j+1][i-3] + 6*(v000 + v010 + v100 + v110))/28;//#5
			h3dctrl[0][j2+1][i2] = (tmp3dctrl[1][j][i-3]+tmp3dctrl[1][j+1][i-3]+6*(v100+v110+tmp3dctrl[0][j][i-3]+tmp3dctrl[0][j+1][i-3]) + 36*(v000 + v010))/98;//#3
			h3dctrl[1][j2][i2] = (tmp3dctrl[0][j-1][i-3]+tmp3dctrl[0][j+1][i-3]+tmp3dctrl[1][j-1][i-3]+tmp3dctrl[1][j+1][i-3] +6*(tmp3dctrl[0][j-1][i]+tmp3dctrl[0][j+1][i]+tmp3dctrl[1][j-1][i]+v110+tmp3dctrl[0][j][i-3]+tmp3dctrl[1][j][i-3])+36*(v100+v000))/112; //#2
			h3dctrl[0][j2][i2] = (tmp3dctrl[1][j-1][i-3]+tmp3dctrl[1][j+1][i-3] +6*(tmp3dctrl[0][j-1][i-3]+tmp3dctrl[0][j+1][i-3]+tmp3dctrl[1][j-1][i]+tmp3dctrl[1][j][i-3]+v110)+36*(v100+tmp3dctrl[0][j-1][i]+tmp3dctrl[0][j][i-3]+v010)+216*v000)/392;//1
		}
	}
	//section 1.3
	j2 = 2*oldcnumy - 2;
	for (i=0; i<3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[0][oldcnumy-1][i]; v001 = tmp3dctrl[0][oldcnumy-1][i+3];
		v100 = tmp3dctrl[1][oldcnumy-1][i]; v101 = tmp3dctrl[1][oldcnumy-1][i+3];
		h3dctrl[1][j2][i2+3] = (tmp3dctrl[0][oldcnumy-2][i] + tmp3dctrl[0][oldcnumy-2][i+3] + tmp3dctrl[1][oldcnumy-2][i] + tmp3dctrl[1][oldcnumy-2][i+3] + 6 * (v000 + v001 + v100 + v101))/28;//#6
		h3dctrl[0][j2][i2+3] = (tmp3dctrl[1][oldcnumy-2][i]+tmp3dctrl[1][oldcnumy-2][i+3]+ 6*(tmp3dctrl[0][oldcnumy-2][i] + tmp3dctrl[0][oldcnumy-2][i+3]+ v100 + v101) + 36*(v000 + v001))/98;//#4
		h3dctrl[1][j2][i2] = (tmp3dctrl[0][oldcnumy-2][i+3]+tmp3dctrl[1][oldcnumy-2][i+3]+6*(tmp3dctrl[0][oldcnumy-2][i]+tmp3dctrl[1][oldcnumy-2][i]+tmp3dctrl[0][oldcnumy-1][i+3]+v101)+36*(v100+v000))/98; //#2
		h3dctrl[0][j2][i2] = (tmp3dctrl[1][oldcnumy-2][i+3] +6*(tmp3dctrl[0][oldcnumy-2][i+3]+tmp3dctrl[1][oldcnumy-2][i]+v101)+36*(v100+tmp3dctrl[0][oldcnumy-2][i]+v001)+216*v000)/343;//1
	}
	for (i = 3; i < oldcnumx*3-3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[0][oldcnumy-1][i]; v001 = tmp3dctrl[0][oldcnumy-1][i+3];
		v100 = tmp3dctrl[1][oldcnumy-1][i]; v101 = tmp3dctrl[1][oldcnumy-1][i+3];
		h3dctrl[1][j2][i2+3] = (tmp3dctrl[0][oldcnumy-2][i] + tmp3dctrl[0][oldcnumy-2][i+3] + tmp3dctrl[1][oldcnumy-2][i] + tmp3dctrl[1][oldcnumy-2][i+3] + 6 * (v000 + v001 + v100 + v101))/32;//#6
		h3dctrl[0][j2][i2+3] = (tmp3dctrl[1][oldcnumy-2][i]+tmp3dctrl[1][oldcnumy-2][i+3]+ 6*(tmp3dctrl[0][oldcnumy-2][i] + tmp3dctrl[0][oldcnumy-2][i+3]+ v100 + v101) + 36*(v000 + v001))/98;//#4
		h3dctrl[1][j2][i2] = (tmp3dctrl[0][oldcnumy-2][i-3]+tmp3dctrl[0][oldcnumy-2][i+3]+tmp3dctrl[1][oldcnumy-2][i-3]+tmp3dctrl[1][oldcnumy-2][i+3]+6*(tmp3dctrl[0][oldcnumy-2][i]+tmp3dctrl[1][oldcnumy-2][i]+tmp3dctrl[0][oldcnumy-1][i-3]+tmp3dctrl[0][oldcnumy-1][i+3]+tmp3dctrl[1][oldcnumy-1][i-3]+v101)+36*(v100+v000))/112; //#2
		h3dctrl[0][j2][i2] = (tmp3dctrl[1][oldcnumy-2][i-3]+tmp3dctrl[1][oldcnumy-2][i+3] +6*(tmp3dctrl[0][oldcnumy-2][i-3]+tmp3dctrl[0][oldcnumy-2][i+3]+tmp3dctrl[1][oldcnumy-2][i]+tmp3dctrl[1][oldcnumy-1][i-3]+v101)+36*(v100+tmp3dctrl[0][oldcnumy-2][i]+tmp3dctrl[0][oldcnumy-1][i-3]+v001)+216*v000)/392;//1
	}
	for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[0][oldcnumy-1][i];
		v100 = tmp3dctrl[1][oldcnumy-1][i];
		h3dctrl[1][j2][i2] = (tmp3dctrl[0][oldcnumy-2][i-3]+tmp3dctrl[1][oldcnumy-2][i-3]+6*(tmp3dctrl[0][oldcnumy-2][i]+tmp3dctrl[1][oldcnumy-2][i]+tmp3dctrl[0][oldcnumy-1][i-3]+tmp3dctrl[1][oldcnumy-1][i-3])+36*(v100+v000))/98; //#2
		h3dctrl[0][j2][i2] = (tmp3dctrl[1][oldcnumy-2][i-3] +6*(tmp3dctrl[0][oldcnumy-2][i-3]+tmp3dctrl[1][oldcnumy-2][i]+tmp3dctrl[1][oldcnumy-1][i-3])+36*(v100+tmp3dctrl[0][oldcnumy-2][i]+tmp3dctrl[0][oldcnumy-1][i-3])+216*v000)/343;//1
	}

	/////////////////////////////////////////////////////////
	for (k = 1; k < oldcnumz-1; k++)
	{
		k2 = k * 2;
		//section 2.1
		for (i=0; i<3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[k][0][i]; v001 = tmp3dctrl[k][0][i+3];
			v010 = tmp3dctrl[k][1][i]; v011 = tmp3dctrl[k][1][i+3];
			v100 = tmp3dctrl[k+1][0][i]; v101 = tmp3dctrl[k+1][0][i+3];
			v110 = tmp3dctrl[k+1][1][i]; v111 = tmp3dctrl[k+1][1][i+3];
			h3dctrl[k2+1][1][i2+3] = (v000 + v001 + v010 + v011 + v100 + v101 + v110 + v111)/8; //#8
			h3dctrl[k2][1][i2+3] = (tmp3dctrl[k-1][0][i] + tmp3dctrl[k-1][0][i+3] + tmp3dctrl[k-1][1][i] + tmp3dctrl[k-1][1][i+3] + v100 + v101 + v110 + v111 + 6*(v000 + v001 + v010 + v011))/32;//#7
			h3dctrl[k2+1][0][i2+3] = (v010 + v011 + v110 + v111 + 6 * (v000 + v001 + v100 + v101))/28;//#6
			h3dctrl[k2+1][1][i2] = (v001 + v011 + v101 + v111+ 6*(v000 + v010 + v100 + v110))/28;//#5
			h3dctrl[k2][0][i2+3] = (tmp3dctrl[k-1][1][i]+tmp3dctrl[k-1][1][i+3]+ v110 + v111+ 6*(v010 + v011+tmp3dctrl[k-1][0][i]+tmp3dctrl[k-1][0][i+3]+ v100 + v101) + 36*(v000 + v001))/112;//#4
			h3dctrl[k2][1][i2] = (tmp3dctrl[k-1][0][i+3] +tmp3dctrl[k-1][1][i+3]+v101+v111 +6*(tmp3dctrl[k-1][0][i]+tmp3dctrl[k-1][1][i]+v100+v110+v001+v011) + 36*(v000 + v010))/112;//#3
			h3dctrl[k2+1][0][i2] = (tmp3dctrl[k][1][i+3] +v111+6*(tmp3dctrl[k][1][i]+v110+tmp3dctrl[k][0][i+3]+v101)+36*(v100+v000))/98; //#2
			h3dctrl[k2][0][i2] = (tmp3dctrl[k-1][1][i+3]+v111 +6*(tmp3dctrl[k-1][0][i+3]+tmp3dctrl[k-1][1][i]+v011+v101+v110)+36*(tmp3dctrl[k-1][0][i]+v100+v010+v001)+216*v000)/392;//1
		}
		for (i = 3; i < oldcnumx*3-3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[k][0][i]; v001 = tmp3dctrl[k][0][i+3];
			v010 = tmp3dctrl[k][1][i]; v011 = tmp3dctrl[k][1][i+3];
			v100 = tmp3dctrl[k+1][0][i]; v101 = tmp3dctrl[k+1][0][i+3];
			v110 = tmp3dctrl[k+1][1][i]; v111 = tmp3dctrl[k+1][1][i+3];
			h3dctrl[k2+1][1][i2+3] = (v000 + v001 + v010 + v011 + v100 + v101 + v110 + v111)/8; //#8
			h3dctrl[k2][1][i2+3] = (tmp3dctrl[k-1][0][i] + tmp3dctrl[k-1][0][i+3] + tmp3dctrl[k-1][1][i] + tmp3dctrl[k-1][1][i+3] + v100 + v101 + v110 + v111 + 6*(v000 + v001 + v010 + v011))/32;//#7
			h3dctrl[k2+1][0][i2+3] = (v010 + v011 + v110 + v111 + 6 * (v000 + v001 + v100 + v101))/28;//#6
			h3dctrl[k2+1][1][i2] = ( tmp3dctrl[k][0][i-3] + tmp3dctrl[k][1][i-3] + tmp3dctrl[k+1][0][i-3] + tmp3dctrl[k+1][1][i-3] + v001 + v011 + v101 + v111+ 6*(v000 + v010 + v100 + v110))/32;//#5
			h3dctrl[k2][0][i2+3] = (tmp3dctrl[k-1][1][i]+tmp3dctrl[k-1][1][i+3]+ v110 + v111+ 6*(v010 + v011+tmp3dctrl[k-1][0][i]+tmp3dctrl[k-1][0][i+3]+ v100 + v101) + 36*(v000 + v001))/112;//#4
			h3dctrl[k2][1][i2] = (tmp3dctrl[k-1][0][i-3] + tmp3dctrl[k-1][0][i+3] + tmp3dctrl[k-1][1][i-3] +tmp3dctrl[k-1][1][i+3]+tmp3dctrl[k+1][0][i-3]+tmp3dctrl[k+1][1][i-3]+v101+v111 +6*(tmp3dctrl[k-1][0][i]+tmp3dctrl[k-1][1][i]+v100+v110+tmp3dctrl[k][0][i-3]+tmp3dctrl[k][1][i-3]+v001+v011) + 36*(v000 + v010))/128;//#3
			h3dctrl[k2+1][0][i2] = (tmp3dctrl[k][1][i-3]+tmp3dctrl[k][1][i+3]+tmp3dctrl[k+1][1][i-3] +v111+6*(tmp3dctrl[k][1][i]+v110+tmp3dctrl[k][0][i-3]+tmp3dctrl[k][0][i+3]+tmp3dctrl[k+1][0][i-3]+v101)+36*(v100+v000))/112; //#2
			h3dctrl[k2][0][i2] = (tmp3dctrl[k-1][1][i-3]+tmp3dctrl[k-1][1][i+3]+tmp3dctrl[k+1][1][i-3]+v111 +6*(tmp3dctrl[k-1][0][i-3]+tmp3dctrl[k-1][0][i+3]+tmp3dctrl[k-1][1][i]+tmp3dctrl[k][1][i-3]+v011+tmp3dctrl[k+1][0][i-3]+v101+v110)+36*(tmp3dctrl[k-1][0][i]+v100+tmp3dctrl[k][0][i-3]+v010+v001)+216*v000)/448;//1
		}
		for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[k][0][i];
			v010 = tmp3dctrl[k][1][i]; 
			v100 = tmp3dctrl[k+1][0][i];
			v110 = tmp3dctrl[k+1][1][i];
			h3dctrl[k2+1][1][i2] = ( tmp3dctrl[k][0][i-3] + tmp3dctrl[k][1][i-3] + tmp3dctrl[k+1][0][i-3] + tmp3dctrl[k+1][1][i-3] + 6*(v000 + v010 + v100 + v110))/28;//#5
			h3dctrl[k2][1][i2] = (tmp3dctrl[k-1][0][i-3] + tmp3dctrl[k-1][1][i-3]+tmp3dctrl[k+1][0][i-3]+tmp3dctrl[k+1][1][i-3] +6*(tmp3dctrl[k-1][0][i]+tmp3dctrl[k-1][1][i]+v100+v110+tmp3dctrl[k][0][i-3]+tmp3dctrl[k][1][i-3]) + 36*(v000 + v010))/112;//#3
			h3dctrl[k2+1][0][i2] = (tmp3dctrl[k][1][i-3]+tmp3dctrl[k+1][1][i-3] +6*(tmp3dctrl[k][1][i]+v110+tmp3dctrl[k][0][i-3]+tmp3dctrl[k+1][0][i-3])+36*(v100+v000))/98; //#2
			h3dctrl[k2][0][i2] = (tmp3dctrl[k-1][1][i-3]+tmp3dctrl[k+1][1][i-3]+6*(tmp3dctrl[k-1][0][i-3]+tmp3dctrl[k-1][1][i]+tmp3dctrl[k][1][i-3]+tmp3dctrl[k+1][0][i-3]+v110)+36*(tmp3dctrl[k-1][0][i]+v100+tmp3dctrl[k][0][i-3]+v010)+216*v000)/392;//1
		}
		//section 2.2
		for(j = 1; j < oldcnumy-1; j++)
		{
			j2 = j*2;
			for (i=0; i<3; i++)
			{
				if(i%3 ==0)i2 = i * 2; else i2++;
				v000 = tmp3dctrl[k][j][i]; v001 = tmp3dctrl[k][j][i+3];
				v010 = tmp3dctrl[k][j+1][i]; v011 = tmp3dctrl[k][j+1][i+3];
				v100 = tmp3dctrl[k+1][j][i]; v101 = tmp3dctrl[k+1][j][i+3];
				v110 = tmp3dctrl[k+1][j+1][i]; v111 = tmp3dctrl[k+1][j+1][i+3];
				h3dctrl[k2+1][j2+1][i2+3] = (v000 + v001 + v010 + v011 + v100 + v101 + v110 + v111)/8; //#8
				h3dctrl[k2][j2+1][i2+3] = (tmp3dctrl[k-1][j][i] + tmp3dctrl[k-1][j][i+3] + tmp3dctrl[k-1][j+1][i] + tmp3dctrl[k-1][j+1][i+3] + v100 + v101 + v110 + v111 + 6*(v000 + v001 + v010 + v011))/32;//#7
				h3dctrl[k2+1][j2][i2+3] = (tmp3dctrl[k][j-1][i] + tmp3dctrl[k][j-1][i+3] + tmp3dctrl[k+1][j-1][i] + tmp3dctrl[k+1][j-1][i+3] + v010 + v011 + v110 + v111 + 6 * (v000 + v001 + v100 + v101))/32;//#6
				h3dctrl[k2+1][j2+1][i2] = (v001 + v011 + v101 + v111+ 6*(v000 + v010 + v100 + v110))/28;//#5
				h3dctrl[k2][j2][i2+3] = (tmp3dctrl[k-1][j-1][i]+tmp3dctrl[k-1][j-1][i+3]+tmp3dctrl[k-1][j+1][i]+tmp3dctrl[k-1][j+1][i+3]+ tmp3dctrl[k+1][j-1][i]+tmp3dctrl[k+1][j-1][i+3]+ v110 + v111+ 6*(tmp3dctrl[k][j-1][i] + tmp3dctrl[k][j-1][i+3] + v010 + v011+tmp3dctrl[k-1][j][i]+tmp3dctrl[k-1][j][i+3]+ v100 + v101) + 36*(v000 + v001))/128;//#4
				h3dctrl[k2][j2+1][i2] = (tmp3dctrl[k-1][j][i+3] +tmp3dctrl[k-1][j+1][i+3]+v101+v111 +6*(tmp3dctrl[k-1][j][i]+tmp3dctrl[k-1][j+1][i]+v100+v110+v001+v011) + 36*(v000 + v010))/112;//#3
				h3dctrl[k2+1][j2][i2] = (tmp3dctrl[k][j-1][i+3]+tmp3dctrl[k][j+1][i+3]+tmp3dctrl[k+1][j-1][i+3]+v111+6*(tmp3dctrl[k][j-1][i]+tmp3dctrl[k][j+1][i]+tmp3dctrl[k+1][j-1][i]+v110+tmp3dctrl[k][j][i+3]+v101)+36*(v100+v000))/112; //#2
				h3dctrl[k2][j2][i2] = (tmp3dctrl[k-1][j-1][i+3]+tmp3dctrl[k-1][j+1][i+3]+tmp3dctrl[k+1][j-1][i+3]+v111 +6*(tmp3dctrl[k-1][j-1][i]+tmp3dctrl[k-1][j][i+3]+tmp3dctrl[k-1][j+1][i]+tmp3dctrl[k][j-1][i+3]+v011+tmp3dctrl[k+1][j-1][i]+v101+v110)+36*(tmp3dctrl[k-1][j][i]+v100+tmp3dctrl[k][j-1][i]+v010+v001)+216*v000)/448;//1
			}
			for (i = 3; i < oldcnumx*3-3; i++)
			{
				if(i%3 ==0)i2 = i * 2; else i2++;
				v000 = tmp3dctrl[k][j][i]; v001 = tmp3dctrl[k][j][i+3];
				v010 = tmp3dctrl[k][j+1][i]; v011 = tmp3dctrl[k][j+1][i+3];
				v100 = tmp3dctrl[k+1][j][i]; v101 = tmp3dctrl[k+1][j][i+3];
				v110 = tmp3dctrl[k+1][j+1][i]; v111 = tmp3dctrl[k+1][j+1][i+3];
				h3dctrl[k2+1][j2+1][i2+3] = (v000 + v001 + v010 + v011 + v100 + v101 + v110 + v111)/8; //#8
				h3dctrl[k2][j2+1][i2+3] = (tmp3dctrl[k-1][j][i] + tmp3dctrl[k-1][j][i+3] + tmp3dctrl[k-1][j+1][i] + tmp3dctrl[k-1][j+1][i+3] + v100 + v101 + v110 + v111 + 6*(v000 + v001 + v010 + v011))/32;//#7
				h3dctrl[k2+1][j2][i2+3] = (tmp3dctrl[k][j-1][i] + tmp3dctrl[k][j-1][i+3] + tmp3dctrl[k+1][j-1][i] + tmp3dctrl[k+1][j-1][i+3] + v010 + v011 + v110 + v111 + 6 * (v000 + v001 + v100 + v101))/32;//#6
				h3dctrl[k2+1][j2+1][i2] = ( tmp3dctrl[k][j][i-3] + tmp3dctrl[k][j+1][i-3] + tmp3dctrl[k+1][j][i-3] + tmp3dctrl[k+1][j+1][i-3] + v001 + v011 + v101 + v111+ 6*(v000 + v010 + v100 + v110))/32;//#5
				h3dctrl[k2][j2][i2+3] = (tmp3dctrl[k-1][j-1][i]+tmp3dctrl[k-1][j-1][i+3]+tmp3dctrl[k-1][j+1][i]+tmp3dctrl[k-1][j+1][i+3]+ tmp3dctrl[k+1][j-1][i]+tmp3dctrl[k+1][j-1][i+3]+ v110 + v111+ 6*(tmp3dctrl[k][j-1][i] + tmp3dctrl[k][j-1][i+3] + v010 + v011+tmp3dctrl[k-1][j][i]+tmp3dctrl[k-1][j][i+3]+ v100 + v101) + 36*(v000 + v001))/128;//#4
				h3dctrl[k2][j2+1][i2] = (tmp3dctrl[k-1][j][i-3] + tmp3dctrl[k-1][j][i+3] + tmp3dctrl[k-1][j+1][i-3] +tmp3dctrl[k-1][j+1][i+3]+tmp3dctrl[k+1][j][i-3]+tmp3dctrl[k+1][j+1][i-3]+v101+v111 +6*(tmp3dctrl[k-1][j][i]+tmp3dctrl[k-1][j+1][i]+v100+v110+tmp3dctrl[k][j][i-3]+tmp3dctrl[k][j+1][i-3]+v001+v011) + 36*(v000 + v010))/128;//#3
				h3dctrl[k2+1][j2][i2] = (tmp3dctrl[k][j-1][i-3]+tmp3dctrl[k][j-1][i+3]+tmp3dctrl[k][j+1][i-3]+tmp3dctrl[k][j+1][i+3]+tmp3dctrl[k+1][j-1][i-3]+tmp3dctrl[k+1][j-1][i+3]+tmp3dctrl[k+1][j+1][i-3] +v111+6*(tmp3dctrl[k][j-1][i]+tmp3dctrl[k][j+1][i]+tmp3dctrl[k+1][j-1][i]+v110+tmp3dctrl[k][j][i-3]+tmp3dctrl[k][j][i+3]+tmp3dctrl[k+1][j][i-3]+v101)+36*(v100+v000))/128; //#2
				h3dctrl[k2][j2][i2] = (tmp3dctrl[k-1][j-1][i-3]+tmp3dctrl[k-1][j-1][i+3]+tmp3dctrl[k-1][j+1][i-3]+tmp3dctrl[k-1][j+1][i+3]+tmp3dctrl[k+1][j-1][i-3]+tmp3dctrl[k+1][j-1][i+3]+tmp3dctrl[k+1][j+1][i-3]+v111 +6*(tmp3dctrl[k-1][j-1][i]+tmp3dctrl[k-1][j][i-3]+tmp3dctrl[k-1][j][i+3]+tmp3dctrl[k-1][j+1][i]+tmp3dctrl[k][j-1][i-3]+tmp3dctrl[k][j-1][i+3]+tmp3dctrl[k][j+1][i-3]+v011+tmp3dctrl[k+1][j-1][i]+tmp3dctrl[k+1][j][i-3]+v101+v110)+36*(tmp3dctrl[k-1][j][i]+v100+tmp3dctrl[k][j-1][i]+tmp3dctrl[k][j][i-3]+v010+v001)+216*v000)/512;//1
			}
			for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
			{
				if(i%3 ==0)i2 = i * 2; else i2++;
				v000 = tmp3dctrl[k][j][i];
				v010 = tmp3dctrl[k][j+1][i];
				v100 = tmp3dctrl[k+1][j][i];
				v110 = tmp3dctrl[k+1][j+1][i];
				h3dctrl[k2+1][j2+1][i2] = (tmp3dctrl[k][j][i-3] + tmp3dctrl[k][j+1][i-3] + tmp3dctrl[k+1][j][i-3] + tmp3dctrl[k+1][j+1][i-3] + 6*(v000 + v010 + v100 + v110))/28;//#5
				h3dctrl[k2][j2+1][i2] = (tmp3dctrl[k-1][j][i-3] + tmp3dctrl[k-1][j+1][i-3]+tmp3dctrl[k+1][j][i-3]+tmp3dctrl[k+1][j+1][i-3] +6*(tmp3dctrl[k-1][j][i]+tmp3dctrl[k-1][j+1][i]+v100+v110+tmp3dctrl[k][j][i-3]+tmp3dctrl[k][j+1][i-3]) + 36*(v000 + v010))/112;//#3
				h3dctrl[k2+1][j2][i2] = (tmp3dctrl[k][j-1][i-3]+tmp3dctrl[k][j+1][i-3]+tmp3dctrl[k+1][j-1][i-3]+tmp3dctrl[k+1][j+1][i-3]+6*(tmp3dctrl[k][j-1][i]+tmp3dctrl[k][j+1][i]+tmp3dctrl[k+1][j-1][i]+v110+tmp3dctrl[k][j][i-3]+tmp3dctrl[k+1][j][i-3])+36*(v100+v000))/112; //#2
				h3dctrl[k2][j2][i2] = (tmp3dctrl[k-1][j-1][i-3]+tmp3dctrl[k-1][j+1][i-3]+tmp3dctrl[k+1][j-1][i-3]+tmp3dctrl[k+1][j+1][i-3]+6*(tmp3dctrl[k-1][j-1][i]+tmp3dctrl[k-1][j][i-3]+tmp3dctrl[k-1][j+1][i]+tmp3dctrl[k][j-1][i-3]+tmp3dctrl[k][j+1][i-3]+tmp3dctrl[k+1][j-1][i]+tmp3dctrl[k+1][j][i-3]+v110)+36*(tmp3dctrl[k-1][j][i]+v100+tmp3dctrl[k][j-1][i]+tmp3dctrl[k][j][i-3]+v010)+216*v000)/448;//1
			}
		}//j=[1,oldcnumy-1)
		//section 2.3
		j2 = 2*oldcnumy - 2;
		for (i=0; i<3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[k][oldcnumy-1][i]; v001 = tmp3dctrl[k][oldcnumy-1][i+3];
			v100 = tmp3dctrl[k+1][oldcnumy-1][i]; v101 = tmp3dctrl[k+1][oldcnumy-1][i+3];
			h3dctrl[k2+1][j2][i2+3] = (tmp3dctrl[k][oldcnumy-2][i] + tmp3dctrl[k][oldcnumy-2][i+3] + tmp3dctrl[k+1][oldcnumy-2][i] + tmp3dctrl[k+1][oldcnumy-2][i+3] + 6 * (v000 + v001 + v100 + v101))/28;//#6
			h3dctrl[k2][j2][i2+3] = (tmp3dctrl[k-1][oldcnumy-2][i]+tmp3dctrl[k-1][oldcnumy-2][i+3]+ tmp3dctrl[k+1][oldcnumy-2][i]+tmp3dctrl[k+1][oldcnumy-2][i+3]+ 6*(tmp3dctrl[k][oldcnumy-2][i] + tmp3dctrl[k][oldcnumy-2][i+3]+tmp3dctrl[k-1][oldcnumy-1][i]+tmp3dctrl[k-1][oldcnumy-1][i+3]+ v100 + v101) + 36*(v000 + v001))/112;//#4
			h3dctrl[k2+1][j2][i2] = (tmp3dctrl[k][oldcnumy-2][i+3]+tmp3dctrl[k+1][oldcnumy-2][i+3]+6*(tmp3dctrl[k][oldcnumy-2][i]+tmp3dctrl[k+1][oldcnumy-2][i]+tmp3dctrl[k][oldcnumy-1][i+3]+v101)+36*(v100+v000))/98; //#2
			h3dctrl[k2][j2][i2] = (tmp3dctrl[k-1][oldcnumy-2][i+3]+tmp3dctrl[k+1][oldcnumy-2][i+3] +6*(tmp3dctrl[k-1][oldcnumy-2][i]+tmp3dctrl[k-1][oldcnumy-1][i+3]+tmp3dctrl[k][oldcnumy-2][i+3]+tmp3dctrl[k+1][oldcnumy-2][i]+v101)+36*(tmp3dctrl[k-1][oldcnumy-1][i]+v100+tmp3dctrl[k][oldcnumy-2][i]+v001)+216*v000)/392;//1
		}
		for (i = 3; i < oldcnumx*3-3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[k][oldcnumy-1][i]; v001 = tmp3dctrl[k][oldcnumy-1][i+3];
			v100 = tmp3dctrl[k+1][oldcnumy-1][i]; v101 = tmp3dctrl[k+1][oldcnumy-1][i+3];
			h3dctrl[k2+1][j2][i2+3] = (tmp3dctrl[k][oldcnumy-2][i] + tmp3dctrl[k][oldcnumy-2][i+3] + tmp3dctrl[k+1][oldcnumy-2][i] + tmp3dctrl[k+1][oldcnumy-2][i+3] + 6 * (v000 + v001 + v100 + v101))/28;//#6
			h3dctrl[k2][j2][i2+3] = (tmp3dctrl[k-1][oldcnumy-2][i]+tmp3dctrl[k-1][oldcnumy-2][i+3]+ tmp3dctrl[k+1][oldcnumy-2][i]+tmp3dctrl[k+1][oldcnumy-2][i+3]+ 6*(tmp3dctrl[k][oldcnumy-2][i] + tmp3dctrl[k][oldcnumy-2][i+3]+tmp3dctrl[k-1][oldcnumy-1][i]+tmp3dctrl[k-1][oldcnumy-1][i+3]+ v100 + v101) + 36*(v000 + v001))/112;//#4
			h3dctrl[k2+1][j2][i2] = (tmp3dctrl[k][oldcnumy-2][i-3]+tmp3dctrl[k][oldcnumy-2][i+3]+tmp3dctrl[k+1][oldcnumy-2][i-3]+tmp3dctrl[k+1][oldcnumy-2][i+3]+6*(tmp3dctrl[k][oldcnumy-2][i]+tmp3dctrl[k+1][oldcnumy-2][i]+tmp3dctrl[k][oldcnumy-1][i-3]+tmp3dctrl[k][oldcnumy-1][i+3]+tmp3dctrl[k+1][oldcnumy-1][i-3]+v101)+36*(v100+v000))/112; //#2
			h3dctrl[k2][j2][i2] = (tmp3dctrl[k-1][oldcnumy-2][i-3]+tmp3dctrl[k-1][oldcnumy-2][i+3]+tmp3dctrl[k+1][oldcnumy-2][i-3]+tmp3dctrl[k+1][oldcnumy-2][i+3] +6*(tmp3dctrl[k-1][oldcnumy-2][i]+tmp3dctrl[k-1][oldcnumy-1][i-3]+tmp3dctrl[k-1][oldcnumy-1][i+3]+tmp3dctrl[k][oldcnumy-2][i-3]+tmp3dctrl[k][oldcnumy-2][i+3]+tmp3dctrl[k+1][oldcnumy-2][i]+tmp3dctrl[k+1][oldcnumy-1][i-3]+v101)+36*(tmp3dctrl[k-1][oldcnumy-1][i]+v100+tmp3dctrl[k][oldcnumy-2][i]+tmp3dctrl[k][oldcnumy-1][i-3]+v001)+216*v000)/448;//1
		}
		for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[k][oldcnumy-1][i];
			v100 = tmp3dctrl[k+1][oldcnumy-1][i];
			h3dctrl[k2+1][j2][i2] = (tmp3dctrl[k][oldcnumy-2][i-3]+tmp3dctrl[k+1][oldcnumy-2][i-3]+6*(tmp3dctrl[k][oldcnumy-2][i]+tmp3dctrl[k+1][oldcnumy-2][i]+tmp3dctrl[k][oldcnumy-1][i-3]+tmp3dctrl[k+1][oldcnumy-1][i-3])+36*(v100+v000))/98; //#2
			h3dctrl[k2][j2][i2] = (tmp3dctrl[k-1][oldcnumy-2][i-3]+tmp3dctrl[k+1][oldcnumy-2][i-3]+6*(tmp3dctrl[k-1][oldcnumy-2][i]+tmp3dctrl[k-1][oldcnumy-1][i-3]+tmp3dctrl[k][oldcnumy-2][i-3]+tmp3dctrl[k+1][oldcnumy-2][i]+tmp3dctrl[k+1][oldcnumy-1][i-3])+36*(tmp3dctrl[k-1][oldcnumy-1][i]+v100+tmp3dctrl[k][oldcnumy-2][i]+tmp3dctrl[k][oldcnumy-1][i-3])+216*v000)/392;//1
		}

	}//k=[1,oldcnumz-1)

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	k2 = 2*oldcnumz - 2;
	//section 3.1
	for (i=0; i<3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[oldcnumz-1][0][i]; v001 = tmp3dctrl[oldcnumz-1][0][i+3];
		v010 = tmp3dctrl[oldcnumz-1][1][i]; v011 = tmp3dctrl[oldcnumz-1][1][i+3];
		h3dctrl[k2][1][i2+3] = (tmp3dctrl[oldcnumz-2][0][i] + tmp3dctrl[oldcnumz-2][0][i+3] + tmp3dctrl[oldcnumz-2][1][i] + tmp3dctrl[oldcnumz-2][1][i+3] + 6*(v000 + v001 + v010 + v011))/28;//#7
		h3dctrl[k2][0][i2+3] = (tmp3dctrl[oldcnumz-2][1][i]+tmp3dctrl[oldcnumz-2][1][i+3]+ 6*(v010 + v011+tmp3dctrl[oldcnumz-2][0][i]+tmp3dctrl[oldcnumz-2][0][i+3]) + 36*(v000 + v001))/98;//#4
		h3dctrl[k2][1][i2] = (tmp3dctrl[oldcnumz-2][0][i+3] +tmp3dctrl[oldcnumz-2][1][i+3]+6*(tmp3dctrl[oldcnumz-2][0][i]+tmp3dctrl[oldcnumz-2][1][i]+v001+v011) + 36*(v000 + v010))/98;//#3
		h3dctrl[k2][0][i2] = (tmp3dctrl[oldcnumz-2][1][i+3]+6*(tmp3dctrl[oldcnumz-2][0][i+3]+tmp3dctrl[oldcnumz-2][1][i]+v011)+36*(tmp3dctrl[oldcnumz-2][0][i]+v010+v001)+216*v000)/343;//1
	}
	for (i = 3; i < oldcnumx*3-3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[oldcnumz-1][0][i]; v001 = tmp3dctrl[oldcnumz-1][0][i+3];
		v010 = tmp3dctrl[oldcnumz-1][1][i]; v011 = tmp3dctrl[oldcnumz-1][1][i+3];
		h3dctrl[k2][1][i2+3] = (tmp3dctrl[oldcnumz-2][0][i] + tmp3dctrl[oldcnumz-2][0][i+3] + tmp3dctrl[oldcnumz-2][1][i] + tmp3dctrl[oldcnumz-2][1][i+3] + 6*(v000 + v001 + v010 + v011))/28;//#7
		h3dctrl[k2][0][i2+3] = (tmp3dctrl[oldcnumz-2][1][i]+tmp3dctrl[oldcnumz-2][1][i+3]+ 6*(v010 + v011+tmp3dctrl[oldcnumz-2][0][i]+tmp3dctrl[oldcnumz-2][0][i+3]) + 36*(v000 + v001))/98;//#4
		h3dctrl[k2][1][i2] = (tmp3dctrl[oldcnumz-2][0][i-3] + tmp3dctrl[oldcnumz-2][0][i+3] + tmp3dctrl[oldcnumz-2][1][i-3] +tmp3dctrl[oldcnumz-2][1][i+3]+6*(tmp3dctrl[oldcnumz-2][0][i]+tmp3dctrl[oldcnumz-2][1][i]+tmp3dctrl[oldcnumz-1][0][i-3]+tmp3dctrl[oldcnumz-1][1][i-3]+v001+v011) + 36*(v000 + v010))/112;//#3
		h3dctrl[k2][0][i2] = (tmp3dctrl[oldcnumz-2][1][i-3]+tmp3dctrl[oldcnumz-2][1][i+3]+6*(tmp3dctrl[oldcnumz-2][0][i-3]+tmp3dctrl[oldcnumz-2][0][i+3]+tmp3dctrl[oldcnumz-2][1][i]+tmp3dctrl[oldcnumz-1][1][i-3]+v011)+36*(tmp3dctrl[oldcnumz-2][0][i]+tmp3dctrl[oldcnumz-1][0][i-3]+v010+v001)+216*v000)/392;//1
	}
	for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[oldcnumz-1][0][i];
		v010 = tmp3dctrl[oldcnumz-1][1][i]; 
		h3dctrl[k2][1][i2] = (tmp3dctrl[oldcnumz-2][0][i-3] + tmp3dctrl[oldcnumz-2][1][i-3] +6*(tmp3dctrl[oldcnumz-2][0][i]+tmp3dctrl[oldcnumz-2][1][i]+tmp3dctrl[oldcnumz-1][0][i-3]+tmp3dctrl[oldcnumz-1][1][i-3]) + 36*(v000 + v010))/98;//#3
		h3dctrl[k2][0][i2] = (tmp3dctrl[oldcnumz-2][1][i-3]+6*(tmp3dctrl[oldcnumz-2][0][i-3]+tmp3dctrl[oldcnumz-2][1][i]+tmp3dctrl[oldcnumz-1][1][i-3])+36*(tmp3dctrl[oldcnumz-2][0][i]+tmp3dctrl[oldcnumz-1][0][i-3]+v010)+216*v000)/343;//1
	}
	//section 3.2
	for(j = 1; j < oldcnumy-1; j++)
	{
		j2 = j*2;
		for (i=0; i<3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[oldcnumz-1][j][i]; v001 = tmp3dctrl[oldcnumz-1][j][i+3];
			v010 = tmp3dctrl[oldcnumz-1][j+1][i]; v011 = tmp3dctrl[oldcnumz-1][j+1][i+3];
			h3dctrl[k2][j2+1][i2+3] = (tmp3dctrl[oldcnumz-2][j][i] + tmp3dctrl[oldcnumz-2][j][i+3] + tmp3dctrl[oldcnumz-2][j+1][i] + tmp3dctrl[oldcnumz-2][j+1][i+3] + 6*(v000 + v001 + v010 + v011))/28;//#7
			h3dctrl[k2][j2][i2+3] = (tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j-1][i+3]+tmp3dctrl[oldcnumz-2][j+1][i]+tmp3dctrl[oldcnumz-2][j+1][i+3]+ 6*(tmp3dctrl[oldcnumz-1][j-1][i] + tmp3dctrl[oldcnumz-1][j-1][i+3] + v010 + v011+tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-2][j][i+3]) + 36*(v000 + v001))/112;//#4
			h3dctrl[k2][j2+1][i2] = (tmp3dctrl[oldcnumz-2][j][i+3] +tmp3dctrl[oldcnumz-2][j+1][i+3]+6*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-2][j+1][i]+v001+v011) + 36*(v000 + v010))/98;//#3
			h3dctrl[k2][j2][i2] = (tmp3dctrl[oldcnumz-2][j-1][i+3]+tmp3dctrl[oldcnumz-2][j+1][i+3]+6*(tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j][i+3]+tmp3dctrl[oldcnumz-2][j+1][i]+tmp3dctrl[oldcnumz-1][j-1][i+3]+v011)+36*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-1][j-1][i]+v010+v001)+216*v000)/392;//1
		}
		for (i = 3; i < oldcnumx*3-3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[oldcnumz-1][j][i]; v001 = tmp3dctrl[oldcnumz-1][j][i+3];
			v010 = tmp3dctrl[oldcnumz-1][j+1][i]; v011 = tmp3dctrl[oldcnumz-1][j+1][i+3];
			h3dctrl[k2][j2+1][i2+3] = (tmp3dctrl[oldcnumz-2][j][i] + tmp3dctrl[oldcnumz-2][j][i+3] + tmp3dctrl[oldcnumz-2][j+1][i] + tmp3dctrl[oldcnumz-2][j+1][i+3] + 6*(v000 + v001 + v010 + v011))/28;//#7
			h3dctrl[k2][j2][i2+3] = (tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j-1][i+3]+tmp3dctrl[oldcnumz-2][j+1][i]+tmp3dctrl[oldcnumz-2][j+1][i+3]+ 6*(tmp3dctrl[oldcnumz-1][j-1][i] + tmp3dctrl[oldcnumz-1][j-1][i+3] + v010 + v011+tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-2][j][i+3]) + 36*(v000 + v001))/112;//#4
			h3dctrl[k2][j2+1][i2] = (tmp3dctrl[oldcnumz-2][j][i-3] + tmp3dctrl[oldcnumz-2][j][i+3] + tmp3dctrl[oldcnumz-2][j+1][i-3] +tmp3dctrl[oldcnumz-2][j+1][i+3]+6*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-2][j+1][i]+tmp3dctrl[oldcnumz-1][j][i-3]+tmp3dctrl[oldcnumz-1][j+1][i-3]+v001+v011) + 36*(v000 + v010))/112;//#3
			h3dctrl[k2][j2][i2] = (tmp3dctrl[oldcnumz-2][j-1][i-3]+tmp3dctrl[oldcnumz-2][j-1][i+3]+tmp3dctrl[oldcnumz-2][j+1][i-3]+tmp3dctrl[oldcnumz-2][j+1][i+3]+6*(tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j][i-3]+tmp3dctrl[oldcnumz-2][j][i+3]+tmp3dctrl[oldcnumz-2][j+1][i]+tmp3dctrl[oldcnumz-1][j-1][i-3]+tmp3dctrl[oldcnumz-1][j-1][i+3]+tmp3dctrl[oldcnumz-1][j+1][i-3]+v011)+36*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-1][j-1][i]+tmp3dctrl[oldcnumz-1][j][i-3]+v010+v001)+216*v000)/448;//1
		}
		for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
		{
			if(i%3 ==0)i2 = i * 2; else i2++;
			v000 = tmp3dctrl[oldcnumz-1][j][i];
			v010 = tmp3dctrl[oldcnumz-1][j+1][i];
			h3dctrl[k2][j2+1][i2] = (tmp3dctrl[oldcnumz-2][j][i-3]+ tmp3dctrl[oldcnumz-2][j+1][i-3]+6*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-2][j+1][i]+tmp3dctrl[oldcnumz-1][j][i-3]+tmp3dctrl[oldcnumz-1][j+1][i-3]) + 36*(v000 + v010))/98;//#3
			h3dctrl[k2][j2][i2] = (tmp3dctrl[oldcnumz-2][j-1][i-3]+tmp3dctrl[oldcnumz-2][j+1][i-3]+6*(tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j][i-3]+tmp3dctrl[oldcnumz-2][j+1][i]+tmp3dctrl[oldcnumz-1][j-1][i-3]+tmp3dctrl[oldcnumz-1][j+1][i-3])+36*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-1][j-1][i]+tmp3dctrl[oldcnumz-1][j][i-3]+v010)+216*v000)/392;//1
		}
	}
	//section 3.3
	j2 = 2*oldcnumy - 2;
	for (i=0; i<3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[oldcnumz-1][oldcnumy-1][i]; v001 = tmp3dctrl[oldcnumz-1][oldcnumy-1][i+3];
		h3dctrl[k2][j2][i2+3] = (tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j-1][i+3]+ 6*(tmp3dctrl[oldcnumz-1][j-1][i] + tmp3dctrl[oldcnumz-1][j-1][i+3] +tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-2][j][i+3]) + 36*(v000 + v001))/98;//#4
		h3dctrl[k2][j2][i2] = (tmp3dctrl[oldcnumz-2][j-1][i+3]+6*(tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j][i+3]+tmp3dctrl[oldcnumz-1][j-1][i+3])+36*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-1][j-1][i]+v001)+216*v000)/343;//1
	}
	for (i = 3; i < oldcnumx*3-3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[oldcnumz-1][oldcnumy-1][i]; v001 = tmp3dctrl[oldcnumz-1][oldcnumy-1][i+3];
		h3dctrl[k2][j2][i2+3] = (tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j-1][i+3]+ 6*(tmp3dctrl[oldcnumz-1][j-1][i] + tmp3dctrl[oldcnumz-1][j-1][i+3] +tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-2][j][i+3]) + 36*(v000 + v001))/98;//#4
		h3dctrl[k2][j2][i2] = (tmp3dctrl[oldcnumz-2][j-1][i-3]+tmp3dctrl[oldcnumz-2][j-1][i+3]+6*(tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j][i-3]+tmp3dctrl[oldcnumz-2][j][i+3]+tmp3dctrl[oldcnumz-1][j-1][i-3]+tmp3dctrl[oldcnumz-1][j-1][i+3])+36*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-1][j-1][i]+tmp3dctrl[oldcnumz-1][j][i-3]+v001)+216*v000)/392;//1
	}
	for (i = oldcnumx*3-3; i < oldcnumx*3; i++)
	{
		if(i%3 ==0)i2 = i * 2; else i2++;
		v000 = tmp3dctrl[oldcnumz-1][oldcnumy-1][i];
		h3dctrl[k2][j2][i2] = (tmp3dctrl[oldcnumz-2][j-1][i-3]+6*(tmp3dctrl[oldcnumz-2][j-1][i]+tmp3dctrl[oldcnumz-2][j][i-3]+tmp3dctrl[oldcnumz-1][j-1][i-3])+36*(tmp3dctrl[oldcnumz-2][j][i]+tmp3dctrl[oldcnumz-1][j-1][i]+tmp3dctrl[oldcnumz-1][j][i-3])+216*v000)/343;//1
	}
	free3dfloat(tmp3dctrl, oldcnumz, oldcnumy);
	cx3 = cnumx * 3;
	llength = cx3 * cnumy;
	for (k = 0; k < cnumz; k++)
	{
		i2 = k*llength;
		for(j = 0; j < cnumy; j++)
		{
			j2 = i2 + j* cx3;
			for (i = 0; i < cx3; i++)
			{
				newctrl[j2+i] = h3dctrl[k][j][i];
			}
		}
	}
	free3dfloat(h3dctrl, z2, y2);
}



/*****************************************************************************
 * FUNCTION: dMIoptimizeall
 * DESCRIPTION: Optimize the control points by calculating derivative of MI over whole image
 * PARAMETERS:
 *    cnumx, cnumy, cnumz: control point resolution
 *    multin1: the input 3-d test image
 *    multin2: the reference image
 *    spacex, spacey, spacez: cordinates spacing of control mesh
 *    xdim1, ydim1, zdim1: the dimensions of the test image
 *    xdim2, ydim2, zdim2: the dimensions of the reference image
 *    itr: numbers of iterations in opitimizing iterations
 *    steps: user defined decaying constant
 *    RBINNUM, TBINNUM: numbers of histogram bins in test image and reference image
 *    fR0, fT0: minimum intensity in test image and reference image
 *    fRmax, fTmax: maximum intensity in test image and reference image
 *    binR, binT: histogram bin width in test image and reference image
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    tmp3dctrl: input and optimized output 3D control point mesh
 * EXIT CONDITIONS:
 *****************************************************************************/
void dMIoptimizeall1d(tmp3dctrl, cnumx, cnumy, cnumz, multin1, multin2, spacex, spacey, spacez, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2,itra, steps, RBINNUM, TBINNUM)
	float *tmp3dctrl;
	int cnumx, cnumy, cnumz;
	float *multin1, *multin2;
	float spacex, spacey, spacez;
	int xdim1, ydim1, zdim1, xdim2, ydim2, zdim2,itra, steps, RBINNUM, TBINNUM;
{
	int i, j, k, i3, kz, jy, ix, l, lx,rx,ly,ry,lz,rz, kl, kr, ll, lr, kz0, jy0, ix0, itr;
	int oldx, oldy, oldz, hlength;
	float *pt;
	float **plk;
	float *****dplk;
	long vsize1, vsize2;
	float newx, newy, newz, ci, b3x, b3y, b3z, cbx, byz, fsum, dfxsum, dfysum, dfzsum, kparam, lparam, b0k, b3l, db3l, logtmp, Bts, ibts, dplkxsum, dplkysum, dplkzsum, fR0, fT0, fRmax, fTmax, binR, binT, ftmp;

	hlength = xdim1*ydim1; vsize1 = hlength * zdim1;vsize2  = xdim2*ydim2*zdim2;

	fT0 = 65535; fTmax = 0;
        for (i=0; i<vsize1; i++) { ftmp = multin1[i];  if (ftmp < fT0) fT0 = ftmp;  if (ftmp > fTmax)  fTmax= ftmp; }
        fR0 = 65535; fRmax = 0;
        for (i=0; i<vsize2; i++) { ftmp = multin2[i];  if (ftmp < fR0) fR0 = ftmp; if (ftmp > fRmax) fRmax= ftmp; }
        binR = (fRmax - fR0+1)/RBINNUM;  binT = (fTmax - fT0+1)/TBINNUM;

	dplk = (float *****)malloc(TBINNUM*sizeof(float****));
	i3 = cnumx*3;
	for(l = 0; l < TBINNUM; l++)
	{
		dplk[l] = (float****)malloc(RBINNUM*sizeof(float***));
		for(k = 0; k < RBINNUM; k++)
		{
			dplk[l][k] = (float***)malloc(cnumz*sizeof(float**));
			for(kz=0; kz<cnumz; kz++)
			{
				dplk[l][k][kz] = (float**)malloc(cnumy*sizeof(float*));
				for(jy=0; jy<cnumy; jy++)
					dplk[l][k][kz][jy] = (float*)malloc(i3*sizeof(float));
			}
		}
	}

	plk = (float **)malloc(TBINNUM * sizeof(float *));
	for(j = 0; j < TBINNUM; j++)
		plk[j] = (float *)malloc(RBINNUM * sizeof(float));
	pt = (float *)malloc(TBINNUM * sizeof(float));


	Bts = (float)steps/(binT*vsize1);
	for (itr = 1; itr < itra; itr++) 
	{
		for (l = 0; l < TBINNUM; l++)
		{
			pt[l] = 0;
			for ( k = 0; k < RBINNUM; k++)
			{
				plk[l][k] = 0;
				for(kz = 0; kz<cnumz; kz++)
				{
					for(jy = 0; jy<cnumy; jy++)
					{
						for(ix = 0; ix<cnumx; ix++)
						{
							dplk[l][k][kz][jy][ix*3] = 0; dplk[l][k][kz][jy][ix*3+1] = 0; dplk[l][k][kz][jy][ix*3+2] =0;
						}
					}
				}
			}
		}
		for (oldz=0; oldz<zdim1; oldz++) for(oldy=0; oldy<ydim1; oldy++) for(oldx=0; oldx<xdim1; oldx++)
		{
			bspline_3d16_1d(tmp3dctrl, spacex, spacey, spacez, cnumx, cnumy, cnumz, oldx, oldy, oldz, &newx, &newy, &newz);
			//got new position. to interpolate the intensity, using mirror boundary conditions to minimize border artifacts
			lz = (int)floor(newz)-1;
			rz = (int)ceil(newz)+2;
			ly = (int)floor(newy)-1;
			ry = (int)ceil(newy)+2;
			lx = (int)floor(newx)-1;
			rx = (int)ceil(newx)+2;
			fsum = 0;
			dfxsum = 0; dfysum = 0; dfzsum = 0;
			for(kz=lz; kz<rz; kz++)
			{
				kz0 = kz; if (kz<0) kz0 = abs(kz); if(kz>=zdim1) kz0 = zdim1*2 - 2 - kz; if (kz0<0) kz0 = abs(kz0); ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!//////////////if zdim1=1 or 2
				for(jy=ly; jy<ry; jy++)
				{
					jy0 = jy; if (jy<0) jy0 = abs(jy); if(jy>=ydim1) jy0 = ydim1*2 - 2 - jy;
					for(ix=lx; ix<rx; ix++)
					{
						ix0 = ix; if (ix<0) ix0 = abs(ix); if(ix>=xdim1) ix0 = xdim1*2 - 2 - ix;
						ci = multin1[kz0*hlength+jy0*xdim1+ix0];
						b3x = B3(newx-ix); b3y = B3(newy-jy); b3z = B3(newz-kz); cbx = ci*b3x; byz = b3y*b3z;
						fsum = cbx*byz + fsum;//sum will be interpolated intensity by rescaled bspline coefficients
						dfxsum = ci*dB3(newx-ix)*byz + dfxsum;
						dfysum = cbx*dB3(newy-jy)*b3z + dfysum;
						dfzsum = cbx*b3y*dB3(newz-kz) + dfzsum;
					}
				}
			}
			kparam = (multin2[oldz*xdim2*ydim2+oldy*xdim2+oldx] - fR0)/binR;//////////////////in2 out of border
			lparam = (fsum - fT0)/binT; /////////////////////////////fsum out of border
			/////////////ignored the part out of bin borders ***************
			kl = (int)ceil(kparam - 0.5); if(kl<0) kl = 0;//if(kl<0) {kl = 0; printf("kl%f,", kparam);}
			kr = (int)floor(kparam + 0.5);if(kr > RBINNUM-1) kr = RBINNUM-1;//if(kr > RBINNUM-1) {kr = RBINNUM-1;printf("kr%f,", kparam);}
			ll = (int)floor(lparam - 1); if(ll<0) ll=0;//if(ll<0) {ll=0;printf("ll%f,",lparam);}
			lr = (int)ceil(lparam + 2); if(lr > TBINNUM) lr = TBINNUM;//if(lr > TBINNUM) {lr = TBINNUM;printf("lr%f,",lparam);}
			for (l = ll; l < lr; l++)
			{
				for ( k = kl; k <= kr; k++)
				{
					b0k = B0(k-kparam); b3l = B3(l-lparam); db3l = dB3(l-lparam)*b0k;
					plk[l][k] = plk[l][k] + b0k*b3l;
					pt[l] = pt[l] + b0k*b3l;///////////////////////cut down the redundent multpile*************
					//loop on 4(or 3)x4x4 control points which got influence from the sampling voxel
					lx = (int)floor(oldx/spacex)-1; if (lx < 0) lx = 0;
					rx = (int)ceil(oldx/spacex)+2; if (rx > cnumx) rx = cnumx;
					ly = (int)floor(oldy/spacey)-1; if (ly < 0) ly = 0;
					ry = (int)ceil(oldy/spacey)+2; if (ry > cnumy) ry = cnumy;
					lz = (int)floor(oldz/spacez)-1; if (lz < 0) lz = 0;
					rz = (int)ceil(oldz/spacez)+2;  if (rz > cnumz) rz = cnumz;
					for(kz = lz; kz<rz; kz++)
					{
						for(jy = ly; jy<ry; jy++)
						{
							for(ix = lx; ix<rx; ix++)
							{
								dplk[l][k][kz][jy][ix*3] = dplk[l][k][kz][jy][ix*3] + db3l*dfxsum*B3(oldx/spacex-ix);
								dplk[l][k][kz][jy][ix*3+1] = dplk[l][k][kz][jy][ix*3+1] + db3l*dfysum*B3(oldy/spacey-jy);
								dplk[l][k][kz][jy][ix*3+2] = dplk[l][k][kz][jy][ix*3+2] + db3l*dfzsum*B3(oldz/spacez-kz);
							}
						}
					}
				}
			} //done with loop on dplk
		}//done with loop on subsampled voxels
		for (l = 0; l < TBINNUM; l++)//////////////////////////deal with plk[l][k] or pt[l] = 0;
		{
			if (pt[l]!= 0)
			{
				for (k = 0; k < RBINNUM; k++)
				{
					plk[l][k] = plk[l][k]/pt[l];
				}
			}
		}
		ibts = Bts/powf((float)(itr+51), (float)0.602);
		for(kz = 0; kz<cnumz; kz++)
		{
			ll = kz*cnumx*cnumy;
			for(jy = 0; jy<cnumy; jy++)
			{
				j = ll + jy*cnumx;
				for(ix = 0; ix<cnumx; ix++)
				{
					i3 = (j + ix) * 3;
					dplkxsum = 0;dplkysum = 0;dplkzsum = 0;
					for (l = 0; l < TBINNUM; l++)
					{
						for ( k = 0; k < RBINNUM; k++)
						{
							if(plk[l][k]!= 0)//////////////
							{
								logtmp = log10f(plk[l][k])*LOG2;
								dplkxsum = dplkxsum + dplk[l][k][kz][jy][ix*3]*logtmp;
								dplkysum = dplkysum + dplk[l][k][kz][jy][ix*3+1]*logtmp;
								dplkzsum = dplkzsum + dplk[l][k][kz][jy][ix*3+2]*logtmp;
							}
						}
					}
					tmp3dctrl[i3] = tmp3dctrl[i3] -dplkxsum*ibts;
					i3++; tmp3dctrl[i3] = tmp3dctrl[i3]-dplkysum*ibts;
					i3++; tmp3dctrl[i3] = tmp3dctrl[i3] -dplkzsum*ibts;
				}
			}
		}
	} //done with iterate trials


	free(pt);
	free2dfloat(plk, TBINNUM);
	free5dfloat(dplk,TBINNUM,RBINNUM,cnumz,cnumy);

}

void dMIoptimize1d(tmp3dctrl, cnumx, cnumy, cnumz, multin1, multin2, spacex, spacey, spacez, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2,itra, steps, RBINNUM, TBINNUM, SAMPLENUM)
        float *tmp3dctrl;
        int cnumx, cnumy, cnumz;
        float *multin1;
	float *multin2;
        float spacex, spacey, spacez;
        int xdim1, ydim1, zdim1, xdim2, ydim2, zdim2,itra, steps, RBINNUM, TBINNUM, SAMPLENUM;
{
        int i, j, k, i3, kz, jy, ix, l, sami, lx,rx,ly,ry,lz,rz, kl, kr, ll, lr, kz0, jy0, ix0, itr, vsize1,vsize2, randtmp;
        int oldx, oldy, oldz, hlength, ipos;
        float *pt;
        float **plk;
        float *****dplk;
        float newx, newy, newz, ci, b3x, b3y, b3z, cbx, byz, fsum, dfxsum, dfysum, dfzsum, kparam, lparam, b0k, b3l, db3l, logtmp, Bts, ibts, dplkxsum, dplkysum, dplkzsum, fR0, fT0, fRmax, fTmax, binR, binT, ftmp;
        div_t zpos, xypos;

        hlength = xdim1*ydim1;  vsize1 = hlength * zdim1; vsize2 = xdim2*ydim2*zdim2;
        fT0 = 65535.0; fTmax = 0;
        for (i=0; i<vsize1; i++) { ftmp = multin1[i];  if (ftmp < fT0) fT0 = ftmp;  if (ftmp > fTmax)  fTmax= ftmp; }
        fR0 = 65535.0; fRmax = 0;
        for (i=0; i<vsize2; i++) { ftmp = multin2[i];  if (ftmp < fR0) fR0 = ftmp; if (ftmp > fRmax) fRmax= ftmp; }
        binR = (fRmax - fR0+1)/RBINNUM;  binT = (fTmax - fT0+1)/TBINNUM;

	dplk = (float *****)malloc(TBINNUM*sizeof(float****));
	i3 = cnumx*3;
	for(l = 0; l < TBINNUM; l++)
	{
		dplk[l] = (float****)malloc(RBINNUM*sizeof(float***));
		for(k = 0; k < RBINNUM; k++)
		{
			dplk[l][k] = (float***)malloc(cnumz*sizeof(float**));
			for(kz=0; kz<cnumz; kz++)
			{
				dplk[l][k][kz] = (float**)malloc(cnumy*sizeof(float*));
				for(jy=0; jy<cnumy; jy++)
					dplk[l][k][kz][jy] = (float*)malloc(i3*sizeof(float));
			}
		}
	}

	plk = (float **)malloc(TBINNUM * sizeof(float *));
	for(j = 0; j < TBINNUM; j++)
		plk[j] = (float *)malloc(RBINNUM * sizeof(float));
	pt = (float *)malloc(TBINNUM * sizeof(float));

	Bts = (float)steps/(binT*SAMPLENUM);
	for (itr = 1; itr < itra; itr++) 
	{
		for (l = 0; l < TBINNUM; l++)
		{
			pt[l] = 0;
			for ( k = 0; k < RBINNUM; k++)
			{
				plk[l][k] = 0;
				for(kz = 0; kz<cnumz; kz++)
				{
					for(jy = 0; jy<cnumy; jy++)
					{
						for(ix = 0; ix<cnumx; ix++)
						{
							dplk[l][k][kz][jy][ix*3] = 0; dplk[l][k][kz][jy][ix*3+1] = 0; dplk[l][k][kz][jy][ix*3+2] =0;
						}
					}
				}
			}
		}
		for (sami=0; sami<SAMPLENUM; sami++)//loop on subsampled voxels
		{
			randtmp = rand() % vsize1;//using random voxels
			zpos = div(randtmp, hlength);
			oldz = zpos.quot;
			xypos = div(zpos.rem, xdim1);
			oldy = xypos.quot;
			oldx = xypos.rem; 
			bspline_3d16_1d(tmp3dctrl, spacex, spacey, spacez, cnumx, cnumy, cnumz, oldx, oldy, oldz, &newx, &newy, &newz);
			//got new position. to interpolate the intensity, using mirror boundary conditions to minimize border artifacts
			lz = (int)floor(newz)-1;
			rz = (int)ceil(newz)+2;
			ly = (int)floor(newy)-1;
			ry = (int)ceil(newy)+2;
			lx = (int)floor(newx)-1;
			rx = (int)ceil(newx)+2;
			fsum = 0;
			dfxsum = 0; dfysum = 0; dfzsum = 0;
			for(kz=lz; kz<rz; kz++)
			{
				kz0 = kz; if (kz<0) kz0 = abs(kz); if(kz>=zdim1) kz0 = zdim1*2 - 2 - kz; if (kz0<0) kz0 = abs(kz0); ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!//////////////if zdim1=1 or 2
				for(jy=ly; jy<ry; jy++)
				{
					jy0 = jy; if (jy<0) jy0 = abs(jy); if(jy>=ydim1) jy0 = ydim1*2 - 2 - jy;
					for(ix=lx; ix<rx; ix++)
					{
						ix0 = ix; if (ix<0) ix0 = abs(ix); if(ix>=xdim1) ix0 = xdim1*2 - 2 - ix;
						ci = multin1[kz0*hlength+jy0*xdim1+ix0];//multin1[kz0][jy0][ix0];//multin1[j+ix0];
						b3x = B3(newx-ix); b3y = B3(newy-jy); b3z = B3(newz-kz); cbx = ci*b3x; byz = b3y*b3z;
						fsum = cbx*byz + fsum;//sum will be interpolated intensity by rescaled bspline coefficients
						dfxsum = ci*dB3(newx-ix)*byz + dfxsum;
						dfysum = cbx*dB3(newy-jy)*b3z + dfysum;
						dfzsum = cbx*b3y*dB3(newz-kz) + dfzsum;
					}
				}
			}

			kparam = (multin2[randtmp] - fR0)/binR;//(in2[oldz][oldy][oldx] - fR0)/binR;///////////////////////////////////in2 out of border
			lparam = (fsum - fT0)/binT; /////////////////////////////fsum out of border
			/////////////ignored the part out of bin borders ***************
			kl = (int)ceil(kparam - 0.5); if(kl<0) kl = 0;//if(kl<0) {kl = 0; printf("kl%f,", kparam);}
			kr = (int)floor(kparam + 0.5);if(kr > RBINNUM-1) kr = RBINNUM-1;//if(kr > RBINNUM-1) {kr = RBINNUM-1;printf("kr%f,", kparam);}
			ll = (int)floor(lparam - 1); if(ll<0) ll=0;//if(ll<0) {ll=0;printf("ll%f,",lparam);}
			lr = (int)ceil(lparam + 2); if(lr > TBINNUM) lr = TBINNUM;//if(lr > TBINNUM) {lr = TBINNUM;printf("lr%f,",lparam);}

			for (l = ll; l < lr; l++)
			{
				for ( k = kl; k <= kr; k++)
				{
					b0k = B0(k-kparam); b3l = B3(l-lparam); db3l = dB3(l-lparam)*b0k;
					plk[l][k] = plk[l][k] + b0k*b3l;
					pt[l] = pt[l] + b0k*b3l;///////////////////////cut down the redundent multpile*************
					//loop on 4(or 3)x4x4 control points which got influence from the sampling voxel
					lx = (int)floor(oldx/spacex)-1; if (lx < 0) lx = 0;
					rx = (int)ceil(oldx/spacex)+2; if (rx > cnumx) rx = cnumx;
					ly = (int)floor(oldy/spacey)-1; if (ly < 0) ly = 0;
					ry = (int)ceil(oldy/spacey)+2; if (ry > cnumy) ry = cnumy;
					lz = (int)floor(oldz/spacez)-1; if (lz < 0) lz = 0;
					rz = (int)ceil(oldz/spacez)+2;  if (rz > cnumz) rz = cnumz;
					for(kz = lz; kz<rz; kz++)
					{
						for(jy = ly; jy<ry; jy++)
						{
							for(ix = lx; ix<rx; ix++)
							{
								dplk[l][k][kz][jy][ix*3] = dplk[l][k][kz][jy][ix*3] + db3l*dfxsum*B3(oldx/spacex-ix);
								dplk[l][k][kz][jy][ix*3+1] = dplk[l][k][kz][jy][ix*3+1] + db3l*dfysum*B3(oldy/spacey-jy);
								dplk[l][k][kz][jy][ix*3+2] = dplk[l][k][kz][jy][ix*3+2] + db3l*dfzsum*B3(oldz/spacez-kz);
							}
						}
					}
				}
			} //done with loop on dplk
		}//done with loop on subsampled voxels
		for (l = 0; l < TBINNUM; l++)//////////////////////////deal with plk[l][k] or pt[l] = 0;
		{
			if (pt[l]!= 0)
			{
				for (k = 0; k < RBINNUM; k++)
				{
					plk[l][k] = plk[l][k]/pt[l];
				}
			}
		}
		ibts = Bts/powf((float)(itr+51), (float)0.602); 
		for(kz = 0; kz<cnumz; kz++)
		{
			for(jy = 0; jy<cnumy; jy++)
			{
				for(ix = 0; ix<cnumx; ix++)
				{
					dplkxsum = 0;dplkysum = 0;dplkzsum = 0;
					for (l = 0; l < TBINNUM; l++)
					{
						for ( k = 0; k < RBINNUM; k++)
						{
							if(plk[l][k]!= 0)//////////////
							{
								logtmp = log10f(plk[l][k])*LOG2;
								dplkxsum = dplkxsum + dplk[l][k][kz][jy][ix*3]*logtmp;
								dplkysum = dplkysum + dplk[l][k][kz][jy][ix*3+1]*logtmp;
								dplkzsum = dplkzsum + dplk[l][k][kz][jy][ix*3+2]*logtmp;
							}
						}
					} 
					ipos = ((kz*cnumy+jy)*cnumx+ix)*3;
					tmp3dctrl[ipos] = tmp3dctrl[ipos] -dplkxsum*ibts;
					ipos++; tmp3dctrl[ipos] = tmp3dctrl[ipos]-dplkysum*ibts;
					ipos++; tmp3dctrl[ipos] = tmp3dctrl[ipos] -dplkzsum*ibts;
				}
			}
		}

	} //done with iterate trials

	free(pt);
	free2dfloat(plk, TBINNUM);
	free5dfloat(dplk,TBINNUM,RBINNUM,cnumz,cnumy);

}




void dMIoptimize1dif(tmp3dctrl, cnumx, cnumy, cnumz, multin1, multin2, spacex, spacey, spacez, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2,itra, steps, RBINNUM, TBINNUM, SAMPLENUM)
        float *tmp3dctrl;
        int cnumx, cnumy, cnumz;
        float *multin1;
	unsigned short *multin2;
        float spacex, spacey, spacez;
        int xdim1, ydim1, zdim1, xdim2, ydim2, zdim2,itra, steps, RBINNUM, TBINNUM, SAMPLENUM;
{
        int i, j, k, i3, kz, jy, ix, l, sami, lx,rx,ly,ry,lz,rz, kl, kr, ll, lr, kz0, jy0, ix0, itr, vsize1,vsize2, randtmp;
        int oldx, oldy, oldz, hlength;
        float *pt;
        float **plk;
        float *****dplk;
        float newx, newy, newz, ci, b3x, b3y, b3z, cbx, byz, fsum, dfxsum, dfysum, dfzsum, kparam, lparam, b0k, b3l, db3l, logtmp, Bts, ibts, dplkxsum, dplkysum, dplkzsum, fR0, fT0, fRmax, fTmax, binR, binT, ftmp;
        div_t zpos, xypos;

        hlength = xdim1*ydim1;  vsize1 = hlength * zdim1; vsize2 = xdim2*ydim2*zdim2;
        fT0 = 65535; fTmax = 0;
        for (i=0; i<vsize1; i++) { ftmp = multin1[i];  if (ftmp < fT0) fT0 = ftmp;  if (ftmp > fTmax)  fTmax= ftmp; }
        fR0 = 65535; fRmax = 0;
        for (i=0; i<vsize2; i++) { ftmp = (float)multin2[i];  if (ftmp < fR0) fR0 = ftmp; if (ftmp > fRmax) fRmax= ftmp; }
        binR = (fRmax - fR0+1)/RBINNUM;  binT = (fTmax - fT0+1)/TBINNUM;


        dplk = (float *****)malloc(TBINNUM*sizeof(float****));
        i3 = cnumx*3;
        for(l = 0; l < TBINNUM; l++)
        {
                dplk[l] = (float****)malloc(RBINNUM*sizeof(float***));
                for(k = 0; k < RBINNUM; k++)
                {
                        dplk[l][k] = (float***)malloc(cnumz*sizeof(float**));
                        for(kz=0; kz<cnumz; kz++)
                        {
                                dplk[l][k][kz] = (float**)malloc(cnumy*sizeof(float*));
                                for(jy=0; jy<cnumy; jy++)
                                        dplk[l][k][kz][jy] = (float*)malloc(i3*sizeof(float));
                        }
                }
        }

        plk = (float **)malloc(TBINNUM * sizeof(float *));
        for(j = 0; j < TBINNUM; j++)
                plk[j] = (float *)malloc(RBINNUM * sizeof(float));
        pt = (float *)malloc(TBINNUM * sizeof(float));

        Bts = (float)steps/(binT*SAMPLENUM);
        for (itr = 1; itr < itra; itr++)
        {
                for (l = 0; l < TBINNUM; l++)
                {
                        pt[l] = 0;
                        for ( k = 0; k < RBINNUM; k++)
                        {
                                plk[l][k] = 0;
                                for(kz = 0; kz<cnumz; kz++)
                                {
                                        for(jy = 0; jy<cnumy; jy++)
                                        {
                                                for(ix = 0; ix<cnumx; ix++)
                                                {
                                                        dplk[l][k][kz][jy][ix*3] = 0; dplk[l][k][kz][jy][ix*3+1] = 0; dplk[l][k][kz][jy][ix*3+2] =0;
                                                }
                                        }
                                }
                        }
                }
                for (sami=0; sami<SAMPLENUM; sami++)//loop on subsampled voxels
                {
                        randtmp = rand() % vsize1; //using random voxels
                        zpos = div(randtmp, hlength);
                        oldz = zpos.quot;
                        xypos = div(zpos.rem, xdim1);
                        oldy = xypos.quot;
                        oldx = xypos.rem;
                        bspline_3d16_1d(tmp3dctrl, spacex, spacey, spacez, cnumx, cnumy, cnumz, oldx, oldy, oldz, &newx, &newy, &newz);
                        //got new position. to interpolate the intensity, using mirror boundary conditions to minimize border artifacts
                        lz = (int)floor(newz)-1;
                        rz = (int)ceil(newz)+2;
                        ly = (int)floor(newy)-1;
                        ry = (int)ceil(newy)+2;
                        lx = (int)floor(newx)-1;
                        rx = (int)ceil(newx)+2;
                        fsum = 0;
                        dfxsum = 0; dfysum = 0; dfzsum = 0;
                        for(kz=lz; kz<rz; kz++)
                        {
                                kz0 = kz; if (kz<0) kz0 = abs(kz); if(kz>=zdim1) kz0 = zdim1*2 - 2 - kz; if (kz0<0) kz0 = abs(kz0); ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!//////////////if zdim1=1 or 2
				k = kz0*hlength;
                                for(jy=ly; jy<ry; jy++)
                                {
                                        jy0 = jy; if (jy<0) jy0 = abs(jy); if(jy>=ydim1) jy0 = ydim1*2 - 2 - jy;
					j = k + jy0*xdim1;
                                        for(ix=lx; ix<rx; ix++)
                                        {
                                                ix0 = ix; if (ix<0) ix0 = abs(ix); if(ix>=xdim1) ix0 = xdim1*2 - 2 - ix;
                                                ci = multin1[j+ix0];//multin1[kz0][jy0][ix0];multin1[randtmp];
                                                b3x = B3(newx-ix); b3y = B3(newy-jy); b3z = B3(newz-kz); cbx = ci*b3x; byz = b3y*b3z;
                                                fsum = cbx*byz + fsum;//sum will be interpolated intensity by rescaled bspline coefficients
                                                dfxsum = ci*dB3(newx-ix)*byz + dfxsum;
                                                dfysum = cbx*dB3(newy-jy)*b3z + dfysum;
                                                dfzsum = cbx*b3y*dB3(newz-kz) + dfzsum;
                                        }
                                }
                        }
                        kparam = (multin2[randtmp] - fR0)/binR;///////////////////////////??????in2 different size, and out of border
                        lparam = (fsum - fT0)/binT; /////////////////////////////fsum out of border
                        /////////////ignored the part out of bin borders ***************
                        kl = (int)ceil(kparam - 0.5); if(kl<0) kl = 0;//if(kl<0) {kl = 0; printf("kl%f,", kparam);}
                        kr = (int)floor(kparam + 0.5);if(kr > RBINNUM-1) kr = RBINNUM-1;//if(kr > RBINNUM-1) {kr = RBINNUM-1;printf("kr%f,", kparam);}
                        ll = (int)floor(lparam - 1); if(ll<0) ll=0;//if(ll<0) {ll=0;printf("ll%f,",lparam);}
                        lr = (int)ceil(lparam + 2); if(lr > TBINNUM) lr = TBINNUM;//if(lr > TBINNUM) {lr = TBINNUM;printf("lr%f,",lparam);}
                        for (l = ll; l < lr; l++)
                        {
                                for ( k = kl; k <= kr; k++)
                                {
                                        b0k = B0(k-kparam); b3l = B3(l-lparam); db3l = dB3(l-lparam)*b0k;
                                        plk[l][k] = plk[l][k] + b0k*b3l;
                                        pt[l] = pt[l] + b0k*b3l;///////////////////////cut down the redundent multpile*************
                                        //loop on 4(or 3)x4x4 control points which got influence from the sampling voxel
                                        lx = (int)floor(oldx/spacex)-1; if (lx < 0) lx = 0;
                                        rx = (int)ceil(oldx/spacex)+2; if (rx > cnumx) rx = cnumx;
                                        ly = (int)floor(oldy/spacey)-1; if (ly < 0) ly = 0;
                                        ry = (int)ceil(oldy/spacey)+2; if (ry > cnumy) ry = cnumy;
                                        lz = (int)floor(oldz/spacez)-1; if (lz < 0) lz = 0;
                                        rz = (int)ceil(oldz/spacez)+2;  if (rz > cnumz) rz = cnumz;
                                        for(kz = lz; kz<rz; kz++)
                                        {
                                                for(jy = ly; jy<ry; jy++)
                                                {
                                                        for(ix = lx; ix<rx; ix++)
                                                        {
                                                                dplk[l][k][kz][jy][ix*3] = dplk[l][k][kz][jy][ix*3] + db3l*dfxsum*B3(oldx/spacex-ix);
                                                                dplk[l][k][kz][jy][ix*3+1] = dplk[l][k][kz][jy][ix*3+1] + db3l*dfysum*B3(oldy/spacey-jy);
                                                                dplk[l][k][kz][jy][ix*3+2] = dplk[l][k][kz][jy][ix*3+2] + db3l*dfzsum*B3(oldz/spacez-kz);
                                                        }
                                                }
                                        }
                                }
                        } //done with loop on dplk
                }//done with loop on subsampled voxels
                for (l = 0; l < TBINNUM; l++)//////////////////////////deal with plk[l][k] or pt[l] = 0;
                {
                        if (pt[l]!= 0)
                        {
                                for (k = 0; k < RBINNUM; k++)
                                {
                                        plk[l][k] = plk[l][k]/pt[l];
                                }
                        }
                }
                ibts = Bts/powf((float)(itr+51), (float)0.602);
                for(kz = 0; kz<cnumz; kz++)
                {
                        ll = kz*cnumy;
                        for(jy = 0; jy<cnumy; jy++)
                        {
                                j = (ll + jy)*cnumx;
                                for(ix = 0; ix<cnumx; ix++)
                                {
                                        i3 = (j + ix) * 3;
                                        dplkxsum = 0;dplkysum = 0;dplkzsum = 0;
                                        for (l = 0; l < TBINNUM; l++)
                                        {
                                                for ( k = 0; k < RBINNUM; k++)
                                                {
                                                        if(plk[l][k]!= 0)//////////////
                                                        {
                                                                logtmp = log10f(plk[l][k])*LOG2;
                                                                dplkxsum = dplkxsum + dplk[l][k][kz][jy][ix*3]*logtmp;
                                                                dplkysum = dplkysum + dplk[l][k][kz][jy][ix*3+1]*logtmp;
                                                                dplkzsum = dplkzsum + dplk[l][k][kz][jy][ix*3+2]*logtmp;
                                                        }
                                                }
                                        }
                                        tmp3dctrl[i3] = tmp3dctrl[i3] -dplkxsum*ibts;
                                        i3++; tmp3dctrl[i3] = tmp3dctrl[i3]-dplkysum*ibts;
                                        i3++; tmp3dctrl[i3] = tmp3dctrl[i3] -dplkzsum*ibts;
                                }
                        }
                }

        } //done with iterate trials

        free(pt);
        free2dfloat(plk, TBINNUM);
        free5dfloat(dplk,TBINNUM,RBINNUM,cnumz,cnumy);

}

int main(argc, argv)
int argc;
char *argv[];
{
        int itr, steps, nlevel, RBINNUM, TBINNUM, SAMPLENUM;
        float ctrlspace, spacex, spacey, spacez, ftmp;
	int i,j,k,l, nxdim1, nydim1, nzdim1, nxdim2, nydim2, nzdim2, nl, cnumx, cnumy, cnumz, oldcnumx, oldcnumy, oldcnumz;/*number of control points on each direction */
	float **timg, **rimg;
	float *ctrl, *oldctrl;
	float g[20];

	unsigned short T0,Tmax;
        FILE *fpin, *fptar, *fpout, *fpdf;     /* inpput/target/output files */
        unsigned char *inc1, *inc2;
        unsigned short *in1, *in2, *out1;
	short *outdf;
        int error;
        char group[5], element[5];      /* Used in VWriteHeader */
        ViewnixHeader vh1, vh2, vhout, vhdf;
        short bytes1, bytes2, bytesdf; /* the number of bytes of two input images */
        int xdim1, ydim1, zdim1, xdim2, ydim2, zdim2, vsize1, vsize2, hlength1, hlength2; /* numbers of voxels on x,y directions, slices number on z */
        float voxelsize_x1, voxelsize_y1, voxelsize_z1, voxelsize_x2, voxelsize_y2, voxelsize_z2;
	short dfmin[3], dfmax[3], dftmp;
	char *indf;

        SLICES  sl1, sl2;       /* Structure containing information about the slices of the scene */
        short numvol; /* volume index */

		//srand(time(0));
		#ifdef __WXWINCE__  
		srand((unsigned)CeGetRandomSeed());
		#else  
		srand((unsigned)time(NULL));  
		#endif  

		LOG2= 1/log10f(2);

        if(argc<8)
        {
                printf("Usage:\n");
                printf("nonrigid:\n");
                printf("input    : name of input file;\n");
                printf("target   : name of target file;\n");
                printf("output   : name of output file;\n");
		printf("converge tolerance: number of iterations\n");
		printf("pyramid level: image pyramid level number\n");
		printf("step size: optimization step size\n");
                printf("ctrlspace: B-spline control point spacing on x-voxel, unit: number of voxels;\n");
                printf("df (optional): deformation field output using IM0 format;\n");
                exit(1);
        }
        fpin = fopen(argv[1], "rb");
        if (fpin == NULL)
        {
                printf("Error in opening source file \n");
                exit(-1);
        }
        fptar = fopen(argv[2], "rb");
        if (fptar == NULL)
        {
                printf("Error in opening target file \n");
                exit(-1);
        }
        fpout = fopen(argv[3], "wb");
        if (fpout == NULL)
        {
                printf("Error in opening the output file\n");
                exit(-1);
        }

	sscanf(argv[4], "%d", &i);
	itr = i *10;
	sscanf(argv[5], "%d", &i);
	nlevel = i;
	sscanf(argv[6], "%d", &i);
	steps = i*i;
	sscanf(argv[7], "%d", &i);
	ctrlspace = i * (float)1.0;

        if(argc == 9) {
		fpdf = fopen(argv[8], "wb");
		if (fpdf == NULL) {
		        printf("Error in opening the output file\n");
		        exit(-1);
		}
		get_file_info(argv[1], &vhdf, &i);
		vhdf.scn.density_measurement_unit_valid = 0;
		free(vhdf.scn.smallest_density_value);
		vhdf.scn.smallest_density_value = (float *)malloc(3*sizeof(float));
		free(vhdf.scn.largest_density_value);
		vhdf.scn.largest_density_value = (float *)malloc(3*sizeof(float));
		free(vhdf.scn.signed_bits);
		vhdf.scn.signed_bits = (short *)malloc(3*sizeof(short));
		free(vhdf.scn.bit_fields);
		vhdf.scn.bit_fields = (short *)malloc(6*sizeof(short));
	}


	RBINNUM = 32; TBINNUM = 32; SAMPLENUM = 2048;
	if(RBINNUM < 0) RBINNUM = DEFAULT_RBINNUM;// 32
	if(TBINNUM < 0) TBINNUM = DEFAULT_TBINNUM; // 32
	if (SAMPLENUM < 0) SAMPLENUM =  DEFAULT_SUBSAMPLENUM;// 2048

	if (ctrlspace < 1) ctrlspace = DEFAULT_CONTROLSPACE; // 15
	//itr = 1000; steps = 100; nlevel = 7; 
	if(itr < 10) itr = DEFAULT_ITERATION;//  1000
	if(steps < 0) steps = DEFAULT_STEPSIZE; // 100
	if(nlevel < 0) nlevel = DEFAULT_LEVELNUM; // 7

	g[0]  = (float) 0.596797; g[1]  = (float) 0.313287; g[2]  = (float)-0.0827691; g[3]  =(float) -0.0921993; g[4]  = (float) 0.0540288;
	g[5]  =  (float)0.0436996; g[6]  = (float)-0.0302508; g[7]  = (float)-0.0225552; g[8]  = (float) 0.0162251; g[9]  = (float) 0.0118738;
	g[10] = (float)-0.00861788; g[11] = (float)-0.00627964; g[12] = (float) 0.00456713; g[13] = (float) 0.00332464; g[14] =(float) -0.00241916;
	g[15] = (float)-0.00176059; g[16] = (float) 0.00128128; g[17] = (float) 0.000932349; g[18] = (float)-0.000678643; g[19] =(float) -0.000493682;

        /*-----------------------*/
        /* Read 3DViewnix header */
        /*-----------------------*/
        get_file_info(argv[1], &vh1, &hlength1);
        get_file_info(argv[2], &vh2, &hlength2);
        get_file_info(argv[1], &vhout, &i);////////////////      //vhout.scn.num_of_bits = 16;

        if (vh1.gen.data_type != IMAGE0 || vh2.gen.data_type != IMAGE0)
        {
                printf("Cannot handle other than IMAGE0 files\n");
                exit(-1);
        }
        if ((vh1.scn.num_of_bits != 8 && vh1.scn.num_of_bits != 16) || (vh2.scn.num_of_bits != 8 && vh2.scn.num_of_bits != 16))
        {
                printf("Only handle 8 bits or 16 bits data.  Cannot handle %d bits data with %d bits data\n", vh1.scn.num_of_bits, vh2.scn.num_of_bits);
                exit(-1);
        }

        /* Compute information about the slices of the scene (number, locations, etc...) */
        compute_slices(&vh1, &sl1);
        compute_slices(&vh2, &sl2);

        bytes1 = vh1.scn.num_of_bits / 8;
        bytes2 = vh2.scn.num_of_bits / 8;

        xdim1 = vh1.scn.xysize[0];
        ydim1 = vh1.scn.xysize[1];
        voxelsize_x1 = vh1.scn.xypixsz[0];
        voxelsize_y1 = vh1.scn.xypixsz[1];
        voxelsize_z1 = vh1.scn.loc_of_subscenes[1] - vh1.scn.loc_of_subscenes[0]; //printf("vz1 = %f, ",voxelsize_z1);

        xdim2 = vh2.scn.xysize[0];
        ydim2 = vh2.scn.xysize[1];
        voxelsize_x2 = vh2.scn.xypixsz[0];
        voxelsize_y2 = vh2.scn.xypixsz[1];
        voxelsize_z2 = vh2.scn.loc_of_subscenes[1] - vh2.scn.loc_of_subscenes[0];

        /************************/
        /* Traverse ALL VOLUMES */
for(numvol=0; numvol<sl1.volumes; numvol++)
{
        zdim1 = sl1.slices[numvol];     zdim2 = sl2.slices[numvol];

        vsize1 = xdim1 * ydim1 * zdim1; vsize2 = xdim2 * ydim2 * zdim2;
	spacex = ctrlspace; ctrlspace = ctrlspace * voxelsize_x1;   spacey = ctrlspace / voxelsize_y1;        spacez = ctrlspace / voxelsize_z1;

        fseek(fpin, (numvol*vsize1/8)+hlength1, 0);
        inc1 = (unsigned char *)malloc(vsize1 * bytes1);
        VReadData((char *)inc1, bytes1, vsize1, fpin, &i);
        switch (bytes1)        {
                case 1: in1 = (unsigned short *)malloc(vsize1 * 2);
                        for (i = 0; i < vsize1; i++) in1[i] = inc1[i];
                        free(inc1);
                        break;
                case 2: in1 = (unsigned short *)inc1;
                        break;
        }

        fseek(fptar, (numvol*vsize2/8)+hlength2, 0);
        inc2 = (unsigned char *)malloc(vsize2 * bytes2);
        VReadData((char *)inc2, bytes2, vsize2, fptar, &j);
        switch (bytes2)        {
                case 1: in2 = (unsigned short *)malloc(vsize2 * 2);
                        for (i = 0; i < vsize2; i++) in2[i] = inc2[i];
                        free(inc2);
                        break;
                case 2: in2 = (unsigned short *)inc2;
                        break;
        }
        /* check whether too many levels */
        i = xdim1;        if(i > ydim1) i = ydim1;        if(i > zdim1) i = zdim1;
        if(i > xdim2) i = xdim2;        if(i > ydim2) i = ydim2;        if(i > zdim2) i = zdim2;
        nxdim1 = xdim1; nydim1 = ydim1; nzdim1 = zdim1; nxdim2 = xdim2; nydim2 = ydim2; nzdim2 = zdim2;
        for (j = 0; i>6; i=i/2, j++, nxdim1=nxdim1/2, nydim1 = nydim1/2, nzdim1 = nzdim1/2, nxdim2=nxdim2/2, nydim2 = nydim2/2, nzdim2 = nzdim2/2) {
        }
        if(j==0) k = 1; else k = j; //Level nlevel-1 to 0
	if(nlevel> k) nlevel = k;

        timg = (float **)malloc(nlevel*sizeof(float*));
	/* pyramid, compute if there's more than 1 layer */
        for (nl = 1; nl <nlevel; nl++) //real level # is nl+1
        {
resetTime();
                l = (int)pow(2,nl-1);
				i = xdim1/l; j = ydim1/l; k = zdim1/l; 
		l = (int)pow(2,nl); nxdim1 = xdim1/l; nydim1 = ydim1/l; nzdim1 = zdim1/l;
                timg[nl] = (float*)malloc(nxdim1*nydim1*nzdim1*sizeof(float));
               	if (nl == 1) pyramid3dif(in1, timg[1], g, 20, xdim1, ydim1, zdim1);
		else	pyramid3dff(timg[nl-1], timg[nl], g, 20, i, j, k);
printf("\n----Pyramid time(test): %f seconds (%d Level:%d,%d,%d),\n",getElapsedTime(), nl,nxdim1,nydim1,nzdim1);
        }
        timg[0] = (float*)malloc(vsize1*sizeof(float));
	for (k = 0; k < vsize1; k++)	timg[0][k] = (float)in1[k];
	free(in1);

        rimg = (float **)malloc(nlevel*sizeof(float*));
        for (nl = 1; nl <nlevel; nl++) //real level # is nl+1
        {
resetTime();
                l = (int)pow(2,nl-1); i = xdim2/l; j = ydim2/l; k = zdim2/l;
		l = (int)pow(2,nl); nxdim2 = xdim2/l; nydim2 = ydim2/l; nzdim2 = zdim2/l;
                rimg[nl] = (float*)malloc(nxdim2*nydim2*nzdim2*sizeof(float));
               	if (nl == 1) pyramid3dif(in2, rimg[1], g, 20, xdim2, ydim2, zdim2);
		else	pyramid3dff(rimg[nl-1], rimg[nl], g, 20, i, j, k);
printf("\n----Pyramid time(ref): %f seconds (%d Level:%d,%d,%d),\n",getElapsedTime(), nl,nxdim2,nydim2,nzdim2);
        }


	for(nl=nlevel-1; nl>=0; nl--)
	{ 
                i = (int)pow(2,nl); nxdim1 = xdim1/i; nydim1 = ydim1/i; nzdim1 = zdim1/i; nxdim2 = xdim2/i; nydim2 = ydim2/i; nzdim2 = zdim2/i;
resetTime(); //test image ci
		calci3(timg[nl],nxdim1, nydim1, nzdim1); //sequential ci
printf("\n----Ci time(test): %f seconds (%d Level:%d,%d,%d),\n",getElapsedTime(), nl,nxdim1,nydim1,nzdim1);

		// control mesh 
		ftmp = nxdim1-(float)1.0;
		cnumx = (int)ceil(ftmp/spacex+1);
		ftmp = nydim1-(float)1.0;
		cnumy = (int)ceil(ftmp/spacey+1);
		ftmp = nzdim1-(float)1.0; 
		cnumz = (int)ceil(ftmp/spacez+1);
		ctrl = (float*)malloc(cnumx*cnumy*cnumz*3*sizeof(float));
		if(nl != nlevel-1) {
		        multicontrol1d(oldctrl, ctrl, oldcnumx, oldcnumy, oldcnumz, cnumx, cnumy, cnumz);
		        free(oldctrl);
		}
		else {  j = cnumx*cnumy*cnumz*3; for(i=0; i<j; i++) ctrl[i]=0;}

resetTime();
                if(nxdim1*nydim1*nzdim1 <= SAMPLENUM+50){
                        if (nl == 0)  dMIoptimize1dif(ctrl, cnumx, cnumy, cnumz, timg[0], in2, spacex, spacey, spacez, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2,itr, steps, RBINNUM, TBINNUM, SAMPLENUM);
			else	dMIoptimizeall1d(ctrl, cnumx, cnumy, cnumz, timg[nl], rimg[nl], spacex, spacey, spacez, nxdim1, nydim1, nzdim1, nxdim2, nydim2, nzdim2,itr, steps/(nl*30+1), RBINNUM, TBINNUM);
printf("\n----All opt time: %f/%d sec (%d Level:%d,%d,%d with control mesh %d,%d,%d),\n",getElapsedTime(),itr, nl,nxdim2,nydim2,nzdim2,cnumx, cnumy, cnumz);
                }
                else {
				if (nl == 0)  dMIoptimize1dif(ctrl, cnumx, cnumy, cnumz, timg[0], in2, spacex, spacey, spacez, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2,itr, steps, RBINNUM, TBINNUM, SAMPLENUM);
                                else	dMIoptimize1d(ctrl, cnumx, cnumy, cnumz, timg[nl], rimg[nl], spacex, spacey, spacez, nxdim1, nydim1, nzdim1, nxdim2, nydim2, nzdim2,itr, steps/(nl*30+1), RBINNUM, TBINNUM, SAMPLENUM);
printf("\n----Subsample opt: %f/%d sec (%d Level:%d,%d,%d with control mesh %d,%d,%d),\n",getElapsedTime(),itr,nl,nxdim2,nydim2,nzdim2,cnumx, cnumy, cnumz);
                }

		if(nl >0) {free(rimg[nl]); free(timg[nl]);}
                oldctrl = ctrl; oldcnumx = cnumx; oldcnumy = cnumy; oldcnumz = cnumz;
	}  //ci and opti

        free(in2);
resetTime();
	out1 = (unsigned short *)malloc(vsize1 * 2);
	if(argc == 9) {
		//save df
		outdf = (short*)malloc(vsize1*6); //deformation feild output
		bsplineimagedf(ctrl, timg[0], out1, spacex, spacey, spacez, xdim1,ydim1,zdim1, cnumx, cnumy, cnumz, outdf, voxelsize_x1, voxelsize_y1, voxelsize_z1);
	}
	else	bsplineimage(ctrl, timg[0], out1, spacex, spacey, spacez, xdim1,ydim1,zdim1, cnumx, cnumy, cnumz);
printf("\n----Bspline time: %f (%d,%d,%d)\n",getElapsedTime(),xdim1,ydim1,zdim1);

	free(ctrl); 
	free(timg[0]);
	free(timg); free(rimg);

       /*  set output file header */
	T0 = 65535; Tmax = 0;
	for(i=0; i<vsize1; i++) {
		if(out1[i]> Tmax) Tmax = out1[i];
		if(out1[i]< T0) T0 = out1[i];
	}
	vhout.scn.smallest_density_value[0]=(float)T0;
	vhout.scn.largest_density_value[0] =(float)Tmax;
	vhout.scn.signed_bits[0] = 0;
	vhout.scn.smallest_density_value_valid = vhout.scn.largest_density_value_valid = 1;
        error = VWriteHeader(fpout, &vhout, group, element);
        if (error <= 104) {
                printf("Fatal error in writing output header\n");
                exit(-1);
        }

	switch (bytes1)
	  {
	    case 1:
	      inc1 = (unsigned char *)malloc(vsize1 * bytes1);
	      for (i = 0; i < vsize1; i++)
		inc1[i] = (unsigned char)out1[i];
	      free(out1);
	      break;

	    case 2:
	      inc1 = (unsigned char *)out1;
	      break;
	  }
	/* Save output volume */
        VWriteData( (char *)inc1, bytes1, vsize1, fpout, &i);////// 8bits or 16 bits
        if(i != vsize1)
        {
                printf("ERROR: Couldn't write volume #%d.\n", numvol+1);
                exit(3);
        }
        free(inc1);

	if(argc == 9) {
	       /*  set output df file header */
		   int n;
		for (n=0; n<3; n++) dfmin[n] = xdim1, dfmax[n] = -xdim1;
		j = vsize1*3;
		for(i=0; i<j; i+=3)
			for (n=0; n<3; n++) {
				dftmp = outdf[i+n];
				if(dftmp > dfmax[n]) dfmax[n] = dftmp;
				if(dftmp < dfmin[n]) dfmin[n] = dftmp;
			}
		vhdf.scn.num_of_integers = 3;
		vhdf.scn.signed_bits[0] = vhdf.scn.signed_bits[1] = vhdf.scn.signed_bits[2] = 1;
		vhdf.scn.signed_bits_valid = 1;
		for (n=0; n<3; n++)
		{
			vhdf.scn.smallest_density_value[n] = (float)dfmin[n];
			vhdf.scn.largest_density_value[n] = (float)dfmax[n];
		}
		vhdf.scn.smallest_density_value_valid = vhdf.scn.largest_density_value_valid = 1;
	vhdf.scn.num_of_density_values = 3;
	if ((dfmin[0]>=-128) && (dfmax[0]<=127) &&
	    (dfmin[1]>=-128) && (dfmax[1]<=127) &&
	    (dfmin[2]>=-128) && (dfmax[2]<=127)) {
		bytesdf = 1;
		vhdf.scn.bit_fields[0] = 0;
		vhdf.scn.bit_fields[1] = 7;
		vhdf.scn.bit_fields[2] = 8;
		vhdf.scn.bit_fields[3] = 15;
		vhdf.scn.bit_fields[4] = 16;
		vhdf.scn.bit_fields[5] = 23;
	}
	else {
		bytesdf = 2;
		vhdf.scn.bit_fields[0] = 0;
		vhdf.scn.bit_fields[1] = 15;
		vhdf.scn.bit_fields[2] = 16;
		vhdf.scn.bit_fields[3] = 31;
		vhdf.scn.bit_fields[4] = 32;
		vhdf.scn.bit_fields[5] = 47;
	}
	vhdf.scn.num_of_bits = bytesdf * 24;

		error = VWriteHeader(fpdf, &vhdf, group, element);
		if (error <= 104) {
		        printf("Fatal error in writing output header\n");
		        exit(-1);
		}

		switch (bytesdf)
		  {
		    case 1:
		      indf = (char *)malloc(vsize1 * 3 * bytesdf);
		      for (i = 0; i < vsize1*3; i++)
			indf[i] = (signed char)outdf[i];
		      free(outdf);
		      break;

		    case 2:
		      indf = (char *)outdf;
		      break;
		  }
		/* Save output volume */
		VWriteData( indf, bytesdf, vsize1*3, fpdf, &i);////// 8bits or 16 bits
		if(i != vsize1*3)
		{
		        printf("ERROR: Couldn't write volume #%d.\n", numvol+1);
		        exit(3);
		}
		free(indf);

	}

}//all vols

        fclose(fpin);
        fclose(fptar);
        VCloseData(fpout);
	if(argc == 9) VCloseData(fpdf);

	exit(0);
}

