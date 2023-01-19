/*
  Copyright 1993-2014, 2017, 2021 Medical Image Processing Group
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

#include <math.h>
#include <assert.h>
#include <string.h>
#include <cv3dv.h>
#include <stdlib.h>
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif

typedef struct
{
	short x,y,z;
} voxelS;

typedef struct 
{
	short x,y,z;
	double inhomo_value;
} voxelElem;

typedef struct 
{
	int a,b,c;
} gscaleR;

typedef struct
{
	int num_voxels;
	voxelElem *pStart;
} regionElem;

void bin_to_grey(unsigned char *bin_buffer, int length, unsigned char *grey_buffer,
   int min_value,int max_value),
 compute_gscale();
int compute_scale_image(char *o_file, char *d_file, char *s_file, double a,
   int b),
 GenerateLabels(char *dt, int w, int h, int s),
 get_factor_nD(voxelElem *pElem, long int mask_volume, double factor[]);

#define SQR(a) ((a)*(a))

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

static double maxarg1,maxarg2;
#define FMAX(a,b) (maxarg1=(a),maxarg2=(b), maxarg1 > maxarg2 ?\
maxarg1 : maxarg2)

double *vector(int n)
{
	double *v;

	v = (double *)malloc(n*sizeof(*v));
	if (v == NULL)
	{
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}
	return v;
}

/******************************for inhomogeneity and find nn****************************************************/
#define SCALE_THRESHOLD 0.01
#define VOLUME_THRESHOLD 0.001  
#define Q_SIZE 65536
int CONNECTIVITY = 6;
unsigned int *tbz,*tby; /* Multiplication tables for slices and rows */
int push=0;  
int tot_dt=0;
int pre_tot_dt=0;
int temp_scale=0;
int height,width,slices,space,num_of_bits,slice_size,volume_size,size_bin;
unsigned char *dt_8;
/*************************************************************************************/

voxelS *Q;      
regionElem *pSort,*pRegion;
voxelElem *pGlobal;
voxelElem *pMask;
gscaleR *gRegion;

/**************************** for function optimization **********************************/
#define GOLD 1.618034
#define GLIMIT 100.0
#define TINY 1.0e-5
#define ITMAX 400
#define CGOLD 0.3819660
#define ZEPS 1.0e-10
#define TOL 2.0e-4
int ncom;
double *pcom,*xicom;
double (*nrfunc)(double a[]);
#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);
/*****************************************************************************************/
#define FEATURES 1
#define STOP_CONDITION 0.001
double pxsize,pysize;
int num_sets;   // total object number
double **aV;
double factor[66];
double factor_1[3];

double function_value_nD(double a[]);
double function_value_1D(double x);
int powell(double p[],double **xi,int n, double ftol,int *iter, double *fret, double (*func)(double a[]));
double brent(double ax,double bx,double cx,double (*f)(double),double tol,double *xmin);
int mnbrak(double *ax, double *bx,double *cx, double *fa, double *fb,double *fc,double (*func)(double));
int FindObject(char *dt, int w, int h,int s,short startx,short starty,short startz);
int inhomo_correct(char *o_file, char *rdat_file, char *b_file, char *m_file, char *c_file, char *i_file, int W);

int number_largest_object,num_pre=0;

/**************************************for compute scale*******************************/
#define FILTER  1
#define SCALE 12
#define HIST_THRESHOLD 0.85
#define tolerance 13.0

int  mean_density_value;
int  background = 0;
float *scale_map;
unsigned long *scale_image;
unsigned long *que;

int pcol,prow,pslice;
int *sphere_no_points;
short (**sphere_points)[3];
double mask[2*FILTER+1][2*FILTER+1], mask_total;

unsigned short *data_scale16; 
float *datascale16;

/*****************************************************************************/
char *datafile[FEATURES], *outfile[FEATURES], *meanfile[FEATURES], *inhomofile[FEATURES], *endname1[FEATURES], *endname2[FEATURES], *templaten1[FEATURES], *templaten2[FEATURES];
char temp[25];
static int Way=0,times=10;
int  NUM_REGIONS_PAINT=250;
int METHOD=0,ii_temp=0;
float NN1=1.0,NN2=1.0;
float sigma_std = 0, SIGMA_TIMES=(float)1.1;
float SIGMA_THRESHOLD=0.75;
float bknd_factor = 1.0;
int flag_method = 1; // flag_method = 1 if sigma_std is not specified; 0 otherwise.
/**************************************************************************************/
void usage()
{
	printf("\n");
	printf("Usage: inhomo_gscale <ori_file> <corrected_file> <inhomo_file> <-mode 1/0> [-zeta] [-zetafactor] [-thetab] [-num_regions] [-bknd_factor] [-times]\n");
	printf("-mode      : multiplicative 1 or additive 0\n");
	printf("-zeta      : stD deviation\n");
	printf("-zetafactor     : zeta factor\n");
	printf("-thetab       : inclusion factor\n");
	printf("-num_regions       : Number of regions\n");
	printf("-bknd_factor  : Background Weight\n");
	printf("-times  : iteration number\n");
	exit(1);
}

/**********************************************************************************/
void parse_commandline(argc, argv)
int argc;
char *argv[];
{
	int args_parsed;

	if (argc < 5)
		usage();

	args_parsed = 1;
	if (strcmp(argv[args_parsed], "-help") == 0)
		usage();
	datafile[0]=argv[args_parsed];
	args_parsed++;
	outfile[0]=argv[args_parsed];
	args_parsed++;
	templaten1[0]=argv[args_parsed];

	if (strcmp(argv[args_parsed], "-mode") == 0)
	{
		args_parsed++;
		sscanf(argv[args_parsed], "%d", &Way);
		args_parsed++;
		templaten1[0]="temp__";
	}
	else
	{
		args_parsed++;
		if (strcmp(argv[args_parsed], "-mode") == 0)
		{
			args_parsed++;
			sscanf(argv[args_parsed], "%d", &Way);
			args_parsed++;
		}
		else
			usage();
	}


	if (strcmp(argv[args_parsed], "-zeta") == 0)
	{
		flag_method = 0;
		args_parsed++;
		sscanf(argv[args_parsed], "%f", &sigma_std);
		args_parsed++;
	}
	if (strcmp(argv[args_parsed], "-zetafactor") == 0)
	{
		args_parsed++;
		sscanf(argv[args_parsed], "%f", &SIGMA_TIMES);
		args_parsed++;
	}
	if (strcmp(argv[args_parsed], "-thetab") == 0)
	{
		args_parsed++;
		sscanf(argv[args_parsed], "%f", &SIGMA_THRESHOLD);
		args_parsed++;
	}
	if (!strcmp(argv[args_parsed],"-num_regions"))
	{
		args_parsed++;
		sscanf(argv[args_parsed], "%d", &NUM_REGIONS_PAINT);
		args_parsed++;
	}
	if (!strcmp(argv[args_parsed],"-bknd_factor"))
	{
		args_parsed++;
		sscanf(argv[args_parsed], "%f", &bknd_factor);
		args_parsed++;
	}
	if (!strcmp(argv[args_parsed],"-times"))
	{
		args_parsed++;
		sscanf(argv[args_parsed], "%d", &times);
		args_parsed++;
	}

}

int fileCopy(char* cSour, char* cDest)
{
	FILE *fp_in, *fp_out;
	long fileLength;
	char *pContent = NULL;

	fp_in = fopen(cSour, "rb");
	if( fp_in == NULL ) 	  return -1;
	fseek(fp_in, 0, SEEK_END);
	fileLength = ftell(fp_in);
	pContent = (char *)malloc(fileLength*sizeof(char));
	if( pContent == NULL ) return -1;

	fseek(fp_in, 0, SEEK_SET);
	fread(pContent, 1, fileLength, fp_in);

	fp_out = fopen(cDest, "wb");
	if( fp_out == NULL ) 	  return -1;
	fwrite(pContent, 1, fileLength, fp_out);

	fclose(fp_out);
	fclose(fp_in); 

	return 0;
}
/*****************************************************************************/
/*  Modified: 3/9/04 data read & write corrected by Dewey Odhner. */
/*  Modified: 3/11/04 8-bit data accommodated by Dewey Odhner. */
int main(argc,argv) 
int argc;
char *argv[];
{
	int temp,ii, num_of_bytes, orig_num_of_bytes;

	int j, error_code, sslice,srow,scol;
	unsigned long min,max;
	static ViewnixHeader vh_in,vh_out;
	char group[6],element[6];
	FILE *fp_in, *fp_out;
	unsigned short *temp_sdata16;
	unsigned char  *temp_cdata8;
	float *data16, *temp_Fdata16;

	parse_commandline(argc, argv); 

	/* sprintf(command,"cp %s temp_ori.IM0",datafile[0]);
	temp=system(command);
	if (temp!=0)
	{
	printf("No such file!\n");
	exit(-1);
	}*/

	//////////  file copy: datafile[0] --> "temp_ori.IM0" /// xinjian 2008.3.13 ///
	error_code = fileCopy(datafile[0], "temp_ori.IM0");
	if( error_code < 0 ) exit(error_code);

	// ***************************************************************** //
	// Reading in the original datafile into an array.                   //
	// ******************************************************************//

	fp_in = fopen(datafile[0], "rb");
	error_code = VReadHeader(fp_in, &vh_in, group, element);
	switch (error_code)
	{
	case 0:
	case 106:
	case 107:
		break;
	default:
		fprintf(stderr, "file = %s; group = %s; element = %s\n", datafile[0], group, element);
	}
	fclose(fp_in);

	pcol = vh_in.scn.xysize[0];
	prow = vh_in.scn.xysize[1];
	pslice = vh_in.scn.num_of_subscenes[0];
	slice_size = pcol * prow;
	volume_size = slice_size * pslice;
	num_of_bits = vh_in.scn.num_of_bits;
	num_of_bytes = orig_num_of_bytes = num_of_bits/8;

	data16 = (float *)malloc(volume_size*sizeof(float));	
	temp_sdata16 = (unsigned short *)malloc(volume_size*sizeof(short));
	if (num_of_bytes < 2)
		temp_cdata8 = (unsigned char *)malloc(volume_size);

	temp_Fdata16 = (float *)malloc(volume_size*sizeof(float));

	if (temp_sdata16==NULL || (num_of_bytes<2 && temp_cdata8==NULL) ||
		temp_Fdata16==NULL)
	{
		printf("Memory allocation error \n");
		exit(-1);
	}

	fp_in = fopen(datafile[0],"rb");

	error_code = VSeekData(fp_in, 0);
	if ((error_code&&error_code<106) ||
		VReadData(num_of_bytes==2? (char *)temp_sdata16:(char *)temp_cdata8,
		num_of_bits/8, volume_size, fp_in, &j))
	{
		fprintf(stderr, "Error reading data.\n");
		exit(-1);
	}
	fclose(fp_in);

	// ***************************************************************************
	// writing out the data_scale16 image to an array.
	// ***************************************************************************

	for (sslice = 0; sslice < pslice; sslice++)
		for (srow = 0; srow < prow; srow++)
			for (scol = 0; scol < pcol; scol++)
			{
				data16[sslice * slice_size + srow * pcol + scol] = (float)(
					num_of_bytes==2?
					temp_sdata16[sslice * slice_size + srow * pcol + scol]:
					temp_cdata8[sslice * slice_size + srow * pcol + scol]);
			}

			fp_out = fopen("temp_ori","wb+");
			if(fp_out == NULL)
			{printf("Open file %s error\n",outfile[0]);
			exit(-1);
			}

			if (fwrite(data16, sizeof(float), volume_size, fp_out) != volume_size)
			{
				fprintf(stderr, "Write file %s error\n", "temp_ori");
				exit(-1);
			}
			fclose(fp_out);
			free(data16);

			// ****************************************************************************
			// ****************************************************************************

			for(ii=0;ii<times;ii++)
			{      
				ii_temp=ii;

				printf("Computing G-Scale .......\n\n");
				temp=compute_scale_image("temp_ori.IM0","temp_ori","temp_scale",1.0,0);
				printf("G-scale finished.......\n\n");

				printf("Inhomo correction.......\n");
				temp=inhomo_correct("temp_ori.IM0","temp_ori","temp_bin.BIM","max_img.IM0","temp_cor","temp_inhomo.IM0",Way);
				if (temp==0)
				{
					printf("Difference is small. Inhomo correction finished.\n");
					break;
				}
				if (temp==2)
				{
					printf("No object! Inhomo_correction failed\n");
					break;
				}
				num_pre=number_largest_object;

				/*****************************************************************************/
				// **************** writing the integer format of temp_cor into temp_ori.IM0 **/

				fp_in = fopen("temp_cor","rb");

				if (fread(temp_Fdata16, sizeof(float),volume_size, fp_in) != volume_size)
				{
					fprintf(stderr, "Error reading data.\n");
					exit(-1);
				}
				fclose(fp_in);

				min = 65536; max = 0;
				for (sslice = 0; sslice < pslice; sslice++)
					for (srow = 0; srow < prow; srow++)
						for (scol = 0; scol < pcol; scol++)
						{
							temp_sdata16[sslice * slice_size + srow * pcol + scol] = (int) temp_Fdata16[sslice * slice_size + srow * pcol + scol];
							if(temp_sdata16[sslice * slice_size + srow * pcol + scol] < min ) min = temp_sdata16[sslice * slice_size + srow * pcol + scol];
							if(temp_sdata16[sslice * slice_size + srow * pcol + scol] > max ) max = temp_sdata16[sslice * slice_size + srow * pcol + scol];
						}

						memcpy(&vh_out, &vh_in, sizeof(vh_in));

						vh_out.scn.num_of_bits = 16;
						vh_out.scn.bit_fields[1] = vh_out.scn.num_of_bits - 1;
						vh_out.scn.smallest_density_value[0] = (float)min;
						vh_out.scn.largest_density_value[0] = (float)max;

						fp_out = fopen("temp_cor.IM0","wb+");
						if(fp_out == NULL)
						{
							printf("Open file %s error\n",outfile[0]);
							exit(-1);
						}

						num_of_bytes = vh_out.scn.num_of_bits/8;

						error_code = VWriteHeader(fp_out, &vh_out, group, element);
						if ((error_code&&error_code<106) ||
							VWriteData(num_of_bytes==2? (char *)temp_sdata16:(char *)temp_cdata8,
							num_of_bytes,volume_size,fp_out,&j))
						{
							fprintf(stderr, "Error writing data.\n");
							exit(-1);
						}
						fclose(fp_out);

					//	system("cp temp_cor.IM0 temp_ori.IM0");
						error_code = fileCopy("temp_cor.IM0", "temp_ori.IM0");
						if( error_code < 0 ) exit(error_code);


			}    

			/*****************************************************************************/
			// **************** writing the integer format of temp_cor into temp_ori.IM0 **/

			fp_in = fopen("temp_cor","rb");

			if (fread(temp_Fdata16, sizeof(float), volume_size, fp_in) != volume_size)
			{
				fprintf(stderr, "Error reading data.\n");
				exit(-1);
			}
			fclose(fp_in);

			min = 65535; max = 0;
			for (sslice = 0; sslice < pslice; sslice++)
				for (srow = 0; srow < prow; srow++)
					for (scol = 0; scol < pcol; scol++)
						if (num_of_bytes == 2)
						{
							temp_sdata16[sslice * slice_size + srow * pcol + scol] = (int) temp_Fdata16[sslice * slice_size + srow * pcol + scol];
							if(temp_sdata16[sslice * slice_size + srow * pcol + scol] < min ) min = temp_sdata16[sslice * slice_size + srow * pcol + scol];
							if(temp_sdata16[sslice * slice_size + srow * pcol + scol] > max ) max = temp_sdata16[sslice * slice_size + srow * pcol + scol];
						}
						else
						{
							temp_cdata8[sslice * slice_size + srow * pcol + scol] = (int) temp_Fdata16[sslice * slice_size + srow * pcol + scol];
							if(temp_cdata8[sslice * slice_size + srow * pcol + scol] < min ) min = temp_cdata8[sslice * slice_size + srow * pcol + scol];
							if(temp_cdata8[sslice * slice_size + srow * pcol + scol] > max ) max = temp_cdata8[sslice * slice_size + srow * pcol + scol];
						}

						memcpy(&vh_out, &vh_in, sizeof(vh_in));

						vh_out.scn.smallest_density_value[0] = (float)min;
						vh_out.scn.largest_density_value[0] = (float)max;

						fp_out = fopen("temp_cor.IM0","wb+");
						if(fp_out == NULL)
						{
							printf("Open file %s error\n",outfile[0]);
							exit(-1);
						}

						num_of_bytes = vh_out.scn.num_of_bits/8;

						error_code = VWriteHeader(fp_out, &vh_out, group, element);
						if ((error_code&&error_code<106) ||
							VWriteData(num_of_bytes==2? (char *)temp_sdata16:(char *)temp_cdata8,
							num_of_bytes,volume_size,fp_out,&j))
						{
							fprintf(stderr, "Error writing data.\n");
							exit(-1);
						}
						fclose(fp_out);
						free (temp_sdata16);
						if (orig_num_of_bytes < 2)
							free (temp_cdata8);
						free (temp_Fdata16);

						/*****************************************************************************/

						/*sprintf(command,"mv temp_cor.IM0 %s", outfile[0]);
						temp= system(command);
						if (temp!=0)
						{
							printf("Error occur!\n");
							exit(-1);
						}*/
						error_code = fileCopy("temp_cor.IM0", outfile[0]);
						if( error_code < 0 ) exit(error_code);


						unlink("temp_scale");

						endname1[0] = "inhomo.IM0";
						endname2[0] = "gscale.IM0";

						inhomofile[0] = (char *)malloc(100);
						meanfile[0]   = (char *)malloc(100);

						strcpy(inhomofile[0],templaten1[0]);
						strcpy(meanfile[0], templaten1[0]);

						strcat(inhomofile[0], endname1[0]);
						strcat(meanfile[0], endname2[0]);

						// ------------------------------------------------------------
						// writing the inhomo and mean images. ------------------------

						/*sprintf(command,"mv temp_inhomo.IM0 %s", inhomofile[0]);
						temp= system(command);
						if (temp!=0)
						{
							printf("Error occur!\n");
							exit(-1);
						}*/
						error_code = fileCopy("temp_inhomo.IM0", inhomofile[0]);
						if( error_code < 0 ) exit(error_code);


						/*sprintf(command,"mv max_img.IM0 %s", meanfile[0]);
						temp= system(command);
						if (temp!=0)
						{
							printf("Error occur!\n");
							exit(-1);
						}*/
						error_code = fileCopy("max_img.IM0", meanfile[0]);
						if( error_code < 0 ) exit(error_code);


						// -----------------------------------------------------------

						unlink("temp_ori.IM0");
						unlink("temp_bin.BIM");
						unlink("orig_largestgscale.IM0");
						unlink("temp_ori");
						unlink("temp_cor");
						unlink("temp__gscale.IM0");
						unlink("temp__inhomo.IM0");

						printf("program exit normally \n");
						return 0;
}

