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

//=======================================================
/**
 * \file   deform.c
 * \brief  command line program to calculate the output of 
 *               deformed image using deformation field parameter files
 * \author Xiaofen Zheng, Jun 2009
 * \notes: use nonrigid.c for the free-form deformation field parameter files 
 */
//======================================================= 
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
/* B-spline related functions: B3
 */
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
        z2tmp = (float)(1 - powf(z1, (float)ll));
	for(k=0; k<zdim; k++) {
		kk = k*llength;
		for(j=0; j<ydim; j++) {
			kj = kk + j*xdim;
		        tmpsum = bcoeff[kj];
		        for(l=1; l<xdim; l++)
		                tmpsum = (float)(tmpsum +bcoeff[kj+l]*powf(z1, (float)l));
		        for(l=0; l<xdim-1; l++)
		                tmpsum = (float)(tmpsum +bcoeff[kj+l]*powf(z1, (float)(2*xdim-2-l)));
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
        z2tmp = (float)(1 - powf(z1, (float)ll));
	for(k=0; k<zdim; k++) {
		kk = k*llength;
		for(i=0; i<xdim; i++)
		{
		        tmpsum = bcoeff[kk+i];
		        for(l=1; l<ydim; l++)
		                tmpsum = (float)(tmpsum + bcoeff[kk+l*xdim+i]* powf(z1, (float)l));
		        for(l=0; l<ydim-1; l++)
		                tmpsum = (float)(tmpsum + bcoeff[kk+l*xdim+i]* powf(z1, (float)(2*ydim-2-l)));
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
        z2tmp = (float)(1 - powf(z1, (float)ll));
	for(j=0; j<ydim; j++) {
		jj = j*xdim;
		for(i=0; i<xdim; i++)
		{
			ji = jj + i;
		        tmpsum = bcoeff[ji];
		        for(l=1; l<zdim; l++)
		                tmpsum = (float)(tmpsum + bcoeff[l*llength+ji]*powf(z1,(float)l));
		        for(l=0; l<zdim-1; l++)
		                tmpsum = (float)(tmpsum + bcoeff[l*llength+ji]*powf(z1,(float)(2*zdim-2-l)));
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

/*****************************************************************************
 * FUNCTION: dfimage
 * DESCRIPTION: Applies deformation field and Bspline mirror interpolation to a 3d image
 * PARAMETERS:
 *    pfin1: the interpolated input data
 *    indf: deformation field (presented by char or short, which was multipled by 4)
 *    xdim, ydim, zdim: the dimensions of the original test image
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    out: store the deformed 3-D volume
 * EXIT CONDITIONS:
 *****************************************************************************/
void dfshortimage(in, indf, out, xdim,ydim,zdim, vdf)
	float *in;
	short *indf;
	unsigned short *out;
	int xdim, ydim, zdim;
	float vdf;
{
	int oldx, oldy, oldz, ipos, dfpos, k0, j0, i0, lz, rz, ly, ry, lx, rx, kk, kj, i, j, k, hlength;
	float newx, newy, newz, ci, fsum;

	ipos = -1; hlength = xdim * ydim;
	for(oldz = 0; oldz<zdim; oldz++) for(oldy= 0; oldy<ydim; oldy++) for (oldx=0; oldx<xdim; oldx++) {
		ipos++; dfpos = ipos * 3;
		newx = (float)(indf[dfpos] * 0.25) + oldx;
		newy = (float)(indf[dfpos+1] * 0.25) + oldy;
		newz = (float)(indf[dfpos+2] * vdf) + oldz;
		lz = (int)floor(newz)-1;  rz = (int)ceil(newz)+2;
                ly = (int)floor(newy)-1;  ry = (int)ceil(newy)+2;
                lx = (int)floor(newx)-1;  rx = (int)ceil(newx)+2;
                fsum = 0;
                        for(k=lz; k<rz; k++)
                        {
                                k0 = k; if (k<0) k0 = abs(k); if(k>=zdim) k0 = zdim*2 - 2 - k;
				kk = k0*hlength;
                                for(j=ly; j<ry; j++)
                                {
                                        j0 = j; if (j<0) j0 = abs(j); if(j>=ydim) j0 = ydim*2 - 2 - j;
					kj = kk+j0*xdim;
                                        for(i=lx; i<rx; i++)
                                        {
                                                i0 = i; if (i<0) i0 = abs(i); if(i>=xdim) i0 = xdim*2 - 2 - i;
                                                ci = in[kj+i0];
                                                fsum = ci*B3(newx-i)*B3(newy-j)*B3(newz-k)+ fsum;
                                        }
                                }
                        }
                        if(fsum<0) out[ipos]=0; else out[ipos] = (unsigned short)(fsum+0.5);
	}
}

void dfcharimage(in, indf, out, xdim,ydim,zdim, vdf)
	float *in;
	char *indf;
	unsigned short *out;
	int xdim, ydim, zdim;
	float vdf;
{
	int oldx, oldy, oldz, ipos, dfpos, k0, j0, i0, lz, rz, ly, ry, lx, rx, kk, kj, i, j, k, hlength;
	float newx, newy, newz, ci, fsum;

	ipos = -1; hlength = xdim * ydim;
	for(oldz = 0; oldz<zdim; oldz++) for(oldy= 0; oldy<ydim; oldy++) for (oldx=0; oldx<xdim; oldx++) {
		ipos++; dfpos = ipos*3;
		newx = (float)(indf[dfpos] * 0.25) + oldx;
		newy = (float)(indf[dfpos+1] * 0.25) + oldy;
		newz = (float)(indf[dfpos+2] * vdf) + oldz;
		lz = (int)floor(newz)-1;  rz = (int)ceil(newz)+2;
                ly = (int)floor(newy)-1;  ry = (int)ceil(newy)+2;
                lx = (int)floor(newx)-1;  rx = (int)ceil(newx)+2;
                fsum = 0;
                        for(k=lz; k<rz; k++)
                        {
                                k0 = k; if (k<0) k0 = abs(k); if(k>=zdim) k0 = zdim*2 - 2 - k;
				kk = k0*hlength;
                                for(j=ly; j<ry; j++)
                                {
                                        j0 = j; if (j<0) j0 = abs(j); if(j>=ydim) j0 = ydim*2 - 2 - j;
					kj = kk+j0*xdim;
                                        for(i=lx; i<rx; i++)
                                        {
                                                i0 = i; if (i<0) i0 = abs(i); if(i>=xdim) i0 = xdim*2 - 2 - i;
                                                ci = in[kj+i0];
                                                fsum = ci*B3(newx-i)*B3(newy-j)*B3(newz-k)+ fsum;
                                        }
                                }
                        }
                        if(fsum<0) out[ipos]=0; 
			else out[ipos] = (unsigned short)(fsum+0.5);
	}
}


int main(argc, argv)
int argc;
char *argv[];
{
	int i,k;/*number of control points on each direction */
	float *pfin1;

	unsigned short T0,Tmax;
        FILE *fpin, *fpout, *fpdf;     /* inpput/output/deformation field files */
        unsigned char *inc1;
        unsigned short *in1, *out1;
	char *incdf;
	short *indf;
        int error;
        char group[5], element[5];      /* Used in VWriteHeader */
        ViewnixHeader vh1, vhout, vhdf;
        int bytes1, bytesdf; /* the number of bytes of two input images */
        int xdim1, ydim1, zdim1, vsize1, hlength1, xdimdf, ydimdf, zdimdf, vsizedf, hlengthdf; /* numbers of voxels on x,y directions, slices number on z */
        float voxelsize_x1, voxelsize_y1, voxelsize_z1, voxelsize_xdf, voxelsize_ydf, voxelsize_zdf, vdf;

        SLICES  sl1, sldf;       /* Structure containing information about the slices of the scene */
        short numvol; /* volume index */

        if(argc<4)
        {
                printf("Usage:\n");
                printf("deform:\n");
                printf("input    : name of input file;\n");
                printf("par      : nonrigid deformation field file;\n");
                printf("output   : name of output file;\n");
                exit(1);
        }
        fpin = fopen(argv[1], "rb");
        if (fpin == NULL)
        {
                printf("Error in opening source file \n");
                exit(-1);
        }
        fpdf = fopen(argv[2], "rb");
        if (fpdf == NULL)
        {
                printf("Error in opening source file \n");
                exit(-1);
        }

        fpout = fopen(argv[3], "wb");
        if (fpout == NULL)
        {
                printf("Error in opening the output file\n");
                exit(-1);
        }

        /*-----------------------*/
        /* Read 3DViewnix header */
        /*-----------------------*/
        get_file_info(argv[1], &vh1, &hlength1);
        if (vh1.gen.data_type != IMAGE0)
        {
                printf("Cannot handle other than IMAGE0 files\n");
                exit(-1);
        }
        if (vh1.scn.num_of_bits != 8 && vh1.scn.num_of_bits != 16)
        {
                printf("Only handle 8 bits or 16 bits data.  Cannot handle %d bits data\n", vh1.scn.num_of_bits);
                exit(-1);
        }
        get_file_info(argv[1], &vhout, &i);
        get_file_info(argv[2], &vhdf, &hlengthdf);
        if (vhdf.gen.data_type != IMAGE0)
        {
                printf("Cannot handle other than IMAGE0 files\n");
                exit(-1);
        }
        if (vhdf.scn.num_of_bits != 24)
        {
                printf("Only handle 24 bits data.  Cannot handle %d bits data\n", vhdf.scn.num_of_bits);
                exit(-1);
        }
	if (vhdf.scn.num_of_density_values != 3) {
                printf("Only handle deformation field in 3 dimensions\n");
	}

        /* Compute information about the slices of the scene (number, locations, etc...) */
        compute_slices(&vh1, &sl1);
        bytes1 = vh1.scn.num_of_bits / 8;
        xdim1 = vh1.scn.xysize[0];
        ydim1 = vh1.scn.xysize[1];
        voxelsize_x1 = vh1.scn.xypixsz[0];
        voxelsize_y1 = vh1.scn.xypixsz[1];
        voxelsize_z1 = vh1.scn.loc_of_subscenes[1] - vh1.scn.loc_of_subscenes[0]; 

        /* Compute information about the slices of the scene (number, locations, etc...) */
        compute_slices(&vhdf, &sldf);
        bytesdf = vhdf.scn.num_of_bits / 8;
        xdimdf = vhdf.scn.xysize[0];
        ydimdf = vhdf.scn.xysize[1];
        voxelsize_xdf = vhdf.scn.xypixsz[0];
        voxelsize_ydf = vhdf.scn.xypixsz[1];
        voxelsize_zdf = vhdf.scn.loc_of_subscenes[1] - vhdf.scn.loc_of_subscenes[0]; 

        /************************/
        /* Traverse ALL VOLUMES */
for(numvol=0; numvol<sl1.volumes; numvol++)
{
        zdim1 = sl1.slices[numvol]; zdimdf = sldf.slices[numvol];
        vsize1 = xdim1 * ydim1 * zdim1; vsizedf = xdimdf * ydimdf * zdimdf* 3;
	if ((xdim1 != xdimdf) || (ydim1 != ydimdf) || (zdim1 != zdimdf)) {
		printf("Different image and deformation field size\n");
                exit(-1);
	}
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
	pfin1 = (float*)malloc(vsize1*sizeof(float));
	for (k = 0; k < vsize1; k++)	pfin1[k] = (float)in1[k];
	free(in1);
	calci3(pfin1,xdim1, ydim1, zdim1);
		
        fseek(fpdf, (numvol*vsizedf/8)+hlengthdf, 0);
        incdf = (char *)malloc(vsizedf * bytesdf);
        VReadData(incdf, bytes1, vsizedf, fpdf, &i);

	vdf = (float)0.25*voxelsize_x1/voxelsize_z1;
	out1 = (unsigned short *)malloc(vsize1 * 2);
        switch (bytes1)        {
                case 1: dfcharimage(pfin1, incdf, out1, xdim1,ydim1,zdim1, vdf);
                        break;
                case 2: indf = (short *)incdf;
			dfshortimage(pfin1, indf, out1, xdim1,ydim1,zdim1, vdf);
                        break;
        }
        free(incdf);
	free(pfin1);

       /*  set output file header, the output will be 16 bits */
	T0 = 65535; Tmax = 0;
	for(i=0; i<vsize1; i++) {
		if(out1[i]> Tmax) Tmax = out1[i];
		if(out1[i]< T0) T0 = out1[i];
	}
	vhout.scn.smallest_density_value[numvol]=(float)T0;
	vhout.scn.largest_density_value[numvol] =(float)Tmax;
	vhout.scn.signed_bits[numvol] = 0;
	vhout.scn.smallest_density_value_valid = vhout.scn.largest_density_value_valid = 1;
        error = VWriteHeader(fpout, &vhout, group, element);
        if (error <= 104) {
                printf("Fatal error in writing output header\n");
                exit(-1);
        }
	/* Save output volume */
	switch (bytes1)  {
	case 1:
		inc1 = (unsigned char *)malloc(vsize1 * bytes1);
		for (i = 0; i < vsize1; i++) inc1[i] = (unsigned char)out1[i];
		free(out1);
		break;
	case 2:
		inc1 = (unsigned char *)out1;
		break;
	}
        VWriteData((char *)inc1, bytes1, vsize1, fpout, &i);////// 8bits or 16 bits
	free(inc1);


}//all vols

        fclose(fpin); fclose(fpdf);
        VCloseData(fpout);

	exit(0);
}