/********************************************************************************************
* FUNCTION: compute_scale_image
* DESCRIPTION: compute the scale of the input image
* PARAMETERS: original image, scale image, constant, background
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
* 
* 
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 07/07/00 by Ying Zhuge
*    Modified : 3/8/04 read check added by Dewey Odhner.
*
*******************************************************************************************/
int compute_scale_image(char *o_file, char *d_file, char *s_file, double a,
    int b)
{
	int i,j,k,l, error_code, count, **histogram, hist_sum[FEATURES], feature_thr[FEATURES];
	unsigned long tti1,tti2;
	unsigned long x,y,z,xx,yy,zz,largest_density_value, ***ppptti1;
	double tt1,tt2;
	static ViewnixHeader vh_in;
	char group[6],element[6],*datafile[FEATURES],*outfile,*dat_file;
	double anisotropy_col,anisotropy_row,anisotropy_slice,homogeneity_sigma,inv_scale_sigma,sigma_constant = 1;
	double sigma_mean,sigma_sum;
	FILE *fp_in, *fp_out;

	for(i = 0;i<FEATURES;i++)
		datafile[i] = o_file;

	dat_file = d_file;
	outfile = s_file;
	sigma_constant=a;
	background=b;  

	fp_in = fopen(datafile[0], "rb");
	error_code = VReadHeader(fp_in, &vh_in, group, element);
	switch (error_code)
	{
	case 0:
	case 106:
	case 107:
		break;
	default:
		fprintf(stderr, "file = %s; group = %s; element = %s\n", datafile[0], group, element);
		exit(-1);
	}
	fclose(fp_in);

	pcol = vh_in.scn.xysize[0];
	prow = vh_in.scn.xysize[1];
	pslice = vh_in.scn.num_of_subscenes[0];
	slice_size = pcol * prow;
	volume_size = slice_size * pslice;
	num_of_bits = vh_in.scn.num_of_bits;

	//*****************************************************************************
	tti1 = 2 * (SCALE + 5);
	ppptti1 = (unsigned long ***) malloc(tti1 * sizeof(unsigned long **));

	ppptti1[0] = (unsigned long **) malloc(tti1 * tti1 * sizeof(unsigned long *));

	for (i = 0; i < (int)tti1; i++)
		ppptti1[i] = ppptti1[0] + i * tti1;
	ppptti1[0][0] = (unsigned long *) malloc(tti1 * tti1 * tti1 * sizeof(unsigned long));

	for (i = 0; i < (int)tti1; i++)
		for (j = 0; j < (int)tti1; j++)
			ppptti1[i][j] = ppptti1[0][0] + (i * tti1 + j) * tti1;

	for (i = 0; i < (int)tti1; i++)
		for (j = 0; j < (int)tti1; j++)
			for (k = 0; k < (int)tti1; k++)
				ppptti1[i][j][k] = 0;

	sphere_no_points = (int *) malloc((SCALE + 1) * sizeof(int));

	sphere_points = (void *) malloc((SCALE + 1) * sizeof(void *));

	anisotropy_col = vh_in.scn.xypixsz[0];
	anisotropy_row = vh_in.scn.xypixsz[1];
	if (pslice > 1)
		anisotropy_slice = vh_in.scn.loc_of_subscenes[1] - vh_in.scn.loc_of_subscenes[0];
	if (anisotropy_slice < 0.0)
		anisotropy_slice = -anisotropy_slice;
	tt1 = anisotropy_col;
	if (tt1 > anisotropy_row)
		tt1 = anisotropy_row;
	if (pslice > 1 && tt1 > anisotropy_slice)
		tt1 = anisotropy_slice;
	anisotropy_col = anisotropy_col / tt1;
	anisotropy_row = anisotropy_row / tt1;
	if (pslice > 1)
		anisotropy_slice = anisotropy_slice / tt1;

	tti1 = SCALE + 5;
	if (pslice > 1)
		printf("Anisotropy: slice = %f, row = %f, column = %f\n",
		anisotropy_slice, anisotropy_row, anisotropy_col);
	else
		printf("Anisotropy: row = %f, column = %f\n", anisotropy_row, anisotropy_col);

	if (pslice > 1)
	{
		for (k = 0; k <= SCALE; k++)
		{
			sphere_no_points[k] = 0;
			for (i = 0; i < 1; i++) 
				for (j = -k - 2; j <= k + 2; j++)
					for (l = -k - 2; l <= k + 2; l++)
						if (ppptti1[tti1 + i][tti1 + j][tti1 + l] == 0)
						{
							tt1 = sqrt(pow(((double) j) * anisotropy_row, 2.0) + pow(((double) l) * anisotropy_col, 2.0));
							if (tt1 <= ((double) k) + 0.5)
							{
								sphere_no_points[k] = sphere_no_points[k] + 1;
								ppptti1[tti1 + i][tti1 + j][tti1 + l] = 2;
							}
						}

						sphere_points[k] = (void *) malloc(3 * sphere_no_points[k] * sizeof(int));

						if (sphere_points[k] == NULL)
						{
							printf("Couldn't allocate memory (execution terminated)\n");
							exit(-1);
						}

						tti2 = 0;
						for (i = 0; i < 1; i++)
							for (j = -k - 2; j <= k + 2; j++)
								for (l = -k - 2; l <= k + 2; l++)
									if (ppptti1[tti1 + i][tti1 + j][tti1 + l] == 2)
									{
										ppptti1[tti1 + i][tti1 + j][tti1 + l] = 1;
										sphere_points[k][tti2][0] = 0;
										sphere_points[k][tti2][1] = j;
										sphere_points[k][tti2][2] = l;
										tti2 = tti2 + 1;
									}
		}
	}
	else
	{
		for (k = 0; k <= SCALE; k++)
		{
			sphere_no_points[k] = 0;
			for (j = -k - 2; j <= k + 2; j++)
				for (l = -k - 2; l <= k + 2; l++)
					if (ppptti1[tti1][tti1 + j][tti1 + l] == 0)
					{
						tt1 = sqrt(pow(((double) j) * anisotropy_row, 2.0)
							+ pow(((double) l) * anisotropy_col, 2.0));
						if (tt1 <= ((double) k) + 0.5)
						{
							sphere_no_points[k] = sphere_no_points[k] + 1;
							ppptti1[tti1][tti1 + j][tti1 + l] = 2;
						}
					}

					sphere_points[k] = (void *) malloc(3 * sphere_no_points[k] * sizeof(int));
					if (sphere_points[k] == NULL)
					{
						printf("Couldn't allocate memory (execution terminated)\n");
						exit(-1);
					}
					tti2 = 0;
					for (j = -k - 2; j <= k + 2; j++)
						for (l = -k - 2; l <= k + 2; l++)
							if (ppptti1[tti1][tti1 + j][tti1 + l] == 2)
							{
								ppptti1[tti1][tti1 + j][tti1 + l] = 1;
								sphere_points[k][tti2][0] = 0;
								sphere_points[k][tti2][1] = j;
								sphere_points[k][tti2][2] = l;
								tti2 = tti2 + 1;
							}
		}
	}
	printf("\n");
	fflush(stdout);
	free(ppptti1[0][0]);
	free(ppptti1[0]);
	free(ppptti1);


	mask_total = 0.0;
	for (yy = -FILTER; yy <= FILTER; yy++)
		for (xx = -FILTER; xx <= FILTER; xx++)
			mask[yy + FILTER][xx + FILTER] = 0;

	for (yy = -FILTER; yy <= FILTER; yy++)
		for (xx = -FILTER; xx <= FILTER; xx++)
		{
			tt2 = pow(anisotropy_col * xx, 2.0);
			tt2 = tt2 + pow(anisotropy_row * yy, 2.0);
			tt2 = 1 / (1 + tt2);
			mask[yy + FILTER][xx + FILTER] = tt2;
			mask_total = mask_total + tt2;
		}


		//**************************************************************************************

		if(FEATURES == 1)
		{
			datascale16 = (float *)malloc(volume_size*sizeof(float));	

			if(datascale16 == NULL)
			{
				printf("Memory allocation error \n");
				exit(-1);
			}
			fp_in = fopen(dat_file,"rb");
			if(fp_in == NULL)
			{
				printf("Cannot open file %s \n",dat_file);
				exit(-1);
			}
			if (fread(datascale16, sizeof(float), volume_size, fp_in) != volume_size)
			{
				fprintf(stderr, "Cannot read file %s \n",dat_file);
				exit(-1);
			}

			fclose(fp_in);

			tt1 = 0;
			largest_density_value = 0;
			for(z = 0;(int)z<pslice;z++)
				for(y = 0;(int)y<prow;y++)
					for(x = 0;(int)x<pcol;x++)
					{
						tt1 = tt1 + datascale16[z*slice_size + y*pcol +x];
						if(datascale16[z*slice_size + y*pcol +x] > largest_density_value)
							largest_density_value = (unsigned long)datascale16[z*slice_size + y*pcol +x];
					}
					mean_density_value = (int)(tt1/volume_size);
					printf("mean_density_value: %d\n",mean_density_value);

					/**********************************************************************************/
					/*----to compute the histogram for each feature and the threshold for true edge---*/

					histogram = (int **)malloc(FEATURES*sizeof(int *));
					histogram[0] = (int *)malloc(FEATURES*(largest_density_value+1)*sizeof(int));
					if(histogram == NULL || histogram[0] == NULL)
						printf("Memory allocation error \n"), exit(1);

					for(j=0;j<=(int)largest_density_value;j++)
						histogram[0][j] = 0;

					for (i=0;i<pslice;i++)
						for (j=0;j<prow;j++)
							for (k=0;k<pcol-1;k++)
							{
								xx = k+1;
								yy = j;
								zz = i;

								tti1 = abs((int)datascale16[i*slice_size+j*pcol+k] - (int)datascale16[zz*slice_size+yy*pcol+xx]);
								histogram[0][tti1]++;
							}

							for (i=0;i<pslice;i++)
								for (j=0;j<prow-1;j++)
									for (k=0;k<pcol;k++)
									{
										xx = k;
										yy = j+1;
										zz = i;

										tti1 = abs((int)datascale16[i*slice_size+j*pcol+k] - (int)datascale16[zz*slice_size+yy*pcol+xx]);
										histogram[0][tti1]++;

									}

									for (i=0;i<pslice-1;i++)
										for(j=0;j<prow;j++)
											for (k=0;k<pcol;k++)
											{
												xx = k;
												yy = j;
												zz = i+1;

												tti1 = abs((int)datascale16[i*slice_size+j*pcol+k] - (int)datascale16[zz*slice_size+yy*pcol+xx]);
												histogram[0][tti1]++;

											}

											hist_sum[0] = 0;
											for(j=0;j<(int)largest_density_value;j++)
												hist_sum[0] = hist_sum[0] + histogram[0][j];

											for(j=0;j<(int)largest_density_value;j++)
											{
												tti1 = 0;
												feature_thr[0] = (int)j;
												for(k=0;k<=j;k++)
													tti1 = tti1+histogram[0][k];
												if (((double)tti1 /(double) hist_sum[0])>=HIST_THRESHOLD)
													break;
											}

											printf("Histogram threshold computation is done \n");
											printf("Features Threshold %d : %f \n", i,(double)feature_thr[0]); 

											//--------------------compute the homogeneity sigma-------------------------------
											sigma_sum = 0;
											count = 0;
											sigma_mean = 0;
											{
												for(z = 0;(int)z<pslice;z++)
													for(y = 0;(int)y<prow;y++)
														for(x = 0;(int)x<pcol-1;x++)
														{
															zz = z;
															yy = y;
															xx = x + 1;
															tti1 =  abs((int)datascale16[z*slice_size + y*pcol +x] - (int)datascale16[zz*slice_size + yy*pcol +xx]);
															if((int)tti1 <= feature_thr[0])
															{
																sigma_sum = sigma_sum + tti1;
																count ++;
															}
														}

														for(z = 0;(int)z<pslice;z++)
															for(y = 0;(int)y<prow-1;y++)
																for(x = 0;(int)x<pcol;x++)
																{
																	zz = z;
																	yy = y+1;
																	xx = x;
																	tti1 =  abs((int)datascale16[z*slice_size + y*pcol +x] - (int)datascale16[zz*slice_size + yy*pcol +xx]);
																	if((int)tti1 <= feature_thr[0])
																	{
																		sigma_sum = sigma_sum + tti1;
																		count ++;
																	}
																}
																for(z = 0;(int)z<pslice-1;z++)
																	for(y = 0;(int)y<prow;y++)
																		for(x = 0;(int)x<pcol;x++)
																		{
																			zz = z+1;
																			yy = y;
																			xx = x;
																			tti1 =  abs((int)datascale16[z*slice_size + y*pcol +x] - (int)datascale16[zz*slice_size + yy*pcol +xx]);
																			if((int)tti1 <= feature_thr[0])
																			{
																				sigma_sum = sigma_sum + tti1;
																				count ++;
																			}
																		}

																		sigma_mean = sigma_sum / count;
											}

											//--------------------compute the homogeneity sigma-------------------------------
											sigma_sum = 0;
											count = 0;
											{
												for(z = 0;(int)z<pslice;z++)
													for(y = 0;(int)y<prow;y++)
														for(x = 0;(int)x<pcol-1;x++)
														{
															zz = z;
															yy = y;
															xx = x + 1;
															tti1 =  abs((int)datascale16[z*slice_size + y*pcol +x] - (int)datascale16[zz*slice_size + yy*pcol +xx]);
															if((int)tti1 <= feature_thr[0])
															{
																sigma_sum = sigma_sum + pow((tti1-sigma_mean),2.);
																count ++;
															}
														}

														for(z = 0;(int)z<pslice;z++)
															for(y = 0;(int)y<prow-1;y++)
																for(x = 0;(int)x<pcol;x++)
																{
																	zz = z;
																	yy = y+1;
																	xx = x;
																	tti1 =  abs((int)datascale16[z*slice_size + y*pcol +x] - (int)datascale16[zz*slice_size + yy*pcol +xx]);
																	if((int)tti1 <= feature_thr[0])
																	{
																		sigma_sum = sigma_sum + pow((tti1 - sigma_mean),2.);
																		count ++;
																	}
																}
																for(z = 0;(int)z<pslice-1;z++)
																	for(y = 0;(int)y<prow;y++)
																		for(x = 0;(int)x<pcol;x++)
																		{
																			zz = z+1;
																			yy = y;
																			xx = x;
																			tti1 =  abs((int)datascale16[z*slice_size + y*pcol +x] - (int)datascale16[zz*slice_size + yy*pcol +xx]);
																			if((int)tti1 <= feature_thr[0])
																			{
																				sigma_sum = sigma_sum + pow((tti1 - sigma_mean),2.);
																				count ++;
																			}
																		}
											}

											/* -------------------------------------------------------------------
											Flag_method toggles between paint method and gradient method of 
											of computing sigma.
											------------------------------------------------------------------*/   
											if(flag_method  == 1) 
												sigma_std = (float)sqrt((double)sigma_sum/(double)count);

											homogeneity_sigma = SIGMA_TIMES*sigma_std;
											printf("homogeneity_sigma value: %f \n",homogeneity_sigma); 

											scale_map = (float *) malloc( (largest_density_value + 1) * sizeof(double));

											//------------------ GAUSSIAN ----------------------------------------------------
											inv_scale_sigma = -0.5 / pow(homogeneity_sigma, 2.0);
											for (i = 0; i <= (int)largest_density_value; i++)
												scale_map[i] = (float)exp(inv_scale_sigma * pow((double) i, 2.0));
		}

		compute_gscale();

		fp_out = fopen(outfile,"wb+");
		if(fp_out == NULL)
		{
			printf("Open file %s error\n",outfile);
			exit(-1);
		}

		if (fwrite(scale_image, sizeof(float), volume_size, fp_out) != volume_size)
		{
			fprintf(stderr, "Write failure.\n");
			exit(-1);
		}
		fclose(fp_out);

		if(FEATURES == 1)
		{
			free(datascale16);
		}
		free(histogram[0]);
		free(histogram);
		free(scale_image);

		free(sphere_no_points);
		for(i=0;i<=SCALE;i++)
			free(sphere_points[i]);
		free(sphere_points);

		printf("exit normally \n");
		return 0;

}
/*****************************************************************************
* FUNCTION: compute_gscale
* DESCRIPTION: Computes the g-scale values for the entire volume and stores  
*        them in a scale-image array.
* PARAMETERS: None
* SIDE EFFECTS: 
* FUNCTIONS CALEED: None
* ENTRY CONDITIONS: 1) scale_map array is alloted and 
*           proper values are assigned
* RETURN VALUE: None
* EXIT CONDITIONS: Compute scale values
* HISTORY:
*  Created: 03/02/00 by Anant Madabhushi and Andre Souza
*  
*****************************************************************************/
void compute_gscale()
{
	FILE *fp_in1, *fp_out;
	static ViewnixHeader vh_in,vh_out;
	int j, xxx, yyy, zzz, slice, row, col;
	int x1, x2, y1, y2, z1, z2, error_code, num_largest_regions;
	int tti2;
	unsigned long label=0;
	unsigned long nn = 0;
	int tmp_xxx = 0;
	double tt1;
	unsigned long pointer_pos = 0;
	unsigned char *dt_8_in, *dt_8_out;
	unsigned short *mean_scene, *max_scene;
	char group[6],element[6],command[100];
	int num_of_bytes;
	float maxint=0.0;
	int iii,jjj,kkk;
	unsigned long  *LARGEST_REGION = NULL; //[NUM_REGIONS_PAINT];
	unsigned long  *SIZE_REGION = NULL; //[NUM_REGIONS_PAINT];
	unsigned long  *MEAN_INT_REGION = NULL; //[NUM_REGIONS_PAINT];
	unsigned long  *MAX_INT_REGION = NULL; //[NUM_REGIONS_PAINT];
	unsigned long *histogram_gscale_regions, num_vox_inhomo=0, num_vox_region=0, min, max, intensity, SCALE_MAX, ttti1, aaa;
	int largest_value;
	float tti1,ttseed,tti5,pppi1,mean_region_intensity=0.0;

	LARGEST_REGION = (unsigned long *)malloc(NUM_REGIONS_PAINT * sizeof(unsigned long));
	SIZE_REGION = (unsigned long *)malloc(NUM_REGIONS_PAINT * sizeof(unsigned long));
	MEAN_INT_REGION = (unsigned long *)malloc(NUM_REGIONS_PAINT * sizeof(unsigned long));
	MAX_INT_REGION = (unsigned long *)malloc(NUM_REGIONS_PAINT * sizeof(unsigned long));

	if( LARGEST_REGION == NULL || SIZE_REGION == NULL || MEAN_INT_REGION == NULL || MAX_INT_REGION == NULL )
		return;

	scale_image = (unsigned long *) malloc(volume_size * sizeof(unsigned long));
	que = (unsigned long *) malloc(volume_size * sizeof(unsigned long));
	mean_scene = (unsigned short *) malloc(volume_size * sizeof(unsigned short));
	max_scene = (unsigned short *) malloc(volume_size * sizeof(unsigned short));

	// Initializing the g-scale (scale_image)

	for (slice = 0; slice < pslice; slice++)
		for (row = 0; row < prow; row++)
			for (col = 0; col < pcol; col++)
			{
				scale_image[slice * slice_size + row * pcol + col] = 0;
			}

			// -------------------------------------------------------------------------
			if (FEATURES == 1)
			{

				for (slice = 0; slice < pslice; slice++)
					for (row = 0; row < prow; row++)
						for (col = 0; col < pcol; col++)
				  {
					  tti1 = datascale16[slice * slice_size + row * pcol + col];
					  if((background==0)&&(tti1<(bknd_factor*mean_density_value)))
						  scale_image[slice * slice_size + row * pcol + col] = volume_size;
					  if(scale_image[slice * slice_size + row * pcol + col]==0)
					  {
						  label++;
						  nn = 1;
						  que[pointer_pos] = slice * slice_size + row * pcol + col;
						  scale_image[slice * slice_size + row * pcol + col] = label;

						  while(pointer_pos < nn)
						  {
							  zzz = que[pointer_pos]/(slice_size);
							  yyy = (que[pointer_pos] - zzz*slice_size)/(pcol);
							  tmp_xxx = que[pointer_pos] - zzz*slice_size;
							  xxx = (int) tmp_xxx%(pcol);

							  pointer_pos++;

							  ttseed = datascale16[zzz*slice_size + yyy*pcol + xxx];

							  x1 = xxx - 1;
							  x2 = xxx + 1;
							  y1 = yyy - 1;
							  y2 = yyy + 1;
							  z1 = zzz - 1;
							  z2 = zzz + 1;

							  if (x1<0)
								  x1=0;

							  if (x2 == pcol)
								  x2=pcol-1;

							  if (y1<0)
								  y1 = 0;

							  if (y2 == prow)
								  y2=prow-1;

							  if (z1<0)
								  z1 = 0;

							  if (z2 == pslice)
								  z2 = pslice - 1;


							  if (scale_image[zzz * slice_size + yyy * pcol + x1]==0)
							  {
								  tti5 = datascale16[zzz * slice_size + yyy * pcol + x1];
								  tti2 = (int) (tti5 - ttseed);
								  if (tti2 < 0)
									  tti2 = -tti2;
								  tt1 = scale_map[tti2];
								  if (tt1 > SIGMA_THRESHOLD)
								  {		
									  que[nn] = zzz * slice_size + yyy * pcol + x1;
									  scale_image[zzz * slice_size + yyy * pcol + x1] = label;
									  nn++;
								  }
							  }

							  if (scale_image[zzz * slice_size + yyy * pcol + x2]==0)
							  {
								  tti5 = datascale16[zzz * slice_size + yyy * pcol + x2];
								  tti2 = (int) (tti5 - ttseed);
								  if (tti2 < 0)
									  tti2 = -tti2;
								  tt1 = scale_map[tti2];
								  if (tt1 > SIGMA_THRESHOLD)
								  {
									  que[nn] = zzz * slice_size + yyy * pcol + x2;
									  scale_image[zzz * slice_size + yyy * pcol + x2] = label;
									  nn++;
								  }
							  }

							  if (scale_image[zzz * slice_size + y1 * pcol + xxx]==0)
							  {
								  tti5 = datascale16[zzz * slice_size + y1 * pcol + xxx];
								  tti5 = (float) (tti5 - ttseed);
								  if (tti2 < 0)
									  tti2 = -tti2;
								  tt1 = scale_map[tti2];
								  if (tt1 > SIGMA_THRESHOLD)
								  {
									  que[nn] = zzz * slice_size + y1 * pcol + xxx;
									  scale_image[zzz * slice_size + y1 * pcol + xxx] = label;
									  nn++;
								  }
							  }

							  if (scale_image[zzz * slice_size + y2 * pcol + xxx]==0)
							  {
								  tti5 = datascale16[zzz * slice_size + y2 * pcol + xxx];
								  tti2 = (int) (tti5 - ttseed);
								  if (tti2 < 0)
									  tti2 = -tti2;
								  tt1 = scale_map[tti2];
								  if (tt1 > SIGMA_THRESHOLD)
								  {
									  que[nn] = zzz * slice_size + y2 * pcol + xxx;
									  scale_image[zzz * slice_size + y2 * pcol + xxx] = label;
									  nn++;
								  }
							  }

							  if (scale_image[z1 * slice_size + yyy * pcol + xxx]==0)
							  {
								  tti5 = datascale16[z1 * slice_size + yyy * pcol + xxx];
								  tti2 = (int) (tti5 - ttseed);
								  if (tti2 < 0)
									  tti2 = -tti2;
								  tt1 = scale_map[tti2];
								  if (tt1 > SIGMA_THRESHOLD)
								  {
									  que[nn] = z1 * slice_size + yyy * pcol + xxx;
									  scale_image[z1 * slice_size + yyy * pcol + xxx] = label;
									  nn++;
								  }
							  }

							  if (scale_image[z2 * slice_size + yyy * pcol + xxx]==0)
							  {
								  tti5 = datascale16[z2 * slice_size + yyy * pcol + xxx];
								  tti2 = (int) (tti5 - ttseed);
								  if (tti2 < 0)
									  tti2 = -tti2;
								  tt1 = scale_map[tti2];
								  if (tt1 > SIGMA_THRESHOLD)
								  {
									  que[nn] = z2 * slice_size + yyy * pcol + xxx;
									  scale_image[z2 * slice_size + yyy * pcol + xxx] = label;
									  nn++;
								  }
							  }

						  } // for the while loop


					  } //for the else
					  pointer_pos = 0;

					  /*if(que)
						  free(que);
					  que = (unsigned long *)malloc(volume_size * sizeof(unsigned long));*/
					  memset(que, 0, volume_size * sizeof(unsigned long));  // modified by xinjian 2008.3.13
						} // for the innermost for.
			} //main if

			for(aaa=0;(int)aaa<volume_size;aaa++)
			{
				if(scale_image[aaa] == volume_size)
					scale_image[aaa] = 0;
			}

			fp_in1 = fopen("temp_ori.IM0", "rb");
			error_code = VReadHeader(fp_in1, &vh_in, group, element);
			switch (error_code)
			{
			case 0:
			case 106:
			case 107:
				break;
			default:
				fprintf(stderr, "file = %s; group = %s; element = %s\n", "temp_ori.IM0", group, element);
				exit(-1);
			}
			fclose(fp_in1);

			height = vh_in.scn.xysize[1];
			width = vh_in.scn.xysize[0];
			slices = vh_in.scn.num_of_subscenes[0];
			slice_size = height * width;
			volume_size = slice_size * slices;
			num_of_bits = vh_in.scn.num_of_bits;
			largest_value = (int)vh_in.scn.largest_density_value[0];

			printf("Number of Regions %d \n", (int)label);

			// Number of g-scale regions.
			SCALE_MAX = (unsigned long) label;

			// Computing the largest g-scale region.
			histogram_gscale_regions = (unsigned long *) malloc((SCALE_MAX + 1) * sizeof(unsigned long));

			// Intialising the histogram vector.
			for(intensity = 0; intensity<(SCALE_MAX+1); intensity++)
				histogram_gscale_regions[intensity] = 0;

			// computing the histogram containing the g-scale regions.
			for(iii = 0; iii<slices; iii++)
				for(jjj = 0; jjj<height; jjj++)
					for(kkk = 0; kkk<width; kkk++)
					{ttti1 = scale_image[iii*slice_size + jjj*width + kkk];
			if(ttti1 != 0) 
				histogram_gscale_regions[ttti1]++;
			}

			dt_8_in = (unsigned char *)malloc(volume_size*sizeof(unsigned char));
			dt_8_out = (unsigned char *)malloc(volume_size*sizeof(unsigned char));

			if (dt_8_out==NULL) 
			{
				fprintf(stderr,"Error allocating memory\n");
				exit(-1);
			}

			/**************************************************************************/
			/* Finding k largest g-scale regions and no. of voxels within each region */ 
			/***************************************************************************/
			for(aaa=0; (int)aaa<volume_size; aaa++)
			{
				mean_scene[aaa] = 0;
				max_scene[aaa] = 0;
			}


			for(num_largest_regions = 1; num_largest_regions<(NUM_REGIONS_PAINT+1); num_largest_regions++)
			{
				min=volume_size; max = 0;
				for(intensity = 0; intensity<(SCALE_MAX+1); intensity++)
				{
					ttti1 = histogram_gscale_regions[intensity];
					if(ttti1 < min ) min = ttti1;
					if(ttti1 > max ) max = ttti1;
				}

				for(intensity = 0; intensity<(SCALE_MAX + 1); intensity++)
				{
					if(histogram_gscale_regions[intensity] == max) 
					{
						LARGEST_REGION[num_largest_regions - 1] = intensity;
					}
				}

				SIZE_REGION[num_largest_regions - 1] = max;
				printf("%d\n", (int)max);
				// Setting the max. value in the histogram to 0.
				histogram_gscale_regions[LARGEST_REGION[num_largest_regions - 1]] = 0;

				/**************************************************************************/
				/*               Painting largest g-scale regions and                     */
				/*          and computing mean intensity within each g-scale region       */
				/**************************************************************************/

				for(aaa=0;(int)aaa<volume_size;aaa++)
				{if (scale_image[aaa]==LARGEST_REGION[num_largest_regions - 1])
				{num_vox_region++;
				dt_8_in[aaa] = num_largest_regions;
				pppi1 = datascale16[aaa];
				if(pppi1 > maxint ) maxint = pppi1;
				mean_region_intensity = mean_region_intensity + pppi1;
				}	
				else if ((num_largest_regions==1) && (scale_image[aaa]!=LARGEST_REGION[num_largest_regions - 1]))
					dt_8_in[aaa]=0;
				}
				MEAN_INT_REGION[num_largest_regions - 1] = (int) mean_region_intensity/num_vox_region;
				MAX_INT_REGION[num_largest_regions - 1] = (int) maxint;
				num_vox_region = 0;
				mean_region_intensity = 0.0;
				maxint = 0.0;
			}

			/*******************************************************************/
			/*           Generating a scene where the voxels have their        */
			/*	       values replaced by the mean intensities of the        */
			/*	       g-scale region in which they belong                   */
			/*******************************************************************/

			for(num_largest_regions = 1; num_largest_regions<(NUM_REGIONS_PAINT+1); num_largest_regions++)
				for(aaa=0;(int)aaa<volume_size;aaa++)
				{if (scale_image[aaa]==LARGEST_REGION[num_largest_regions-1])
				{mean_scene[aaa] = (unsigned short)MEAN_INT_REGION[num_largest_regions-1];
			max_scene[aaa] = (unsigned short)MAX_INT_REGION[num_largest_regions-1];
			num_vox_inhomo++;
			}
				}

				/*****************************************************************************/

				memcpy(&vh_out, &vh_in, sizeof(vh_in));

				vh_out.scn.num_of_bits = 8;
				vh_out.scn.bit_fields[1] = vh_out.scn.num_of_bits - 1;
				vh_out.scn.smallest_density_value[0] = 0;
				vh_out.scn.largest_density_value[0] = 255;

				fp_out = fopen("orig_largestgscale.IM0","wb+");
				if(fp_out == NULL)
				{
					printf("Open file %s error\n",outfile[0]);
					exit(-1);
				}

				num_of_bytes = vh_out.scn.num_of_bits/8;

				error_code = VWriteHeader(fp_out, &vh_out, group, element);
				if ((error_code&&error_code<106) ||
					VWriteData((char *)dt_8_in,num_of_bytes,volume_size,fp_out,&j))
				{
					fprintf(stderr, "Error writing data.\n");
					exit(-1);
				}
				fclose(fp_out);

				/*****************************************************************************/

				min=65535; max = 0;
				for(aaa = 0; (int)aaa<volume_size; aaa++)
				{
					ttti1 = (int) max_scene[aaa];
					if(ttti1 < min ) min = ttti1;
					if(ttti1 > max ) max = ttti1;
				}

				memcpy(&vh_out, &vh_in, sizeof(vh_in));
				vh_out.scn.num_of_bits = 16;
				vh_out.scn.bit_fields[1] = vh_out.scn.num_of_bits - 1;
				vh_out.scn.smallest_density_value[0] = 0;
				vh_out.scn.largest_density_value[0] = (float) max;

				fp_out = fopen("max_img.IM0","wb+");
				if(fp_out == NULL)
				{
					printf("Open file %s error\n",outfile[0]);
					exit(-1);
				}

				num_of_bytes = vh_out.scn.num_of_bits/8;
				error_code = VWriteHeader(fp_out, &vh_out, group, element);
				if ((error_code&&error_code<106) ||
					VWriteData((char *)max_scene,num_of_bytes,volume_size,fp_out,&j))
				{
					fprintf(stderr, "Error writing data.\n");
					exit(-1);
				}
				fclose(fp_out);

				/*****************************************************************************/

				sprintf(command,"ndthreshold %s temp_bin.BIM 0 %d %d","max_img.IM0",1,(int) max+1);
				system(command);

				if(scale_map)
					free(scale_map);

				printf("\rG-Scale computation is done.     \n"); fflush(stdout);	

				if( LARGEST_REGION != NULL )
					free(LARGEST_REGION); 
				if( SIZE_REGION != NULL )
					free(SIZE_REGION); 
				if( MEAN_INT_REGION != NULL )
					free(MEAN_INT_REGION); 
				if( MAX_INT_REGION != NULL )
					free(MAX_INT_REGION); 

				return;   
}

/********************************************************************************************
* FUNCTION: inhomo_correct
* DESCRIPTION: output the corrected image given the original image
* PARAMETERS: original image, mask binary image, corrected image, inhomogeneity image and additive
*             or multiplicative mode
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
*  
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 07/07/00 by Ying Zhuge
*    Modified : Jiamin Liu
*    Modified: 11/21/01 inhomo_intensity initialization corrected
*       by Dewey Odhner.
*    Modified : Anant Madabhushi
*    Modified: 3/8/04 dt_object initialized by Dewey Odhner.
*    Modified: 3/8/04 data_cor initialized by Dewey Odhner.
*    Modified: 3/9/04 8-bit scenes handled by Dewey Odhner.
*
*******************************************************************************************/
int inhomo_correct(o_file,rdat_file,b_file,m_file,c_file,i_file,W)

char *o_file,*rdat_file,*b_file,*m_file,*c_file,*i_file;
int W;

{
	int i,j,k,error1, tti1,tti2,tti3,ii,jj, iteration;
	int xx,yy,zz;
	double tt1;
	ViewnixHeader vh,vh1;
	FILE *in,*out,*fp_out;
	unsigned char *ptr, *data_bin;
	char group[6],elem[6];
	regionElem *pTemp;
	voxelElem *pLocation;
	regionElem swap;
	double return_min,return_a,return_b,return_c,return_d,freturn,ftol=1.0e-5;
	double sum_t[10], fmin,fmax;
	int largest_value;
	unsigned short **mean_data16;
	float **data16;
	unsigned char **mean_data8;
	unsigned short *inhomo_int;
	float *inhomo_intensity, *cor_data16;
	double **bV,**cV,**dV,**dir;
	long int volume=0;
	char *dt_object;

	/*****************************************************************/
	/*       Reading in the binary file                               */
	/*****************************************************************/

	in=fopen(b_file,"rb");
	if (in==NULL)
	{
		fprintf(stderr,"Error in opening the binary files\n");
		exit(-1);
	}

	error1=VReadHeader(in,&vh1,group,elem);
	if (error1 && error1<=104) 
	{   
		fprintf(stderr,"Fatal error in reading binary header\n");
		exit(-1);
	}

	if (vh1.gen.data_type!=IMAGE0)
	{
		fprintf(stderr,"Input files should be IMAGE0\n");
		exit(-1);
	}

	if (vh1.scn.num_of_bits!=1)
	{
		fprintf(stderr,"The second input file should be binary file\n");
		exit(-1);
	}

	if (vh1.scn.dimension!=3)
	{
		fprintf(stderr,"The input files should be 3 dimensional\n");
		exit(-1);
	}

	slices = vh1.scn.num_of_subscenes[0];
	width = vh1.scn.xysize[0];
	height =  vh1.scn.xysize[1];
	pxsize = vh1.scn.xypixsz[0];
	pysize = vh1.scn.xypixsz[1];

	slice_size = height*width;
	volume_size = slices*slice_size;
	space = (int)(vh1.scn.loc_of_subscenes[1] - vh1.scn.loc_of_subscenes[0]);

	size_bin = (slice_size*vh1.scn.num_of_bits+7)/8;
	data_bin = (unsigned char *)malloc(size_bin);

	dt_8 = (unsigned char *)malloc(volume_size*sizeof(char));

	// store all connected objects

	if (data_bin==NULL || dt_8==NULL) 
	{
		fprintf(stderr,"Error allocating memory\n");
		exit(-1);
	}

	pGlobal = (voxelElem *)malloc(volume_size*sizeof(voxelElem));
	if(pGlobal == NULL)
	{
		fprintf(stderr,"Error allocating memory\n");
		exit(-1);
	}

	error1 = VSeekData(in,0);
	if (error1 && error1<106)
	{
		fprintf(stderr, "Seek error.\n");
		exit(-1);
	}

	for(ptr=dt_8,i=0;i <slices ;i++,ptr += slice_size)
	{
		if (VReadData((char *)data_bin,1,size_bin,in,&j))
		{
			fprintf(stderr,"Could not read data\n");
			exit(-1);
		}
		bin_to_grey(data_bin,slice_size,ptr,0,1);
	}

	free(data_bin);
	fclose(in);

	/*****************************************************************/
	/*       Reading in the image file                                */
	/*****************************************************************/

	in = fopen(o_file,"rb");
	if (in==NULL)
	{
		fprintf(stderr,"Error in opening the files\n");
		exit(-1);
	}

	error1=VReadHeader(in,&vh,group,elem);
	if (error1 && error1<=104)
	{
		fprintf(stderr,"Fatal error in reading header\n");
		exit(-1);
	}
	fclose(in);


	num_of_bits = vh.scn.num_of_bits;
	largest_value= (int)vh.scn.largest_density_value[0];

	// **************************************************************
	// ***********************************************************************

	// defining a float array to read in the float data and initializing.
	cor_data16 = (float *)malloc(volume_size*sizeof(float));

	for(zz = 0;zz < slices;zz++)
		for(yy = 0;yy < height;yy++)
			for(xx = 0;xx < width; xx++)
			{
				cor_data16[zz*height*width + yy*width + xx] = 0.0;
			}

			data16 = (float **)malloc(FEATURES*sizeof(float*));
			data16[0] = (float *)malloc(FEATURES*volume_size*sizeof(float));
			if(cor_data16 == NULL||data16[0]==NULL)
			{
				fprintf(stderr,"Memory allocation error \n");
				exit(-1);
			}

			memset(data16[0],0,sizeof(float)*volume_size);

			for (i = 0;i< FEATURES;i++)
			{
				data16[i] =  data16[0] + i*volume_size;
				in = fopen(rdat_file,"rb");
				if (in == NULL)
				{
					fprintf(stderr,"Fatal error in opening file\n");
					exit(-1);
				}

				if (fread(data16[i], sizeof(float), volume_size, in) != volume_size)
				{
					fprintf(stderr, "Cannot read file %s \n", rdat_file);
					exit(-1);
				}

				fclose(in);
			}

			/*****************************************************************/
			/*       Reading in the mean image                               */
			/*****************************************************************/

			in = fopen(m_file,"rb");
			if (in==NULL)
			{
				fprintf(stderr,"Error in opening the files\n");
				exit(-1);
			}

			error1=VReadHeader(in,&vh,group,elem);
			if (error1 && error1<=104)
			{
				fprintf(stderr,"Fatal error in reading header\n");
				exit(-1);
			}
			fclose(in);


			num_of_bits = vh.scn.num_of_bits;
			largest_value= (int)vh.scn.largest_density_value[0];

			if(num_of_bits == 8)
			{
				mean_data8 = (unsigned char **)malloc(FEATURES*sizeof(unsigned char*));
				mean_data8[0] = (unsigned char *)malloc(FEATURES*volume_size*sizeof(char));
				if(mean_data8 ==NULL||mean_data8[0]==NULL)
				{
					fprintf(stderr,"Memory allocation error \n");
					exit(-1);
				}
				for (i = 0;i< FEATURES;i++)
				{
					mean_data8[i] =  mean_data8[0] + i*volume_size;
					in = fopen(m_file,"rb");
					if (in == NULL)
					{
						fprintf(stderr,"Fatal error in opening file\n");
						exit(-1);
					}
					error1 = VSeekData(in, 0);
					if ((error1&&error1<106) ||VReadData((char *)mean_data8[i], 1,volume_size, in, &j))
					{
						fprintf(stderr, "Error reading data.\n");
						exit(-1);
					}
					fclose(in);
				}
			}
			else if(num_of_bits == 16)
			{
				mean_data16 = (unsigned short **)malloc(FEATURES*sizeof(unsigned short*));
				mean_data16[0] = (unsigned short *)malloc(FEATURES*volume_size*sizeof(unsigned short));
				if(mean_data16 == NULL||mean_data16[0]==NULL)
				{
					fprintf(stderr,"Memory allocation error \n");
					exit(-1);
				}

				memset(mean_data16[0],0,2*volume_size);

				for (i = 0;i< FEATURES;i++)
				{
					mean_data16[i] =  mean_data16[0] + i*volume_size;
					in = fopen(m_file,"rb");
					if (in == NULL)
					{
						fprintf(stderr,"Fatal error in opening file\n");
						exit(-1);
					}

					error1=VSeekData(in, 0);

					if (error1==0 || error1==106 || error1==107)
						error1=VReadData((char *)mean_data16[i], 2, volume_size, in, &j);	  

					if (error1)
					{
						fprintf(stderr,"Could not read data\n");
						exit(-1);
					}

					fclose(in);
				}
			}

			/*************find the connected object****************/

			pRegion = (regionElem *)malloc(sizeof(regionElem)*Q_SIZE);
			if (pRegion ==NULL)
			{
				fprintf(stderr,"Could not allocate memory.\n");
				exit(-1);
			}

			/* Build Multiplication tables */

			tbz=(unsigned int *)malloc(sizeof(int)*slices);
			tby=(unsigned int *)malloc(sizeof(int)*vh1.scn.xysize[1]);
			if (tbz==NULL || tby==NULL) 
			{
				fprintf(stderr,"Error in allocating data\n");
				exit(-1);
			}
			for(j=vh1.scn.xysize[0],i=0;i<vh1.scn.xysize[1];i++)
				tby[i]=i*j;

			for(j=vh1.scn.xysize[0]*vh1.scn.xysize[1],i=0;i<slices;i++)
				tbz[i]=i*j;

			Q = (voxelS *)malloc(volume_size*sizeof(voxelS));

			/* Generate the actual labels */
			num_sets = GenerateLabels((char *)dt_8,vh1.scn.xysize[0],vh1.scn.xysize[1],slices);

			free(Q);
			free(dt_8);

			/* Free Multiplication tables */
			free(tby); free(tbz);

			if(num_sets>0)
			{
				pSort = (regionElem *)malloc(sizeof(regionElem)*num_sets);
				if(pSort == NULL)
				{
					fprintf(stderr,"Error in allocating data\n");
					exit(-1);
				}
				pTemp = pSort; 
				for(i = 0;i<num_sets;i++)
				{
					*pTemp = *pRegion;
					pTemp++;
					pRegion++;
				}
				free(pRegion-num_sets);

				for(i = 0;i<num_sets-1;i++)
				{
					k = i;
					for(j = i+1;j<num_sets;j++)
						if(pSort[j].num_voxels > pSort[k].num_voxels)
							k = j;
					swap = pSort[i];
					pSort[i] = pSort[k];
					pSort[k] = swap;
				}

				dt_object = (char *)calloc(volume_size, sizeof(char));    
				if (dt_object == NULL)
				{
					fprintf(stderr,"Error allocating memory\n");
					exit(-1);
				}

				/*************get one connected object********/
				pTemp = pSort;
				for(i=0;i<num_sets;i++)
				{
					pLocation = pTemp->pStart;
					tti1 = pTemp->num_voxels;

					for(k=0;k<tti1;k++)
					{
						xx = pLocation->x;
						yy = pLocation->y;
						zz = pLocation->z;

						tti2 = zz*slice_size + yy*width + xx;
						dt_object[tti2]=1;
						pLocation++;
					}
					pTemp++;
				}

				/*************get one connected object end********/

				pMask = (voxelElem *)malloc(volume_size*sizeof(voxelElem));
				if(pMask == NULL)
				{
					fprintf(stderr,"Error allocating memory\n");
					exit(-1);
				}

				pLocation=pMask;
				volume=0;
				for(i = 0;i<slices;i++)
					for(j = 0;j<height;j++) 
						for(k = 0;k<width;k++)
						{
							tti1 =i*height*width + j*width + k;
							if (dt_object[tti1]==1)
							{

								pLocation->x=k;
								pLocation->y=j;
								pLocation->z=i;
								pLocation++;

								volume=volume+1;
							}
						}

						free(dt_object);
						num_sets=1;
						printf("\nObject_sum: Volume %-6d \n", (int)volume);

						pLocation = pMask;
						number_largest_object = volume;

						if (abs(number_largest_object-num_pre)>num_pre*STOP_CONDITION)
						{

							for(i=0;i<num_sets;i++)
							{
								tti1 = volume;

								pLocation = pMask;
								for(k=0;k<tti1;k++)
								{
									tt1 = 1.0;
									xx = pLocation->x;
									yy = pLocation->y;
									zz = pLocation->z;
									tti2 = zz*slice_size + yy*width + xx;

									for(j=0;j<FEATURES;j++)
									{
										if ((num_of_bits == 8)&&(W==0))
											tt1 = tt1*((double)data16[j][tti2]-(double)mean_data8[j][tti2]);
										if ((num_of_bits == 8)&&(W==1))
											tt1 = tt1*((double)data16[j][tti2]/(double)mean_data8[j][tti2]);
										if ((num_of_bits == 16)&&(W==0))
											tt1 = (double)tt1*((float)data16[j][tti2]-(float)mean_data16[j][tti2]);
										if ((num_of_bits == 16)&&(W==1))
											tt1 = (double)tt1*((float)data16[j][tti2]/(float)mean_data16[j][tti2]);
									}
									if (tt1 < 0)
										tt1 = -exp(log(fabs(tt1))/(double)FEATURES);   
									else
										tt1 = exp(log(fabs(tt1))/(double)FEATURES);
									pLocation->inhomo_value = tt1;
									pLocation++;
								}

							}

							aV = (double **)malloc(num_sets*sizeof(double*));
							aV[0] = (double*)malloc(num_sets*10*sizeof(double));
							for(i = 0;i<num_sets;i++)
								aV[i] = aV[0] + i*10;

							for(i = 0;i<num_sets;i++)
								for(j =0 ;j < 10; j++)
									aV[i][j] =0;

							bV = (double **)malloc(num_sets*sizeof(double*));
							bV[0] = (double*)malloc(num_sets*10*sizeof(double));
							for(i = 0;i<num_sets;i++)
								bV[i] = bV[0] + i*10;

							for(i = 0;i<num_sets;i++)
								for(j =0 ;j < 10; j++)
									bV[i][j] =0;

							cV = (double **)malloc(num_sets*sizeof(double*));
							cV[0] = (double*)malloc(num_sets*10*sizeof(double));
							for(i = 0;i<num_sets;i++)
								cV[i] = cV[0] + i*10;

							for(i = 0;i<num_sets;i++)
								for(j =0 ;j < 10; j++)
									cV[i][j] =0;

							dV = (double **)malloc(num_sets*sizeof(double*));
							dV[0] = (double*)malloc(num_sets*10*sizeof(double));
							for(i = 0;i<num_sets;i++)
								dV[i] = dV[0] + i*10;

							for(i = 0;i<num_sets;i++)
								for(j =0 ;j < 10; j++)
									dV[i][j] =0;

							dir = (double **)malloc(10*sizeof(double *));
							dir[0] = (double *)malloc(10*10*sizeof(double));
							for(i = 0;i<10;i++)
								dir[i] = dir[0] + i*10;

							// for each region, compute the n-Dimenensional optimization function
							// return value is kept in aV[i]

							for(i = 0;i<num_sets;i++)
							{
								for(ii=0;ii<10;ii++)
									for(jj=0;jj<10;jj++)
										if(jj==ii)
											dir[ii][jj]= 1.0;
										else
											dir[ii][jj]= 0.0;     

								get_factor_nD(pMask,volume,factor);

								for(j =0 ;j < 10; j++)
									aV[i][j]= 0.0;
								powell(aV[i],dir,10,ftol,&iteration,&freturn, &function_value_nD);
								return_a=freturn;


								for(j =0 ;j < 10; j++)
									bV[i][j]= 0.0;
								powell(bV[i],dir,10,ftol,&iteration,&freturn, &function_value_nD);
								return_b=freturn;

								for(j =0 ;j < 10; j++)
									cV[i][j]= 0.0;
								powell(cV[i],dir,10,ftol,&iteration,&freturn, &function_value_nD);
								return_c=freturn;

								for(j =0 ;j < 10; j++)
									dV[i][j]= 0.0;
								powell(dV[i],dir,10,ftol,&iteration,&freturn, &function_value_nD);
								return_d=freturn;

								return_min=return_a;

								if (return_b < return_min) 
								{
									return_min=return_b;
									for(j =0 ;j < 10; j++) 
										aV[i][j] = bV[i][j];
								}
								if (return_c < return_min)
								{
									return_min=return_c;
									for(j =0 ;j < 10; j++) 
										aV[i][j] = cV[i][j];
								}
								if (return_d < return_min)
								{
									return_min=return_d;
									for(j =0 ;j < 10; j++) 
										aV[i][j] = dV[i][j];
								}

							}

							inhomo_intensity = (float *) malloc(volume_size*sizeof(float));
							if(inhomo_intensity == NULL)
							{
								fprintf(stderr,"Error in allocating memory\n");
								exit(-1);
							}
							memset(inhomo_intensity,0,sizeof(float)*volume_size);

							for (j=0;j<10;j++) 
							{
								bV[0][j]=0;
								sum_t[j]=0;
							}
							if (num_sets==1)
								for (j=0;j<10;j++)
									bV[0][j]=aV[0][j];

							fmin = 65535.0;
							fmax = 0.0;  

							for(zz = 0;zz < slices;zz++)
								for(yy = 0;yy < height;yy++)
									for(xx = 0;xx < width; xx++)
									{
										tt1 = bV[0][0]*xx*xx + bV[0][1]*yy*yy + bV[0][2]*zz*zz + bV[0][3]*xx*yy + 
											bV[0][4]*yy*zz + bV[0][5]*xx*zz + bV[0][6]*xx + bV[0][7]*yy + bV[0][8]*zz + bV[0][9];

										if (W==1)
											tt1=tt1*(double)(100.0);  

										tti3 =zz*height*width + yy*width + xx;
										inhomo_intensity[tti3] = (float)tt1;
										if (tt1<fmin) fmin =tt1;
										if (tt1>fmax) fmax =tt1; 
									}

									if (fmin < 0)
									{
										for(i = 0;i<slices;i++)
											for(j = 0;j<height;j++)
												for(k = 0;k<width;k++)
												{
													tti1 =i*height*width + j*width + k;
													if (Way==0)
														inhomo_intensity[tti1] = (float)(inhomo_intensity[tti1] - fmin);
													else
														inhomo_intensity[tti1]= (float)(inhomo_intensity[tti1] - fmin+50);
												}
												if (Way==0)
												{
													vh1.scn.smallest_density_value[0] = 0 ;
													vh1.scn.largest_density_value[0] = (float)(fmax-fmin);
												}
												else
												{
													vh1.scn.smallest_density_value[0] = 50 ;
													vh1.scn.largest_density_value[0] = (float)(fmax-fmin+50);
												}

									}
									else
									{
										vh1.scn.smallest_density_value[0] =(float)(fmin);
										vh1.scn.largest_density_value[0] = (float)fmax;
									}


									vh1.scn.largest_density_value_valid = 1;
									vh1.scn.smallest_density_value_valid = 1;

									vh1.scn.num_of_bits=16;
									if (vh1.scn.bit_fields_valid) vh1.scn.bit_fields[1]=15;
									vh1.scn.dimension_in_alignment_valid=0;
									vh1.scn.bytes_in_alignment_valid=0;

									out = fopen(i_file,"wb");
									if(out == NULL)
									{
										fprintf(stderr,"Could not open inhomo_file\n");
										exit(-1);
									}
									error1=VWriteHeader(out,&vh1,group,elem);
									if (error1 && error1<=104)
									{
										fprintf(stderr,"Fatal error in writing header\n");
										exit(-1);
									}

									// define another array to make the computation accurate

									inhomo_int=(unsigned short *)malloc(volume_size*sizeof(short));
									for(i=0;i<volume_size;i++)
										inhomo_int[i]=(int)(inhomo_intensity[i])+(int)((inhomo_intensity[i]-(int)(inhomo_intensity[i]))*2); 

									if (VWriteData((char *)inhomo_int,sizeof(short),volume_size,out,&j))
									{
										fprintf(stderr, "Error writing data.\n");
										exit(-1);
									}
									free(inhomo_int); 
									fclose(out);

									for(i = 0;i<FEATURES;i++)
									{
										fp_out=fopen(c_file,"wb+");
										if (fp_out==NULL) 
										{
											fprintf(stderr,"Could not open output file\n");
											exit(-1);
										}

										fmin = 65535;fmax = 0;
										for(zz = 0; zz<slices; zz++)
											for(yy = 0; yy<height; yy++)
												for(xx = 0; xx<width; xx++)
												{
													tti1 = zz*height*width + yy*width + xx;
													if ((W==1)&&(inhomo_intensity[tti1] != 0))
													{
														tt1 = 100.0*(double)data16[i][tti1]/(double)inhomo_intensity[tti1];
														data16[i][tti1] =(float)(tt1);
														cor_data16[tti1] = (float)(tt1);
													}	      
													if (W==0)
													{
														tt1 = data16[i][tti1] - inhomo_intensity[tti1];
														data16[i][tti1] =(float)(tt1);
														cor_data16[tti1] = (float)(tt1);
													}
													if ( tt1 > fmax) fmax = tt1;
													if ( tt1 < fmin) fmin = tt1;
												}

												if(fmin < 0)
												{
													for(zz = 0; zz<slices; zz++)
														for(yy = 0; yy<height; yy++)
															for(xx = 0; xx<width; xx++)
															{
																tti1 = zz*height*width + yy*width + xx;
																data16[i][tti1] = (float)(data16[i][tti1] - fmin);
																cor_data16[tti1] = (float)(cor_data16[i] - fmin);
															}
															vh1.scn.smallest_density_value[0] = 0 ;
															vh1.scn.largest_density_value[0] = (float)(fmax - fmin);
												}
												else
												{
													vh1.scn.smallest_density_value[0] = (float)fmin;
													vh1.scn.largest_density_value[0] = (float)fmax;
												}

												if (fwrite(cor_data16, sizeof(float), volume_size, fp_out) != volume_size)
													fprintf(stderr, "Could not write data.\n"), exit(-1);
												fclose(fp_out);
									}

									for(i = 0;i<FEATURES;i++)
										free(data16[i]);
									free(data16);
									free(cor_data16);
									free(inhomo_intensity);
									free(pSort);
									free(aV[0]);
									free(aV);
									free(bV[0]);  
									free(bV);
									free(cV[0]);  
									free(cV);
									free(dV[0]);
									free(dV);
									free(dir[0]);
									free(dir);
									free(pMask);
									free(pGlobal);
									return 1; 
						}
						else
						{
							for(i = 0;i<FEATURES;i++)
								free(data16[i]);
							free(data16);

							free(pGlobal);
							free(pMask);
							return 0; // difference is small
						}
			}
			else
			{
				for(i = 0;i<FEATURES;i++)
					free(data16[i]);
				free(data16);
				free(pGlobal);
				return 2; // no object
			}

}

/********************************************************************************************
* FUNCTION: GenerateLabels
* DESCRIPTION: find the connected objects satisfied some condition after thresholding the 
*              original image using the computed inteval.
* PARAMETERS:  the array after thresholding the original image, width, hight and slices about
*              the original image
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: the number of connected objects satisfied some condition
*  
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 07/07/00 by Tad
*    Modified : 11/13/00 by Ying Zhuge
*               removed the flag -1 between two adjacent regions
*               sorting regions in terms of the voxel number of region
*
*******************************************************************************************/
int GenerateLabels(char *dt, int w, int h, int s)
{
	short x,y,z;
	int i;
	int volume;
	char *pt;
	regionElem *pTemp;
	int sum_voxel = 0; 
	int nSets=0;

	nSets=0;
	pre_tot_dt = tot_dt = 0;
	i = 0;
	pTemp = pRegion;
	pt = dt;

	for(z=0;z<s;z++)
		for(y=0;y<h;y++)
			for(x=0;x<w;x++)
			{
				if (*pt==1)
				{
					if (nSets==Q_SIZE)
					{
						fprintf(stderr,"Queue cannot hold any more sets\n");
						break;
					}
					nSets++;

					volume = FindObject(dt,w,h,s,x,y,z); //test
					if (volume < VOLUME_THRESHOLD*volume_size)
					{
						nSets--;
						tot_dt=pre_tot_dt;
						push = 0;
					}
					else
					{
						printf("Object# %-5d Volume %-6d \n",nSets,volume);

						(*pTemp).num_voxels = volume;
						(*pTemp).pStart = pGlobal + pre_tot_dt;
						pTemp ++;
						sum_voxel = sum_voxel + volume;

						pre_tot_dt = tot_dt;
						push = 0;
					}
				}
				pt++;
			}

			return nSets;
}

//********************************************************************************************
void PushQ(char *d, short x, short y, short z, int* total_num)
{
	voxelElem voxel;
	int _t;

	*total_num =  *total_num + 1;
	_t=tbz[z] + tby[y] + x; 
	voxel.x = x;
	voxel.y = y;
	voxel.z = z;
	voxel.inhomo_value = 0;
	temp_scale= d[_t];
	d[_t] = 0;           //The point will never be selected again
	pGlobal[tot_dt++] = voxel;
	Q[push].x=x;
	Q[push].y=y; 
	Q[push].z=z; 
	push++;

}



//********************************************************************************************
void PopQ(voxelS* p)
{ 
	push--;
	*p = Q[push]; 

}

/********************************************************************************************
* FUNCTION: FindObject
* DESCRIPTION: find the connected object in a given array, which is a binary image
* PARAMETERS: array, width, hight, slices, starting positions of x,y and z.
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: the number of voxel connected object
*              
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 07/07/00 by Tad
*    Modified by Ying Zhuge
*
*******************************************************************************************/
int FindObject(char *dt, int w, int h,int s,short startx,short starty,short startz)
{
	int  nsum;
	short x,y,z;
	voxelS retP;

	nsum=0;
	x = startx;
	y = starty;
	z = startz;
	PushQ(dt,x,y,z,&nsum); 

	if (CONNECTIVITY==6) {  /* FACE CONNECTED */
		while (push != 0) {
			PopQ(&retP);
			x = retP.x;
			y = retP.y;
			z = retP.z;

			if (x!=0 && dt[tbz[z]+tby[y]+x-1]==1)  
				PushQ(dt,x-1,y,z,&nsum);
			if (x!=(w-1) && dt[tbz[z]+tby[y]+x+1]==1)
				PushQ(dt,x+1,y,z,&nsum);
			if (y!=0 && dt[tbz[z]+tby[y-1]+x]==1)   
				PushQ(dt,x,y-1,z,&nsum);
			if (y!=(h-1) && dt[tbz[z]+tby[y+1]+x]==1)
				PushQ(dt,x,y+1,z,&nsum);
			if (z!=0 && dt[tbz[z-1]+tby[y]+x]==1) 
				PushQ(dt,x,y,z-1,&nsum);
			if (z!=(s-1) && dt[tbz[z+1]+tby[y]+x]==1) 
				PushQ(dt,x,y,z+1,&nsum);

			/* Face Connected */
			/*******************
			if (x!=0 && dt[tbz[z]+tby[y]+x-1]==1)  
			PushQ(dt,x-1,y,z,&nsum);
			if (x!=(w-1) && dt[tbz[z]+tby[y]+x+1]==1)
			PushQ(dt,x+1,y,z,&nsum);
			if (y!=0 && dt[tbz[z]+tby[y-1]+x]==1)   
			PushQ(dt,x,y-1,z,&nsum);
			if (y!=(h-1) && dt[tbz[z]+tby[y+1]+x]==1)
			PushQ(dt,x,y+1,z,&nsum);
			if (z!=0 && dt[tbz[z-1]+tby[y]+x]==1)  
			PushQ(dt,x,y,z-1,&nsum);
			if (z!=(s-1) && dt[tbz[z+1]+tby[y]+x]==1)
			PushQ(dt,x,y,z+1,&nsum);
			*******************/

		}
	}
	else if (CONNECTIVITY==18) { /* EDGE CONNECTED */
		while (push != 0) {
			PopQ(&retP);
			x = retP.x;
			y = retP.y;
			z = retP.z;

			/* Face Connected */
			if (x!=0 && dt[tbz[z]+tby[y]+x-1]==1)  
				PushQ(dt,x-1,y,z,&nsum);
			if (x!=(w-1) && dt[tbz[z]+tby[y]+x+1]==1)
				PushQ(dt,x+1,y,z,&nsum);
			if (y!=0 && dt[tbz[z]+tby[y-1]+x]==1)   
				PushQ(dt,x,y-1,z,&nsum);
			if (y!=(h-1) && dt[tbz[z]+tby[y+1]+x]==1)
				PushQ(dt,x,y+1,z,&nsum);
			if (z!=0 && dt[tbz[z-1]+tby[y]+x]==1)    
				PushQ(dt,x,y,z-1,&nsum);
			if (z!=(s-1) && dt[tbz[z+1]+tby[y]+x]==1)
				PushQ(dt,x,y,z+1,&nsum);

			/* Edge Connected */
			if (x!=0 && z!=0 && dt[tbz[z-1]+tby[y]+x-1]==1)       
				PushQ(dt,x-1,y,z-1,&nsum);
			if (x!=0 && z!=(s-1) && dt[tbz[z+1]+tby[y]+x-1]==1)   
				PushQ(dt,x-1,y,z+1,&nsum);
			if (x!=(w-1) && z!=0 && dt[tbz[z-1]+tby[y]+x+1]==1)   
				PushQ(dt,x+1,y,z-1,&nsum);
			if (x!=(w-1) && z!=(s-1) && dt[tbz[z-1]+tby[y]+x-1]==1)
				PushQ(dt,x+1,y,z+1,&nsum);

			if (x!=0 && y!=0 && dt[tbz[z]+tby[y-1]+x-1]==1)        
				PushQ(dt,x-1,y-1,z,&nsum);
			if (x!=0 && y!=(h-1) && dt[tbz[z]+tby[y+1]+x-1]==1)   
				PushQ(dt,x-1,y+1,z,&nsum);
			if (x!=(w-1) && y!=0 && dt[tbz[z]+tby[y-1]+x+1]==1)   
				PushQ(dt,x+1,y-1,z,&nsum);
			if (x!=(w-1) && y!=(h-1) && dt[tbz[z]+tby[y+1]+x-1]==1)
				PushQ(dt,x+1,y+1,z,&nsum);

			if (y!=0 && z!=0 && dt[tbz[z-1]+tby[y-1]+x]==1)        
				PushQ(dt,x,y-1,z-1,&nsum);
			if (y!=0 && z!=(s-1) && dt[tbz[z+1]+tby[y-1]+x]==1)    
				PushQ(dt,x,y-1,z+1,&nsum);
			if (y!=(h-1) && z!=0 && dt[tbz[z-1]+tby[y+1]+x]==1)    
				PushQ(dt,x,y+1,z-1,&nsum);
			if (y!=(h-1) && z!=(s-1) && dt[tbz[z-1]+tby[y+1]+x]==1)
				PushQ(dt,x,y+1,z+1,&nsum);

		}
	}
	else {  /* VERTEX CONNECTED */
		while (push != 0) {
			PopQ(&retP);
			x = retP.x;
			y = retP.y;
			z = retP.z;

			/* Face Connected */
			if (x!=0 && dt[tbz[z]+tby[y]+x-1]==1)   
				PushQ(dt,x-1,y,z,&nsum);
			if (x!=(w-1) && dt[tbz[z]+tby[y]+x+1]==1)
				PushQ(dt,x+1,y,z,&nsum);
			if (y!=0 && dt[tbz[z]+tby[y-1]+x]==1)    
				PushQ(dt,x,y-1,z,&nsum);
			if (y!=(h-1) && dt[tbz[z]+tby[y+1]+x]==1)
				PushQ(dt,x,y+1,z,&nsum);
			if (z!=0 && dt[tbz[z-1]+tby[y]+x]==1)    
				PushQ(dt,x,y,z-1,&nsum);
			if (z!=(s-1) && dt[tbz[z+1]+tby[y]+x]==1)
				PushQ(dt,x,y,z+1,&nsum);

			/* Edge Connected */
			if (x!=0 && z!=0 && dt[tbz[z-1]+tby[y]+x-1]==1)       
				PushQ(dt,x-1,y,z-1,&nsum);
			if (x!=0 && z!=(s-1) && dt[tbz[z+1]+tby[y]+x-1]==1)   
				PushQ(dt,x-1,y,z+1,&nsum);
			if (x!=(w-1) && z!=0 && dt[tbz[z-1]+tby[y]+x+1]==1)   
				PushQ(dt,x+1,y,z-1,&nsum);
			if (x!=(w-1) && z!=(s-1) && dt[tbz[z-1]+tby[y]+x-1]==1)
				PushQ(dt,x+1,y,z+1,&nsum);

			if (x!=0 && y!=0 && dt[tbz[z]+tby[y-1]+x-1]==1)      
				PushQ(dt,x-1,y-1,z,&nsum);
			if (x!=0 && y!=(h-1) && dt[tbz[z]+tby[y+1]+x-1]==1) 
				PushQ(dt,x-1,y+1,z,&nsum);
			if (x!=(w-1) && y!=0 && dt[tbz[z]+tby[y-1]+x+1]==1) 
				PushQ(dt,x+1,y-1,z,&nsum);
			if (x!=(w-1) && y!=(h-1) && dt[tbz[z]+tby[y+1]+x-1]==1)
				PushQ(dt,x+1,y+1,z,&nsum);

			if (y!=0 && z!=0 && dt[tbz[z-1]+tby[y-1]+x]==1)      
				PushQ(dt,x,y-1,z-1,&nsum);
			if (y!=0 && z!=(s-1) && dt[tbz[z+1]+tby[y-1]+x]==1)   
				PushQ(dt,x,y-1,z+1,&nsum);
			if (y!=(h-1) && z!=0 && dt[tbz[z-1]+tby[y+1]+x]==1)   
				PushQ(dt,x,y+1,z-1,&nsum);
			if (y!=(h-1) && z!=(s-1) && dt[tbz[z-1]+tby[y+1]+x]==1)
				PushQ(dt,x,y+1,z+1,&nsum);

			/* Vertex Connected */
			if (x!=0 && y!=0 && z!=0 && 
				dt[tbz[z-1]+tby[y-1]+x-1]==1) 
				PushQ(dt,x-1,y-1,z-1,&nsum);
			if (x!=0 && y!=0 && z!=(s-1) && 
				dt[tbz[z+1]+tby[y-1]+x-1]==1) 
				PushQ(dt,x-1,y-1,z+1,&nsum);
			if (x!=0 && y!=(h-1) && z!=0 && 
				dt[tbz[z-1]+tby[y+1]+x-1]==1) 
				PushQ(dt,x-1,y+1,z-1,&nsum);
			if (x!=(w-1) && y!=0 && z!=0 && 
				dt[tbz[z-1]+tby[y-1]+x+1]==1)
				PushQ(dt,x+1,y-1,z-1,&nsum);
			if (x!=0 && y!=(h-1) && z!=(s-1) 
				&& dt[tbz[z+1]+tby[y+1]+x-1]==1) 
				PushQ(dt,x-1,y+1,z+1,&nsum);
			if (x!=(w-1) && y!=(h-1) && z!=0 
				&& dt[tbz[z-1]+tby[y+1]+x+1]==1) 
				PushQ(dt,x+1,y+1,z-1,&nsum);
			if (x!=(w-1) && y!=0 && z!=(s-1)
				&& dt[tbz[z+1]+tby[y-1]+x+1]==1)
				PushQ(dt,x+1,y-1,z+1,&nsum);
			if (x!=(w-1) && y!=(h-1) && z!=(s-1) 
				&& dt[tbz[z+1]+tby[y+1]+x+1]==1)
				PushQ(dt,x+1,y+1,z+1,&nsum);

		}
	}
	return nsum;
}

/****************************************************************************************************
* FUNCTION: bin_to_grey
* DESCRIPTION: convert binary image to grey 8-bit image 
* PARAMETERS: the binary image array, volume size of the image, grey image array, min intensity, 
*             max intensity
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
*     
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 07/07/00 by Tad
*
***************************************************************************************************/
void bin_to_grey(unsigned char *bin_buffer, int length, unsigned char *grey_buffer, int min_value,int max_value)
{
	register int i, j;
	static unsigned char mask[8]= { 1,2,4,8,16,32,64,128 };
	unsigned char *bin;
	unsigned char *grey;

	bin = bin_buffer;
	grey = grey_buffer;

	for(j=7,i=length; i>0; i--)    {
		if ( (*bin & mask[j]) )  {
			*grey = max_value;
		}
		else 
			*grey = min_value;

		grey++;

		if (j==0) {
			bin++; j=7;
		}
		else
			j--;
	}
}

/**********************************************************************************************
* FUNCTION: mnbrak
* DESCRIPTION:Given a function func, and given distinct initial points ax and bx, this routine
*  searches in the downhill direction (defined by the function as evaluated at the initial points) 
* and returns new points ax, bx,cx that bracket a minimum of the function. Also returned are the
*  function values at the three points,fa,fb,and fc.
*********************************************************************************************/

int mnbrak(double *ax, double *bx,double *cx, double *fa, double *fb,double *fc,double (*func)(double))
{
	double ulim,u,r,q,fu,dum;

	*fa = (*func)(*ax);
	*fb = (*func)(*bx);
	if (*fb > *fa)
	{
		SHFT(dum,*ax,*bx,dum);
		SHFT(dum,*fb,*fa,dum);
	}
	*cx = (*bx) +GOLD*(*bx-*ax);
	*fc = (*func)(*cx);
	while (*fb > *fc)
	{
		r = (*bx-*ax)*(*fb-*fc);
		q = (*bx-*cx)*(*fb-*fa);
		u = (*bx)-((*bx-*cx)*q-(*bx-*ax)*r)/(2.0*SIGN(FMAX(fabs(q-r),TINY),q-r));
		ulim = (*bx)+GLIMIT*(*cx-*bx);

		if((*bx-u)*(u-*cx)>0.0)
		{
			fu = (*func)(u);
			if(fu < *fc)
			{
				*ax = (*bx);
				*bx = u;
				*fa=(*fb);
				*fb = fu;
				return 0;
			}
			else if(fu>*fb)
			{
				*cx = u;
				*fc = fu;
				return 0;
			}
			u = (*cx)+GOLD*(*cx-*bx);
			fu = (*func)(u);
		}
		else if((*cx-u)*(u-ulim)>0.0)
		{
			fu = (*func)(u);
			if(fu<*fc)
			{
				SHFT(*bx,*cx,u,*cx+GOLD*(*cx-*bx));
				SHFT(*fb,*fc,fu,(*func)(u));
			}
		}
		else if((u-ulim)*(ulim-*cx)>=0.0)
		{
			u = ulim;
			fu = (*func)(u);
		}
		else
		{
			u = (*cx)+GOLD*(*cx-*bx);
			fu = (*func)(u);
		}
		SHFT(*ax,*bx,*cx,u);
		SHFT(*fa,*fb,*fc,fu);
	}
	return 1;
}

/*******************************************************************************************/
/********************************************************************************************
* FUNCTION: brent
* DESCRIPTION: Given a function, and given a bracketing triplet of abscissas ax,bx,cx(such 
* that bx is between ax and cx, and f(bx) is less than both f(ax) and f(cx)), this routine 
* isolates the minimum to a fractional precision of about tol using Brent's method. The 
* abscissa of the minimum is returned as xmin, and the minimum function value is returned 
* as brent, the returned function value. 
*
* PARAMETERS:
*    ax,bx,cx : a bracketing triplet of abscissas
*    *f       : function address;
*    tol      : fractional precision 
*    *xmin    : minimum abscissa
*    
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
*      *xmin
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 11/05/00 by Ying Zhuge
*
*******************************************************************************************/
double brent(double ax,double bx,double cx,double (*f)(double),double tol,double *xmin)
{

	int iter;
	double a,b,d,etemp,fu,fv,fw,fx,p,q,r,tol1,tol2,u,v,w,x,xm;
	double e = 0.0;

	a = (ax < cx ? ax : cx);
	b = (ax > cx ? ax : cx);

	x = w = v = bx;
	fw = fv = fx = (*f)(x);
	for (iter=0;iter<ITMAX;iter++)
	{
		xm = 0.5*(a+b);
		tol2 = 2.0*(tol1 = tol * fabs(x)+ZEPS);
		if(fabs(x-xm) <= (tol2-0.5*(b-a)))

		{
			*xmin = x;
			return fx;
		}
		if(fabs(e)>tol1)
		{
			r = (x-w)*(fx-fv);
			q = (x-v)*(fx-fw);
			p = (x-v)*q-(x-w)*r;
			q = 2.0*(q-r);
			if(q>0.0) p = -p;
			q = fabs(q);
			etemp = e;
			e = d;
			if(fabs(p)>=fabs(0.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x))
				d = CGOLD*(e=(x>=xm ? a-x:b-x));
			else 
			{
				d = p/q;
				u = x+d;
				if(u-a < tol2 || b-u < tol2)
					d = SIGN(tol1,xm-x);
			}
		}
		else
		{
			d = CGOLD*(e = (x>=xm?a-x: b-x));
		}
		u = (fabs(d)>=tol1?x+d:x+SIGN(tol1,d));
		fu = (*f)(u);

		if(fu<=fx)
		{
			if(u>=x)
				a = x;
			else 
				b = x;
			SHFT(v,w,x,u);
			SHFT(fv,fw,fx,fu);
		}
		else
		{
			if(u<x) 
				a = u;
			else
				b = u;
			if(fu<=fw || w ==x)
			{
				v = w;
				w = u;
				fv = fw;
				fw = fu;
			}
			else if (fu <=fv ||v ==x || v ==w)
			{
				v = u;
				fv = fu;
			}
		}
	}
	fprintf(stderr,"too many iterations in brent \n");
	*xmin = x;
	return fx;
}

/*******************************************************************************************/
/********************************************************************************************
* FUNCTION: powell
* DESCRIPTION: Minimization of a function func of n variables. Input consists of an initial starting point 
*              p[1..n]; an initial matrix xi[1..n][1..n], whose columns contain the initial set of directions
*              (usually the n unit vectors); and ftol, the fractional tolerance in the function value such that
*              failure to decrease by more than this amount on one iteration signals doneness. On output, p is 
*              set to the best point found, xi is the then-current direction set, fret is the returned function
*              value at p, and iter is the number of iterations taken. The routine linmin is used. 
*
* PARAMETERS:
*    p[]: n-dimensional point location
*    xi[][]:n*n direction matrix; 
*    n : dimension number;
*    ftol: fractional tolerance in the function value
*    iter: iteration number
*    *fret : return function value
*    *func: function address 
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
*      function value
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 11/05/00 by Ying Zhuge
*
*******************************************************************************************/

int powell(double p[],double **xi,int n, double ftol,int *iter, double *fret, double (*func)(double []))
{

	void linmin(double p[],double xi[],int n, double *fret, double (*func)(double []));
	int i,ibig,j;
	double del,fp,fptt,t;
	double *pt,*ptt,*xit;

	pt = vector(n);
	ptt = vector(n);
	xit = vector(n);
	*fret = (*func)(p);


	for (j = 0;j<n;j++) pt[j]=p[j];
	for (*iter = 1;;++(*iter))
	{
		fp = (*fret);
		ibig = 0;
		del = 0.0;
		for(i=0;i<n;i++)
		{
			for (j=0;j<n;j++)
				xit[j] = xi[j][i];
			fptt = (*fret);
			linmin(p,xit,n,fret,func);
			if(fptt - (*fret)>del)
			{
				del = fptt-(*fret);
				ibig = i;
			}
		}
		if((double)4.0*(fp-(*fret))<(double)ftol*((double)fabs(fp)+(double)fabs(*fret))+(double)TINY)
		{
			free(xit);
			free(ptt);
			free(pt);
			return 0;
		}
		if(*iter ==ITMAX)
		{
			fprintf(stderr,"powell exceeding maximum iterations. \n");
			free(xit);
			free(ptt);
			free(pt);
			return 0;

		}

		for(j = 0;j<n;j++)
		{
			ptt[j] = (double)2.0*p[j]-pt[j];
			xit[j] = p[j]-pt[j];
			pt[j]=p[j];
		}
		fptt = (*func)(ptt);
		if(fptt < fp)
		{
			t = (double)2.0*(fp-2.0*(*fret)+fptt)*(double)SQR(fp-(*fret)-del)-del*(double)SQR(fp-fptt);
			if(t<0.0)
			{
				linmin(p,xit,n,fret,func);
				for(j= 0;j<n;j++)
				{
					xi[j][ibig]=xi[j][n-1];
					xi[j][n-1] = xit[j];
				}
			}
		}
	}
}


/*******************************************************************************************/
/********************************************************************************************
* FUNCTION: linmin
* DESCRIPTION: Given an n-dimensional point p and an n-dimensioanl xi direction,
*              moves and reset p to where the function func(p) takes on a minimum
*              along the direction xi from p,and replaces xi by the actual vector
*              displacement that p was moved. Also returns as fret the value of func
*              at the returned location p. This is actually all accomplished by calling
*               the routines mnbrak and brent.
* PARAMETERS:
*    p[]: n-dimensional point location
*    xi[]:n-dimensional direction; 
*    n : dimension number;
*    *fret: minimum function value
*    *func: function address 
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
*      function value
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 11/05/00 by Ying Zhuge
*
*******************************************************************************************/

void linmin(double p[],double xi[],int n, double *fret, double (*func)(double []))
{
	double f1dim(double x);

	int j;
	double xx,xmin,fx,fb,fa,bx,ax;

	ncom = n;
	pcom = vector(n);
	xicom = vector(n);
	nrfunc = func;
	for(j=0;j<n;j++)
	{
		pcom[j]=p[j];
		xicom[j]=xi[j];
	}
	ax = 0.0;
	xx = 1.0;
	mnbrak(&ax,&xx,&bx,&fa,&fx,&fb,f1dim);
	*fret = brent(ax,xx,bx,f1dim,TOL,&xmin);
	for(j=0;j<n;j++)
	{
		xi[j] *=xmin;
		p[j]+=xi[j];
	}
	free(xicom);
	free(pcom);
}

/*****************************************************************************
* FUNCTION: f1dim
* DESCRIPTION: compute the function value for n-dimension
* PARAMETERS:
*    x: direction displacement
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
*      function value
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 11/05/00 by Ying Zhuge
*
*****************************************************************************/
double f1dim(double x)
{
	int j;
	double f;
	double *xt;

	xt = vector(ncom);
	for(j=0;j<ncom;j++)
		xt[j]=pcom[j]+x*xicom[j];
	f = (*nrfunc)(xt);
	free(xt);
	return f;
}

/*****************************************************************************
* FUNCTION: get_factor_nD
* DESCRIPTION: compute the factor array for estimated function
* PARAMETERS:
*    *pElem: point to regionElem array;
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
*      factor array
*
*      f(x,y,z) = a0*X*X+a1*Y*Y+a2*Z*Z+a3*X*Y+a4*Y*Z+a5*X*Z+a6*X+a7*Y+a8*Z+a9
*      (f(x,y,z)-inhomo(x,y,z)) * (f(x,y,z)-inhomo(x,y,z))
*
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 11/14/00 by Ying Zhuge
*
*****************************************************************************/
int get_factor_nD(voxelElem *pElem, long int mask_volume, double factor[])
{
	long int i,tti1;
	int x,y,z;
	double inhomo;
	voxelElem *pLocation;

	tti1 = mask_volume;
	pLocation = pElem;

	for(i=0;i<66;i++)
		factor[i] = 0.0;

	for(i = 0;i<tti1;i++)
	{ 
		x = pLocation->x;
		y = pLocation->y;
		z = pLocation->z;
		inhomo = pLocation->inhomo_value;

		factor[0] = factor[0] + (double)x*x*x*x;
		factor[1] = factor[1] + (double)2*x*x*y*y;
		factor[2] = factor[2] + (double)2*x*x*z*z;
		factor[3] = factor[3] + (double)2*x*x*x*y;
		factor[4] = factor[4] + (double)2*x*x*y*z;
		factor[5] = factor[5] + (double)2*x*x*x*z;
		factor[6] = factor[6] + (double)2*x*x*x;
		factor[7] = factor[7] + (double)2*x*x*y;
		factor[8] = factor[8] + (double)2*x*x*z;
		factor[9] = factor[9] + (double)2*x*x;
		factor[10] = factor[10] + (double)2*x*x*inhomo;

		factor[11] = factor[11] + (double)y*y*y*y;
		factor[12] = factor[12] + (double)2*y*y*z*z;
		factor[13] = factor[13] + (double)2*x*y*y*y;
		factor[14] = factor[14] + (double)2*y*y*y*z;
		factor[15] = factor[15] + (double)2*x*y*y*z;
		factor[16] = factor[16] + (double)2*x*y*y;
		factor[17] = factor[17] + (double)2*y*y*y;
		factor[18] = factor[18] + (double)2*y*y*z;
		factor[19] = factor[19] + (double)2*y*y;
		factor[20] = factor[20] + (double)2*y*y*inhomo;

		factor[21] = factor[21] + (double)z*z*z*z;
		factor[22] = factor[22] + (double)2*x*y*z*z;
		factor[23] = factor[23] + (double)2*y*z*z*z;
		factor[24] = factor[24] + (double)2*x*z*z*z;
		factor[25] = factor[25] + (double)2*x*z*z;
		factor[26] = factor[26] + (double)2*y*z*z;
		factor[27] = factor[27] + (double)2*z*z*z;
		factor[28] = factor[28] + (double)2*z*z;
		factor[29] = factor[29] + (double)2*z*z*inhomo;

		factor[30] = factor[30] + (double)x*x*y*y;
		factor[31] = factor[31] + (double)2*x*y*y*z;
		factor[32] = factor[32] + (double)2*x*x*y*z;
		factor[33] = factor[33] + (double)2*x*x*y;
		factor[34] = factor[34] + (double)2*x*y*y;
		factor[35] = factor[35] + (double)2*x*y*z;
		factor[36] = factor[36] + (double)2*x*y;
		factor[37] = factor[37] + (double)2*x*y*inhomo;

		factor[38] = factor[38] +(double)y*y*z*z;
		factor[39] = factor[39] +(double)2*x*y*z*z;
		factor[40] = factor[40] +(double)2*x*y*z;
		factor[41] = factor[41] +(double)2*y*y*z;
		factor[42] = factor[42] +(double)2*y*z*z;
		factor[43] = factor[43] +(double)2*y*z;
		factor[44] = factor[44] +(double)2*y*z*inhomo;

		factor[45] = factor[45] + (double)x*x*z*z;
		factor[46] = factor[46] + (double)2*x*x*z;
		factor[47] = factor[47] + (double)2*x*y*z;
		factor[48] = factor[48] + (double)2*x*z*z;
		factor[49] = factor[49] + (double)2*x*z;
		factor[50] = factor[50] + (double)2*x*z*inhomo;

		factor[51] = factor[51] + (double)x*x;
		factor[52] = factor[52] + (double)2*x*y;
		factor[53] = factor[53] + (double)2*x*z;
		factor[54] = factor[54] + (double)2*x;
		factor[55] = factor[55] + (double)2*x*inhomo;

		factor[56] = factor[56] + (double)y*y;
		factor[57] = factor[57] + (double)2*y*z;
		factor[58] = factor[58] + (double)2*y;
		factor[59] = factor[59] + (double)2*y*inhomo;

		factor[60] = factor[60] + (double)z*z;
		factor[61] = factor[61] + (double)2*z;
		factor[62] = factor[62] + (double)2*z*inhomo;

		factor[63] = factor[63] + (double)1;
		factor[64] = factor[64] + (double)2*inhomo;

		factor[65] = factor[65] + (double)inhomo*inhomo;

		pLocation++;
	}

	return 0;
}

/*****************************************************************************
* FUNCTION: function_value_nD
* DESCRIPTION: compute the function value
* PARAMETERS:
*    a: parameters value
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: function get_factor should be called first 
* RETURN VALUE:
*      function value
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 11/14/00 by Ying Zhuge
*
*****************************************************************************/
double function_value_nD(double a[10])
{
	double ret;

	ret = (double)a[0]*a[0]*factor[0] + (double)a[0]*a[1]*factor[1] + (double)a[0]*a[2]*factor[2] + 
		(double)a[0]*a[3]*factor[3] + (double)a[0]*a[4]*factor[4] + (double)(double)a[0]*a[5]*factor[5] +
		(double)a[0]*a[6]*factor[6] + (double)a[0]*a[7]*factor[7] + (double)a[0]*a[8]*factor[8] +
		(double)a[0]*a[9]*factor[9] - (double)a[0]*factor[10] +
		(double)a[1]*a[1]*factor[11] + (double)a[1]*a[2]*factor[12] + (double)a[1]*a[3]*factor[13] +
		(double)a[1]*a[4]*factor[14] + (double)a[1]*a[5]*factor[15] + (double)a[1]*a[6]*factor[16] +
		(double)a[1]*a[7]*factor[17] + (double)a[1]*a[8]*factor[18] + (double)a[1]*a[9]*factor[19] -
		(double)a[1]*factor[20] +
		(double)a[2]*a[2]*factor[21] + (double)a[2]*a[3]*factor[22] + (double)a[2]*a[4]*factor[23] +
		(double)a[2]*a[5]*factor[24] + (double)a[2]*a[6]*factor[25] + (double)a[2]*a[7]*factor[26] +
		(double)a[2]*a[8]*factor[27] + (double)a[2]*a[9]*factor[28] - (double)a[2]*factor[29] +
		(double)a[3]*a[3]*factor[30] + (double)a[3]*a[4]*factor[31] + (double)a[3]*a[5]*factor[32] +
		(double)a[3]*a[6]*factor[33] + (double)a[3]*a[7]*factor[34] + (double)a[3]*a[8]*factor[35] +
		(double)a[3]*a[9]*factor[36] - (double)a[3]*factor[37] +
		(double)a[4]*a[4]*factor[38] + (double)a[4]*a[5]*factor[39] + (double)a[4]*a[6]*factor[40] +
		(double)a[4]*a[7]*factor[41] + (double)a[4]*a[8]*factor[42] + (double)a[4]*a[9]*factor[43] -
		(double)a[4]*factor[44]+
		(double)a[5]*a[5]*factor[45] + (double)a[5]*a[6]*factor[46] + (double)a[5]*a[7]*factor[47] + 
		(double)a[5]*a[8]*factor[48] + (double)a[5]*a[9]*factor[49] - (double)a[5]*factor[50] +
		(double)a[6]*a[6]*factor[51] + (double)a[6]*a[7]*factor[52] + (double)a[6]*a[8]*factor[53] +
		(double)a[6]*a[9]*factor[54] - (double)a[6]*factor[55]+
		(double)a[7]*a[7]*factor[56] + (double)a[7]*a[8]*factor[57] + (double)a[7]*a[9]*factor[58] -
		(double)a[7]*factor[59] +
		(double)a[8]*a[8]*factor[60] + (double)a[8]*a[9]*factor[61] - (double)a[8]*factor[62] +
		(double)a[9]*a[9]*factor[63] - (double)a[9]*factor[64] + (double)factor[65];

	return ret;

}


/*****************************************************************************
* FUNCTION: get_factor_1D
* DESCRIPTION: compute the factor array for estimated function
* PARAMETERS:
*    *pElem: point to regionElem array;
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: 
*      factor array
*
*      f(x,y,z) = a0*X*X+a1*Y*Y+a2*Z*Z+a3*X*Y+a4*Y*Z+a5*X*Z+a6*X+a7*Y+a8*Z+a9
*      (f(x,y,z)-inhomo(x,y,z)) * (f(x,y,z)-inhomo(x,y,z))
*
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 11/14/00 by Ying Zhuge
*
*****************************************************************************/
int get_factor_1D(double cof[])
{
	int i, xx,yy,zz;
	double f1,f2;

	for(i=0;i<3;i++)
		factor_1[i] = 0;

	// ---------------- for whole image ------------------------------
	for(zz = 0;zz < slices; zz++)
		for(yy = 0; yy < height; yy++)
			for(xx = 0;xx < width; xx++)
			{
				f1 = aV[0][0]*xx*xx + aV[0][1]*yy*yy + aV[0][2]*zz*zz + aV[0][3]*xx*yy + aV[0][4]*yy*zz +
					aV[0][5]*xx*zz + aV[0][6]*xx + aV[0][7]*yy + aV[0][8]*zz + aV[0][9];

				f2 = cof[0]*xx*xx + cof[1]*yy*yy + cof[2]*zz*zz + cof[3]*xx*yy + cof[4]*yy*zz +
					cof[5]*xx*zz + cof[6]*xx + cof[7]*yy + cof[8]*zz + cof[9];

				factor_1[0] = factor_1[0] + (double)f1*f1;
				factor_1[1] = factor_1[1] + (double)2*f1*f2;
				factor_1[2] = factor_1[2] + (double)f2*f2;


			}


			return 0;
}

/*****************************************************************************
* FUNCTION: function_value_1D
* DESCRIPTION: compute the function value
* PARAMETERS:
*    a: parameters value
*     
* SIDE EFFECTS: None
* ENTRY CONDITIONS: function get_factor should be called first 
* RETURN VALUE:
*      function value
*         
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 11/26/00 by Ying Zhuge
*
*****************************************************************************/
double function_value_1D(double x)
{

	double ret;

	ret = factor_1[0] - x*factor_1[1] + x*x*factor_1[2];

	return ret;
}









