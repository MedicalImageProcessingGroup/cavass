/*
  Copyright 1993-2009 Medical Image Processing Group
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

#include <stdlib.h>
#include <stdio.h>
//#include <unistd.h>
#include <errno.h>
#include <mpi.h>
#include <math.h>

//#include "3dv.h"
//#include "ElapsedTime.h"

#include <Viewnix.h>
#include  "cv3dv.h"
#include "p3dFuzzyConn.h"
/****************INCLUDE FOR FAST TRACKING******************/
#include "chash.h"
#include "hheap.h"

static int       ntasks;
static int       myrank;

static ViewnixHeader vh_in;
static int       object_mean[MAX_OBJECTS],object_function[MAX_OBJECTS],object_function_mode[MAX_OBJECTS],homogeneity_function;
static int       space_function,scale_function,max_iteration,max_scale=8,selected_object,cutoff_threshold=1;
static int       object_affn_flag=0,iter_relative_flag=0,affinity_flag=0,scale_flag=0,union_affinity_flag=0,iteration_flag=0;
static double    object_sigma[MAX_OBJECTS],homogeneity_sigma,tolerance;
static char      *object_points_file[MAX_OBJECTS], *maskfile,*scale_file,*x_affinity_file,*y_affinity_file,*z_affinity_file;
static int       track_algorithm=2; /* 0=hheap; 1=chash ; 2=chash2 */
int              nSeeds;
int              SeedsData[1000][3];

static Object_str object_info;
static Chunk_str  chunk_info;
static Scale_str  scale_info;

double anisotropy_slice, anisotropy_row, anisotropy_col,tt1;

typedef struct
{
	short x, y, z;
}
Voxel;

Voxel nbor[6] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },  { -1, 0, 0 }, { 0, -1, 0 }, { 0, 0, -1 } };

static void * H;
typedef  unsigned short   OutCellType;

VoxelWithValue            seeds[MAX_PROCESSOR][MAX_SEEDS],slave_seeds[MAX_SEEDS];
int                       num_seeds[MAX_PROCESSOR],slave_numseeds;

OutCellType               *left_overlaps[MAX_PROCESSOR],*right_overlaps[MAX_PROCESSOR];

static char               *input_filename, *output_filename;
static unsigned char      *data_8;
static unsigned short     *data_16;

static unsigned char      *chunk_data8, *flag_data;
static unsigned short     *chunk_data16,*out_data;  /* input and out data for chunk image */


unsigned char             *feature_scale;   /* scale data */
unsigned short            *x_affinity, *y_affinity, *z_affinity;
float                     *pt_material;

int *sphere_no_points;
short (**sphere_points)[3];

float              *scale_map;
float              *homogeneity_map;
int                pslice,prow,pcol,pobject,num_of_bits, slice_size,volume_size;

MPI_Datatype              chunk_strtype;
MPI_Datatype              object_strtype;
MPI_Datatype              scale_strtype;

void parse_command_line(int argc, char *argv[]);

#define Push_xyz push_xyz_chash2
#define Repush_xyz(x, y, z, w, ow) repush_xyz_chash2((x), (y), (z), (w), (ow))
#define Pop_xyz pop_xyz_chash2

int main(int argc, char* argv[])
{

	/* set up 3 blocks */
	int                  blockcounts[4]={7,3,2,3};
	int                  i;
	MPI_Datatype         types[4];
	MPI_Aint             displs[4];


	/* Initialize MPI. */
	MPI_Init(&argc, &argv);

	/* find out my identity in the default communicator */
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	/* set up user-defined MPI data structures */

	/* initialize types and displs with addresses of items */
	MPI_Address(&chunk_info.cols, displs);
	MPI_Address(&chunk_info.pxsize_x, displs+1);
	MPI_Address(&chunk_info.num_of_bits, displs+2);
	MPI_Address(&chunk_info.affinity_flag,displs+3);

	types[0] = MPI_INT;
	types[1] = MPI_FLOAT;
	types[2] = MPI_INT;
	types[3] = MPI_CHAR;

	for(i = 3;i>=0;i--)
		displs[i] -= displs[0];

	MPI_Type_struct(4,blockcounts,displs,types,&chunk_strtype);
	MPI_Type_commit(&chunk_strtype);

	/* for object_strtype */
	blockcounts[0] = MAX_OBJECTS + 1;
	blockcounts[1] = MAX_OBJECTS;
	blockcounts[2] = 2*MAX_OBJECTS;

	MPI_Address(&object_info.num_objects, &displs[0]);
	MPI_Address(&object_info.sigma[0], &displs[1]);
	MPI_Address(&object_info.function[0], &displs[2]);

	types[0] = MPI_INT;
	types[1] = MPI_DOUBLE;
	types[2] = MPI_INT;

	for(i = 2;i>=0;i--)
		displs[i] -= displs[0];

	MPI_Type_struct(3,blockcounts,displs,types,&object_strtype);
	MPI_Type_commit(&object_strtype);

	/* for scale_strtype */
	blockcounts[0] = 1;
	blockcounts[1] = 1;
	blockcounts[2] = 1;

	MPI_Address(&scale_info.max_scale, &displs[0]);
	MPI_Address(&scale_info.tolerance, &displs[1]);
	MPI_Address(&scale_info.sigma, &displs[2]);

	types[0] = MPI_INT;
	types[1] = MPI_FLOAT;
	types[2] = MPI_DOUBLE;

	for(i = 2;i>=0;i--)
		displs[i] -= displs[0];

	MPI_Type_struct(3,blockcounts,displs,types,&scale_strtype);
	MPI_Type_commit(&scale_strtype);
	/* end of MPI data structures */

	if(myrank == 0)
	{
		parse_command_line(argc, argv);
		master();
	}
	else 
		slave();

	/* shut down MPI */
	MPI_Finalize();

	return 0;
}

void master()
{
	int i, j, k, x, y,z,tti1,tti2,iteration;
	int overlap_slice, begin_slice,end_slice,slice_size,chunk_size,chunk_slices,error_code;
	//	double anisotropy_slice, anisotropy_row, anisotropy_col,tt1;
	char group[6],element[6];
	FILE *fp_in, *fp_out;
	int rank;
	MPI_Status	status;			/* returned from MPI */
	int queue_flag;
	unsigned char *scale_image;
	unsigned short *affn_image;

	int  st_length = 0;

	time_t t1,t2,t3,t4;

	time(&t2);

	MPI_Comm_size(MPI_COMM_WORLD, &ntasks);	

	/* set parameters for other processors */
	object_info.num_objects = pobject;
	for(i = 0;i<pobject;i++)
	{
		object_info.mean_intensity[i] = object_mean[i];
		object_info.sigma[i] = object_sigma[i];
		object_info.function[i] = object_function[i];
		object_info.function_mode[i] = object_function_mode[i];
	}

	for(rank = 0;rank<ntasks;++rank)
	{
		if(rank !=myrank)
		{
			MPI_Send(&object_info, 1, object_strtype,rank, OBJINFO_WORKTAG, MPI_COMM_WORLD);
		}
	}

	scale_info.max_scale = max_scale;
	scale_info.tolerance = tolerance;
	scale_info.sigma = homogeneity_sigma;

	for(rank = 0;rank<ntasks;++rank)
	{
		if(rank !=myrank)
		{
			MPI_Send(&scale_info, 1, scale_strtype,rank, SCALEINFO_WORKTAG, MPI_COMM_WORLD);
		}
	}

	pcol = vh_in.scn.xysize[0];
	prow = vh_in.scn.xysize[1];
	chunk_info.cols = vh_in.scn.xysize[0];
	chunk_info.rows = vh_in.scn.xysize[1];
	slice_size = chunk_info.cols * chunk_info.rows;

	chunk_info.num_of_bits = vh_in.scn.num_of_bits;
	chunk_info.largest_intensity = vh_in.scn.largest_density_value[0];
	chunk_info.pxsize_x = vh_in.scn.xypixsz[0];
	chunk_info.pxsize_y = vh_in.scn.xypixsz[1];

	chunk_info.scale_flag = scale_flag;
	chunk_info.affinity_flag = affinity_flag;
	chunk_info.object_affn_flag = object_affn_flag;

	if (vh_in.scn.num_of_subscenes[0] > 1)
		chunk_info.thickness = fabs(vh_in.scn.loc_of_subscenes[1] - vh_in.scn.loc_of_subscenes[0]);
	else
		chunk_info.thickness = 0;

	if (ntasks>2)
		chunk_slices = (vh_in.scn.num_of_subscenes[0] + ntasks - 2)/(ntasks-1); /* master process does't do computation work */
	else if(ntasks ==2)
		chunk_slices = vh_in.scn.num_of_subscenes[0];
	else if(ntasks <2 )
	{
		printf("Needs at least two workstations!...\n");
		MPI_Abort(MPI_COMM_WORLD,errno);
	}

	chunk_info.slices = chunk_slices;

	anisotropy_col = vh_in.scn.xypixsz[0];
	anisotropy_row = vh_in.scn.xypixsz[1];
	anisotropy_slice = chunk_info.thickness ;

	tt1 = anisotropy_col;
	if (tt1 > anisotropy_row)
		tt1 = anisotropy_row;
	if ( vh_in.scn.num_of_subscenes[0] > 1 && tt1 > anisotropy_slice)
		tt1 = anisotropy_slice;
	anisotropy_col = anisotropy_col / tt1;
	anisotropy_row = anisotropy_row / tt1;
	anisotropy_slice = anisotropy_slice / tt1;

	overlap_slice = max_scale * tt1/anisotropy_slice + 0.5;

	//chunk_info.left_overlap =  overlap_slice;
	//chunk_info.right_overlap = overlap_slice;

	chunk_info.left_overlap = 0;
	chunk_info.right_overlap = 2;

	/* read a 3D input image */

	time(&t3);
	fp_in = fopen(input_filename,"r+b");
	if(fp_in == NULL)
	{
		printf("Can not open input file!\n");
		exit(-1);
	}

	if(vh_in.scn.num_of_bits == 8)
	{
		data_8 = (unsigned char *)malloc((chunk_info.slices + chunk_info.right_overlap)*slice_size*sizeof(unsigned char));
		if(data_8 == 0)	
			MPI_Abort(MPI_COMM_WORLD,errno);
	}
	else if(vh_in.scn.num_of_bits == 16)
	{
		data_16 = (unsigned short *)malloc((chunk_info.slices+ chunk_info.right_overlap)*slice_size*sizeof(unsigned short));
		if(data_16 == 0)
			MPI_Abort(MPI_COMM_WORLD,errno);
	}

	/* distribute the chunk information chunk_data to all other processors */
	begin_slice = end_slice = 0;
	for (i = 0;i<ntasks;i++)
	{
		if (i != myrank)
		{
			chunk_info.begin_slice = begin_slice;

			end_slice = begin_slice + chunk_info.slices - 1;
			chunk_info.end_slice = end_slice;

			if(end_slice + chunk_info.right_overlap >= vh_in.scn.num_of_subscenes[0])
			{
				chunk_info.end_slice = vh_in.scn.num_of_subscenes[0]-1;
				chunk_info.slices = chunk_info.end_slice - chunk_info.begin_slice + 1;
				chunk_info.right_overlap = 0;
			}

			begin_slice = end_slice + 1;
			MPI_Send(&chunk_info, 1, chunk_strtype,i, CHUNKINFO_WORKTAG, MPI_COMM_WORLD);

			if(vh_in.scn.num_of_bits == 8)
			{
				VSeekData(fp_in, 0);
				VSeekData(fp_in, chunk_info.begin_slice*slice_size*sizeof(char));
				chunk_size = (chunk_info.slices+chunk_info.right_overlap)*slice_size;
				error_code = VReadData((char*)data_8, 1, chunk_size, fp_in, &j);
				MPI_Send(data_8,chunk_size,MPI_CHAR,i,CHUNKDATA_WORKTAG,MPI_COMM_WORLD);
			}
			else if(vh_in.scn.num_of_bits == 16)
			{
				VSeekData(fp_in, 0);
				VSeekData(fp_in, chunk_info.begin_slice*slice_size*sizeof(short));
				chunk_size = (chunk_info.slices+chunk_info.right_overlap)*slice_size;
				error_code = VReadData((char*)data_16, 2, chunk_size, fp_in, &j);
				MPI_Send(data_16,chunk_size,MPI_UNSIGNED_SHORT,i,CHUNKDATA_WORKTAG,MPI_COMM_WORLD);
				printf( "master sending ... chunk_size = %d \n", chunk_size );
			}
		}
	}
	fclose(fp_in);

	time(&t4);
	printf("Reading and transfering data:%f seconds\n",difftime(t4,t3)); 


	if(vh_in.scn.num_of_bits == 8)
		free(data_8);
	else if(vh_in.scn.num_of_bits == 16)
		free(data_16);

	scale_image = (unsigned char *)malloc(chunk_slices*slice_size*sizeof(unsigned char));
	affn_image = (unsigned short *)malloc(chunk_slices*slice_size*sizeof(unsigned short));
	if((scale_image == 0)||(affn_image ==0))
		MPI_Abort(MPI_COMM_WORLD,errno);

	/* collect and write scale chunk data from slaves */

	printf( "scale_flag = %d, affinity_flag = %d \n", scale_flag, affinity_flag );
	if(scale_flag)
	{    
		printf( "coming here, scale_flag \n" );

		time(&t3);

		vh_in.scn.num_of_bits = 8;
		vh_in.scn.bit_fields[1] = vh_in.scn.num_of_bits - 1;

		vh_in.scn.smallest_density_value[0] = 0;  
		vh_in.scn.largest_density_value[0] = scale_info.max_scale;

		fp_out = fopen(scale_file,"w+b");
		error_code = VWriteHeader(fp_out, &vh_in, group, element);

		for( i = 0;i<ntasks;++i)
		{
			if(i !=myrank)
			{
				MPI_Recv(&chunk_info, 1, chunk_strtype, i, SCALETAG,MPI_COMM_WORLD,&status);
				printf("Collecting scale chunk scale from node %d...\n",i);
				chunk_size = chunk_info.slices*slice_size;
				MPI_Recv(scale_image,chunk_size,MPI_CHAR,i,SCALETAG,MPI_COMM_WORLD,&status);
				VSeekData(fp_out,0);
				VSeekData(fp_out, chunk_info.begin_slice*slice_size);
				VWriteData((char*)scale_image, vh_in.scn.num_of_bits/8,chunk_size, fp_out,&j);
			}
		}
		fclose(fp_out);

		time(&t4);
		printf("scale computation:%f seconds\n",difftime(t4,t3)); 
	}

	///* collect and write affinity data */
	if(affinity_flag)
	{
		printf( "coming here, affinity_flag \n" );

		time(&t3);

		vh_in.scn.num_of_bits = 16;
		vh_in.scn.bit_fields[1] = vh_in.scn.num_of_bits - 1;

		vh_in.scn.smallest_density_value[0] = 0;  
		vh_in.scn.largest_density_value[0] = CONN;

		fp_out = fopen("affn_image.IM0","w+b");
		error_code = VWriteHeader(fp_out, &vh_in, group, element);

		for( i = 0;i<ntasks;++i)
		{
			if(i !=myrank)
			{
				MPI_Recv(&chunk_info, 1, chunk_strtype, i, AFFNTAG,MPI_COMM_WORLD,&status);
				printf("Collecting chunk affinity from node %d...\n",i);
				chunk_size = chunk_info.slices*slice_size;
				MPI_Recv(affn_image,chunk_size,MPI_UNSIGNED_SHORT,i,AFFNTAG,MPI_COMM_WORLD,&status);
				VSeekData(fp_out,0);
				VSeekData(fp_out, chunk_info.begin_slice*slice_size*sizeof(short));
				VWriteData((char*)affn_image, vh_in.scn.num_of_bits/8,chunk_size, fp_out,&j);
			}
		}
		fclose(fp_out);

		time(&t4);
		printf("affinity computation :%f seconds\n",difftime(t4,t3)); 
	}

	/* distribute seeds and track queue status for fast tracking*/
	for(i = 0;i<ntasks-1;i++)
	{
		memset(seeds[i],0,1000*sizeof(VoxelWithValue));
		num_seeds[i] = 0;
	}

	/* malloc the overlapping slices */
	for(i = 0;i<ntasks-1;i++)
	{
		left_overlaps[i] = (unsigned short *) malloc(slice_size*sizeof(short));
		right_overlaps[i] = (unsigned short *) malloc(slice_size*sizeof(short));
		if((left_overlaps[i] == 0)||(right_overlaps[i] ==0))
			MPI_Abort(MPI_COMM_WORLD,errno);
		memset(left_overlaps[i],0,slice_size*sizeof(short));
		memset(right_overlaps[i],0,slice_size*sizeof(short));
	}

//	fp_in = fopen(object_points_file[selected_object], "r");
//	while (fscanf(fp_in, "%d %d %d ", &x, &y, &z) == 3)
	for( i=0; i<nSeeds; i++ )
	{
		x = SeedsData[i][0];	y = SeedsData[i][1];	z = SeedsData[i][2];
		printf("seed: (%d, %d, %d)\n", x,y,z);

		i = z/chunk_slices;
		seeds[i][num_seeds[i]].x = x;
		seeds[i][num_seeds[i]].y = y;
		seeds[i][num_seeds[i]].z = z-i*chunk_slices;
		seeds[i][num_seeds[i]].val = CONN;
		num_seeds[i]++;
	}
//	fclose(fp_in);

	time(&t3);  /* starting fast tracking */

	queue_flag = 1;
	iteration = 0;
	while(queue_flag)
	{
		printf(" fast tracking %d...\n",iteration++);
		/* distribute seeds to processors */
		for(i = 0;i<ntasks;i++)
		{
			if(i<myrank)
			{
				printf(" num_seeds[%d] =  %d \n",i, num_seeds[i] );

				MPI_Send(&num_seeds[i], 1, MPI_INT,i, NUMSEEDS_WORKTAG, MPI_COMM_WORLD);
				if(num_seeds[i])
					MPI_Send(seeds[i],num_seeds[i]*4,MPI_SHORT,i, SEEDS_WORKTAG, MPI_COMM_WORLD);
				/* may have problem here because of the incompatible data type */
			}
			else if(i>myrank)
			{
				printf(" num_seeds[%d] =  %d \n",i-1, num_seeds[i-1] );

				MPI_Send(&num_seeds[i-1], 1, MPI_INT,i, NUMSEEDS_WORKTAG, MPI_COMM_WORLD);
				if(num_seeds[i-1])
					MPI_Send(seeds[i-1],num_seeds[i-1]*4,MPI_SHORT,i, SEEDS_WORKTAG, MPI_COMM_WORLD);
			}
		}

		printf("*** Pass by here, master, %i\n", i );    		

		/* collect the overlapping slices */
		for (i = 0;i<ntasks;i++)
		{
			if(i < myrank)
			{
				memset(left_overlaps[i],0,slice_size*sizeof(short));
				memset(right_overlaps[i],0,slice_size*sizeof(short));

				//wait for a message (first, get new message length)				
				MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
				printf("master, %d  \n", i);
				st_length = 0;
				MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );

				printf("recv length, %d \n", st_length);
				if (st_length <= 0)    continue;

				if (status.MPI_TAG == CONNTAG ) 
				{
					MPI_Recv(&chunk_info, 1, chunk_strtype, i, CONNTAG,MPI_COMM_WORLD,&status);
					printf("Collecting overlaping slice data from node %d...\n",i);
					//if(chunk_info.left_overlap>0)
					MPI_Recv(left_overlaps[i],slice_size,MPI_UNSIGNED_SHORT,i,CONNTAG,MPI_COMM_WORLD,&status);
					if(chunk_info.right_overlap>0)
						MPI_Recv(right_overlaps[i],slice_size,MPI_UNSIGNED_SHORT,i,CONNTAG,MPI_COMM_WORLD,&status);
				}

			}
			else if(i>myrank)
			{
				memset(left_overlaps[i-1],0,slice_size*sizeof(short));
				memset(right_overlaps[i-1],0,slice_size*sizeof(short));

				printf("master, %d  \n", i);
				MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );

				st_length = 0;
				MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );

				printf("recv length, %d \n", st_length);
				if (st_length <= 0)    continue;

				if (status.MPI_TAG == CONNTAG ) 
				{				
					MPI_Recv(&chunk_info, 1, chunk_strtype, i, CONNTAG,MPI_COMM_WORLD,&status);
					printf("Collecting overlaping slice data from node %d...\n",i);
					//if(chunk_info.left_overlap>0)
					MPI_Recv(left_overlaps[i-1],slice_size,MPI_UNSIGNED_SHORT,i,CONNTAG,MPI_COMM_WORLD,&status);
					if(chunk_info.right_overlap>0)
						MPI_Recv(right_overlaps[i-1],slice_size,MPI_UNSIGNED_SHORT,i,CONNTAG,MPI_COMM_WORLD,&status);
				}
			}
		}

		printf("*** Pass by here, master,after Recv \n" );    


		queue_flag = 0;

		for(i = 0;i<ntasks-1;i++)
		{
			memset(seeds[i],0,MAX_SEEDS*sizeof(VoxelWithValue));
			num_seeds[i] = 0;
		}

		/* determine how to update the local queues */ 
		/** for left overlapping slice */
		for(i=1;i<ntasks-1;i++)
		{
			for(y=0;y<prow;y++)
				for(x = 0;x<pcol;x++)
				{
					tti1 = right_overlaps[i-1][y*pcol+x];
					tti2 = left_overlaps[i][y*pcol+x];

					if(tti1>tti2)
					{
						queue_flag = 1;
						seeds[i][num_seeds[i]].x = x;
						seeds[i][num_seeds[i]].y = y;
						seeds[i][num_seeds[i]].z = 0;
						seeds[i][num_seeds[i]].val = tti1;	  
						num_seeds[i]++;
					}
				}
		}

		/** for right overlapping slice */
		for(i=0;i<ntasks-2;i++)
		{
			for(y=0;y<prow;y++)
				for(x = 0;x<pcol;x++)
				{
					tti1 = left_overlaps[i+1][y*pcol+x];
					tti2 = right_overlaps[i][y*pcol+x];

					if(tti1>tti2)
					{
						queue_flag = 1;
						seeds[i][num_seeds[i]].x = x;
						seeds[i][num_seeds[i]].y = y;
						seeds[i][num_seeds[i]].z = chunk_slices;
						seeds[i][num_seeds[i]].val = tti1;			  
						num_seeds[i]++;
					}
				}
		}

	} /* end of while (queue_flag) */

	time(&t4);
	printf("tracking...:%f seconds\n",difftime(t4,t3)); 

	time(&t3);
	for(i = 0;i<ntasks-1;i++)
	{
		free(left_overlaps[i]);
		free(right_overlaps[i]);
	}

	/** We have obtained connectivity , so stop workers' tracking. */ 
	for(i = 0;i<ntasks;++i)
	{
		if (i != myrank) 
		{
			MPI_Send(0, 0, MPI_INT, i, STOPTAG, MPI_COMM_WORLD);
		}
	}
	/* collect connectivity value */

	fp_out = fopen(output_filename,"w+b");
	error_code = VWriteHeader(fp_out, &vh_in, group, element);

	for( i = 0;i<ntasks;++i)
	{
		if(i !=myrank)
		{
			MPI_Recv(&chunk_info, 1, chunk_strtype, i, CONNTAG,MPI_COMM_WORLD,&status);
			printf("Collecting chunk connectivity data from node %d...\n",i);
			chunk_size = chunk_info.slices*slice_size;
			MPI_Recv(affn_image,chunk_size,MPI_UNSIGNED_SHORT,i,CONNTAG,MPI_COMM_WORLD,&status);
			VSeekData(fp_out,0);
			VSeekData(fp_out, chunk_info.begin_slice*slice_size*sizeof(short));
			VWriteData((char *)affn_image, vh_in.scn.num_of_bits/8,chunk_size, fp_out,&j);
		}
	}
	fclose(fp_out);

	time(&t4);
	printf("Collecting and writing:%f seconds...\n",difftime(t4,t3)); 


	/** We have all the answers now, so kill the workers. */ 
	for(i = 0;i<ntasks;++i)
	{
		if (i != myrank) 
		{
			MPI_Send(0, 0, MPI_INT, i, DIETAG, MPI_COMM_WORLD);
		}
	}

	if(scale_image !=0)
		free(scale_image);
	if(affn_image !=0)
		free(affn_image);

	printf("master: done. \n");

	time(&t1);
	printf("Total computation last:%f seconds\n",difftime(t1,t2)); 
}




void parse_command_line(int argc, char *argv[])
{
	int i, args_parsed, object, error_code, necessary_parameters = 0;
	int flag_objects = 0, flag_homogeneity = 0, flag_space = 0;
	int flag_tolerance = 0, flag_affinity = 0, flag_iteration = 0;
	int found = 0, old_found = -1;
	char group[5], element[5];
	FILE *fp;

	int num_bits;

	args_parsed = 1;

	input_filename = argv[args_parsed++];
	output_filename = argv[args_parsed++];

	fp = fopen(input_filename, "rb");
	if (fp == NULL)
		fprintf(stderr,"File open error!\n");

	error_code = VReadHeader(fp, &vh_in, group, element);
	switch (error_code)
	{
	case 0:
	case 106:
	case 107:
		break;
	default:
		fprintf(stderr, "file = %s; group = %s; element = %s\n", input_filename, group,  element);
		fprintf(stderr,"IM0 header reading error!\n");
	}
	// num_of_bits = vh_in.scn.num_of_bits;
	fclose(fp);


	if (argc < 10)
		usage(args_parsed, argv);

	while (found > old_found)
	{
		old_found = found;
		if (argc > args_parsed && strcmp(argv[args_parsed], "-objects") == 0)
		{
			if (flag_objects != 0)
			{
				printf("Objects defined twice\n");
				usage(args_parsed, argv);
			}
			flag_objects = 1;
			found = found + 1;
			args_parsed++;
			necessary_parameters = necessary_parameters + 1;
			if (sscanf(argv[args_parsed++], "%d", &pobject) != 1)
				usage(args_parsed, argv);

			for (object = 0; object < pobject; object++)
			{
				if (sscanf(argv[args_parsed++], "%d", &object_mean[object]) != 1)
					usage(args_parsed, argv);
				if (sscanf(argv[args_parsed++], "%lf", &object_sigma[object]) != 1)
					usage(args_parsed, argv);
				if (sscanf(argv[args_parsed++], "%d", &object_function[object]) != 1)
					usage(args_parsed, argv);
				if (argc > args_parsed && strcmp(argv[args_parsed], "-mode") == 0) 
				{
					args_parsed++;
					if (sscanf(argv[args_parsed++], "%d", &object_function_mode[object]) != 1)
						usage(args_parsed, argv);
					if(object_function_mode[object] > 2) {
						printf("Valid modes: 0, 1, 2\n");
						usage(args_parsed, argv);
					}
				}
				else 
					object_function_mode[object] = 0;

				//object_points_file[object] = argv[args_parsed++];
			}
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-seeds") == 0)
		{			
			args_parsed++;
			necessary_parameters = necessary_parameters + 1;
			if (sscanf(argv[args_parsed++], "%d", &nSeeds) != 1)
				usage(args_parsed, argv);

			if( nSeeds > 1000 )
				printf("Too many seeds, need to be less than 1000. \n");

			for (i = 0; i < nSeeds; i++)
			{
				if (sscanf(argv[args_parsed++], "%d", &SeedsData[i][0] ) != 1)
					usage(args_parsed, argv);
				if (sscanf(argv[args_parsed++], "%d", &SeedsData[i][1] ) != 1)
					usage(args_parsed, argv);
				if (sscanf(argv[args_parsed++], "%d", &SeedsData[i][2] ) != 1)
					usage(args_parsed, argv);				
			}
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-homogeneity") == 0)
		{
			if (flag_homogeneity != 0)
			{
				printf("Homogeneity parameters defined twice\n");
				usage(args_parsed, argv);
			}
			flag_homogeneity = 1;
			found = found + 1;
			args_parsed++;
			necessary_parameters = necessary_parameters + 1;
			if (sscanf(argv[args_parsed++], "%lf", &homogeneity_sigma) != 1)
				usage(args_parsed, argv);
			if (sscanf(argv[args_parsed++], "%d", &homogeneity_function) != 1)
				usage(args_parsed, argv);
			scale_function = homogeneity_function;
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-space") == 0)
		{
			if (flag_space != 0)
			{
				printf("Weighting function defined twice\n");
				usage(args_parsed, argv);
			}
			flag_space = 1;
			found = found + 1;
			args_parsed++;
			necessary_parameters = necessary_parameters + 1;
			if (sscanf(argv[args_parsed++], "%d", &space_function) != 1)
				usage(args_parsed, argv);
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-tolerance") == 0)
		{
			if (flag_tolerance != 0)
			{
				printf("Tolerance value defined twice\n");
				usage(args_parsed, argv);
			}
			flag_tolerance = 1;
			found = found + 1;
			args_parsed++;
			necessary_parameters = necessary_parameters + 1;
			if (sscanf(argv[args_parsed++], "%lf", &tolerance) != 1)
				usage(args_parsed, argv);
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-max_iteration") == 0)
		{
			if (flag_iteration != 0)
			{
				printf("Iteration number defined twice\n");
				usage(args_parsed, argv);
			}
			flag_iteration = 1;
			found = found + 1;
			iteration_flag = 1;
			args_parsed++;
			if (sscanf(argv[args_parsed++], "%d", &max_iteration) != 1)
				usage(args_parsed, argv);
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-cutoff_threshold") == 0)
		{
			args_parsed++;
			if (sscanf(argv[args_parsed++], "%d", &cutoff_threshold) != 1)
				usage(args_parsed, argv);
			if(cutoff_threshold < 0) {
				printf("Cutoff threshold must be +ve\n");
				usage(args_parsed, argv);
			}
			found = found + 1;
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-max_scale") == 0)
		{
			args_parsed++;
			if (sscanf(argv[args_parsed++], "%d", &max_scale) != 1)
				usage(args_parsed, argv);
			if(max_scale < 1) {
				printf("Maximum scale must be greater than 1.\n");
				usage(args_parsed, argv);
			}
			found = found + 1;
		}

		if (argc>args_parsed && strcmp(argv[args_parsed], "-track_algorithm")==0)
		{
			args_parsed++;
			if (argc<=args_parsed ||
				sscanf(argv[args_parsed], "%d", &track_algorithm)!=1)
				usage(args_parsed, argv);
			args_parsed++;
		}
		if (argc > args_parsed && strcmp(argv[args_parsed], "-selected_object") == 0)
		{
			args_parsed++;
			if (sscanf(argv[args_parsed++], "%d", &selected_object) != 1)
				usage(args_parsed, argv);
			selected_object = selected_object - 1;
			found = found + 1;
		}
		if (argc > args_parsed && strcmp(argv[args_parsed], "-scale_file") == 0)
		{
			scale_flag = 1;
			args_parsed++;
			scale_file = argv[args_parsed++];
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-affinity_file") == 0)
		{
			if (flag_affinity != 0)
			{
				printf("Affinity files defined twice\n");
				usage(args_parsed, argv);
			}
			flag_affinity = 1;
			affinity_flag = 1;
			found = found + 1;
			args_parsed++;
			if (vh_in.scn.num_of_subscenes[0] == 1)
			{
				if (argc < args_parsed + 2)
					usage(args_parsed, argv);
				x_affinity_file = argv[args_parsed++];
				y_affinity_file = argv[args_parsed++];
			}
			else
			{
				if (argc < args_parsed + 3)
					usage(args_parsed, argv);
				x_affinity_file = argv[args_parsed++];
				y_affinity_file = argv[args_parsed++];
				z_affinity_file = argv[args_parsed++];
			}
		}
		if (argc > args_parsed && strcmp(argv[args_parsed], "-object_affn_flag") == 0)
		{
			args_parsed++;
			sscanf(argv[args_parsed++], "%f", &object_affn_flag);
			found = found + 1;
		}
		if (argc > args_parsed && strcmp(argv[args_parsed], "-mask") == 0)
		{
			args_parsed++;
			maskfile = argv[args_parsed++];
			found = found + 1;
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-union_affinity_flag") == 0)
		{
			args_parsed++;
			sscanf(argv[args_parsed++], "%d", &union_affinity_flag);
			found = found + 1;
		}

		if (argc > args_parsed && strcmp(argv[args_parsed], "-iter_relative_flag") == 0)
		{
			args_parsed++;
			sscanf(argv[args_parsed++], "%d", &iter_relative_flag);
			found = found + 1;
		}


	}

}


/*****************************************************************************
* FUNCTION: usage
* DESCRIPTION: Displays the usage message on stderr and exits with code 1.
* PARAMETERS: None
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variable argv0 should be set to the program name.
* RETURN VALUE: None
* EXIT CONDITIONS: Exits with code 1.
* HISTORY:
*  Created: 4/2/96 by Dewey Odhner
*  Modified: 4/17/96 output of original values allowed by Dewey Odhner
*  Modified: 5/21/96 histogram-type affinity allowed by Dewey Odhner
*  Modified: 7/25/96 covariance affinity type allowed by Dewey Odhner
*
*****************************************************************************/
void usage(int argc, char *argv[] )
{
	int i;
	printf("Parsed upto: \n");
	for(i = 0; i< argc; i++) 
		printf("%s ",argv[i]);
	printf("\n------------------------------------\n\n"); 
	fprintf(stderr, "Usage: %s <in> <out>", argv[0]);
	fprintf(stderr, " -objects <nob> <moi> <soi> <foi> [-mode <mdi>]");
	fprintf(stderr, " -seeds <nSeeds> <x,y,z...>");
	fprintf(stderr, " -homogeneity <sh> <fh>");
	fprintf(stderr, " -space <fs> -tolerance <tv>");
	fprintf(stderr, " [-affinity <ax> <ay> <az> (not needed for 2D)]");
	fprintf(stderr, " [-cutoff_threshold <cth>]");
	fprintf(stderr, " [-max_scale <msc>]");
	fprintf(stderr, " [-iteration <itn>]");
	fprintf(stderr, " [-track_algorithm <ta>] ");
	fprintf(stderr, " [-selected_object <so>]");
	fprintf(stderr, "     nob: Numbr of Objects\n");
	fprintf(stderr, "     moi, soi, foi, mdi: mean, sigma, function, mode (optional) \n");
	fprintf(stderr, "     nSeeds,     the number of seeds for object 1\n");
	fprintf(stderr, "     sh, fh: sigman and function for homogeneity\n");
	fprintf(stderr, "     fs: weighting function for scale-based neighborhood\n");
	fprintf(stderr, "     tv: tolerance value for scale computation\n");
	fprintf(stderr, "     ax: x_affinity filename (optional)\n");
	fprintf(stderr, "     ay: y_affinity filename (optional)\n");
	fprintf(stderr, "     az: z_affinity filename (optional)\n");
	fprintf(stderr, "     cth: cutoff threshold (optional)\n");
	fprintf(stderr, "     msc: maximum scale (optional)\n");
	fprintf(stderr, "     ta: tracking algorithm: 0=hheap, 1=chash, 2=chash2\n");
	fprintf(stderr, "     itn: maximum iteration number (optional)\n");
	fprintf(stderr, "     so: selected object number (1 for the first) (optional)\n");
	fprintf(stderr, "     Functions: 0=Gaussian; 1=Linear; 2=Box\n");
	fprintf(stderr, "     Function modes: 0=regular; 1=step-down; 2=step-up\n");
	exit(1);
}


void slave()
{
	int i;
	int st_length;
	MPI_Status	status;			/* returned from MPI */

	for(;;)
	{
		//wait for a message (first, get new message length)				
		MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );

		if(status.MPI_TAG == DIETAG)
		{			
			///*free affinity and return to master program */
			free(feature_scale);
			free(x_affinity);
			free(y_affinity);
			free(z_affinity);
			free(out_data);
			free(flag_data);
			return ;
		}		
		else if(status.MPI_TAG == OBJINFO_WORKTAG)
		{
			MPI_Recv(&object_info, 1,object_strtype,MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);  
			st_length = 0;
			MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
			printf("slave, after recv 1, %d \n", st_length);

		}
		else if(status.MPI_TAG == SCALEINFO_WORKTAG)
		{
			MPI_Recv(&scale_info, 1,scale_strtype,MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			st_length = 0;
			MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
			printf("slave, after recv 2, %d \n", st_length);

		}
		else if(status.MPI_TAG == CHUNKINFO_WORKTAG)
		{
			MPI_Recv(&chunk_info, 1,chunk_strtype,MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			st_length = 0;
			MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
			printf("slave, after recv 3, %d \n", st_length);


			prow = chunk_info.rows;
			pcol = chunk_info.cols;
			pslice = chunk_info.slices + chunk_info.right_overlap;

			slice_size = chunk_info.rows * chunk_info.cols;
			volume_size = slice_size * (chunk_info.slices + chunk_info.right_overlap);

			pobject = object_info.num_objects;

			printf("chunk_info.num_of_bits = %d \n", chunk_info.num_of_bits);

		}
		else if(status.MPI_TAG == CHUNKDATA_WORKTAG)
		{

			if (chunk_info.num_of_bits == 8)
			{
				chunk_data8 = (unsigned char*) malloc(volume_size*sizeof(char));
				if(chunk_data8 == 0)
					MPI_Abort(MPI_COMM_WORLD,errno);
				MPI_Recv(chunk_data8, volume_size, MPI_CHAR, MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);

				st_length = 0;
				MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
				printf("slave, chunk, after recv 5, %d \n", st_length);

			}
			if (chunk_info.num_of_bits == 16)
			{
				chunk_data16 = (unsigned short*) malloc(volume_size*sizeof(short));
				if(chunk_data16 == 0)
					MPI_Abort(MPI_COMM_WORLD,errno);
				MPI_Recv(chunk_data16, volume_size, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);

				printf("slave receiving ... chunck_size = %d \n", volume_size);

				st_length = 0;
				MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
				printf("slave, chunk, after recv 5, %d \n", st_length);

			}

			feature_scale = (unsigned char *) malloc(volume_size*sizeof(char));
			x_affinity = (unsigned short *) malloc(volume_size*sizeof(short));
			y_affinity = (unsigned short *) malloc(volume_size*sizeof(short));
			z_affinity = (unsigned short *) malloc(volume_size*sizeof(short));
			pt_material = (float *)malloc(volume_size*sizeof(float));

			for (i = 0;i<volume_size;i++)
			{
				feature_scale[i] = 0;
				x_affinity[i] = 0;
				y_affinity[i] = 0;
				z_affinity[i] = 0;
				pt_material[i] = 0;
			}

			if((feature_scale==0)||(x_affinity==0)||(y_affinity==0)||(z_affinity==0))
				MPI_Abort(MPI_COMM_WORLD,errno);

			printf("slave, come to work, scale_info.sigma = %f, chunk_info.largest_intensity = %d \n", scale_info.sigma, chunk_info.largest_intensity);
			/* once all parameters are ready, slave can start its local computation*/

			/* compute look up table for scale computation */
			scale_map = (float *)malloc((chunk_info.largest_intensity+1)*sizeof(float));
			homogeneity_map = (float *) malloc((chunk_info.largest_intensity+1)*sizeof(float));

			if((scale_map == 0)||(homogeneity_map ==0))
				MPI_Abort(MPI_COMM_WORLD,errno);

			for (i = 0; i <= chunk_info.largest_intensity; i++)
			{
				scale_map[i] = exp(-0.5*((float)(i*i))/(4*scale_info.sigma*scale_info.sigma)); 
				homogeneity_map[i] = exp(-0.5*((float)(i*i))/(scale_info.sigma*scale_info.sigma)); 
			}

			printf("scale_map[4] = %f, scale_map[50]=%f, scale_map[600] = %f, %f\n",scale_map[4], scale_map[50], scale_map[600], scale_map[2700]);
			///* look up table for point number and point position for each scale */
			scale_prepare();
			///* compute the scale value for each voxel in the chunk image */	
			compute_feature_scale();

			printf("scale_flag = %d, object_affn_flag = %d,  affinity_flag = %d", chunk_info.scale_flag, chunk_info.object_affn_flag, chunk_info.affinity_flag);


			/* tell master chunk scale computation is done */
			if(chunk_info.scale_flag)
			{
				MPI_Send(&chunk_info, 1, chunk_strtype, status.MPI_SOURCE,SCALETAG,MPI_COMM_WORLD);
				MPI_Send(feature_scale, chunk_info.slices*slice_size, MPI_CHAR,status.MPI_SOURCE,SCALETAG,MPI_COMM_WORLD);
			}
			/* compute scale-based affinity */
			if (chunk_info.object_affn_flag)
			{
				for (i = 0;i<volume_size;i++)
				{
					x_affinity[i] = 1;
					y_affinity[i] = 1;
					z_affinity[i] = 1;
				}

				compute_material();
				compute_affinity_feature_only();    
			}
			else
			{
				compute_homogeneity();   
				compute_material();
				compute_affinity();
			}

			/* tell master chunk affinity computation is done */
			if(chunk_info.affinity_flag)
			{
				MPI_Send(&chunk_info, 1, chunk_strtype, status.MPI_SOURCE,AFFNTAG,MPI_COMM_WORLD);
				MPI_Send(z_affinity, chunk_info.slices*slice_size, MPI_UNSIGNED_SHORT,status.MPI_SOURCE,AFFNTAG,MPI_COMM_WORLD);
			}

			/* free memory allocated in slave program when computation is done  */
			if(chunk_info.num_of_bits == 8)
				free(chunk_data8);
			else if(chunk_info.num_of_bits == 16)
				free(chunk_data16);
			if(pt_material)
				free(pt_material);

			free(scale_map);
			free(homogeneity_map);  

			printf("*** Pass by here, slave 3, %d\n", chunk_info.slices );    			

		}
		else if(status.MPI_TAG == NUMSEEDS_WORKTAG)
		{			
				/*}		
				else if(status.MPI_TAG == SEEDS_WORKTAG)
				{*/
		/* receive seeds from master for fuzzy tracking */
			out_data = (unsigned short *) malloc(volume_size*sizeof(short));
			flag_data = (unsigned char *) malloc(volume_size*sizeof(char));
			if((out_data==0)||(flag_data==0))
				MPI_Abort(MPI_COMM_WORLD,errno);
			memset(out_data,0,volume_size*sizeof(short));	

			while( 1 )
			{
			//	MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );				

				MPI_Recv(&slave_numseeds, 1, MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);

				if(status.MPI_TAG == STOPTAG)
				{
					printf("*** iteration end \n" );    

					/* send back the chunk information to master */
					MPI_Send(&chunk_info, 1, chunk_strtype, status.MPI_SOURCE,CONNTAG,MPI_COMM_WORLD);
					/* send back the connectivity chunk to master */
					MPI_Send(out_data, chunk_info.slices*slice_size, MPI_UNSIGNED_SHORT,status.MPI_SOURCE,CONNTAG,MPI_COMM_WORLD);			

					break;
				}

				if(slave_numseeds)
				{
					MPI_Recv(&slave_seeds,slave_numseeds*4,MPI_SHORT,MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);
					fuzzy_track();
				}

				/* send back the overlapping slices to master */
				MPI_Send(&chunk_info, 1, chunk_strtype, status.MPI_SOURCE,CONNTAG,MPI_COMM_WORLD);
				//if(chunk_info.left_overlap>0)
				MPI_Send(out_data,slice_size,MPI_UNSIGNED_SHORT,status.MPI_SOURCE,CONNTAG,MPI_COMM_WORLD);
				if(chunk_info.right_overlap>0)
					MPI_Send(out_data+chunk_info.slices*slice_size,slice_size,MPI_UNSIGNED_SHORT,status.MPI_SOURCE,CONNTAG,MPI_COMM_WORLD);

				/* reset the seeds and num_seeds of slave */
				memset(slave_seeds,0,slave_numseeds*sizeof(VoxelWithValue));
				slave_numseeds = 0;
			}
		}
	}	
}

void scale_prepare()
{

	int    i,j,k,l;          /* loop counter */
	int    ***ppptti1;
	int    tti1,tti2;
	double tt1;
	
	tti1 = 2 * (scale_info.max_scale + 5);
	ppptti1 = (int ***) malloc(tti1 * sizeof(int **));   // very bad, pointer malloc
	if (ppptti1 == 0)  
		MPI_Abort(MPI_COMM_WORLD,errno);

	ppptti1[0] = (int **) malloc(tti1*tti1 * sizeof(int*));
	if (ppptti1[0] == 0)  
		MPI_Abort(MPI_COMM_WORLD,errno); 

	ppptti1[0][0] = (int *) malloc(tti1*tti1*tti1*sizeof(int));
	if (ppptti1[0][0] == 0)  
		MPI_Abort(MPI_COMM_WORLD,errno); 

	printf("tti1 = %d, scale_info.max_scale = %d \n", tti1, scale_info.max_scale);

    for (i = 0; i < tti1; i++)                  // add , 2009.6.9
        ppptti1[i] = ppptti1[0] + i * tti1;
  
	for (i = 0; i < tti1; i++)
		for (j = 0; j < tti1; j++)
			ppptti1[i][j] = ppptti1[0][0] + (i * tti1 + j) * tti1;

	for (i = 0; i < tti1; i++)
		for (j = 0; j < tti1; j++)
			for (k = 0; k < tti1; k++)
				ppptti1[i][j][k] = 0;

	sphere_no_points = (int *) malloc((scale_info.max_scale + 1) * sizeof(int));
	if (sphere_no_points == NULL)
		MPI_Abort(MPI_COMM_WORLD,errno); 

	sphere_points = (short (**)[3]) malloc((scale_info.max_scale + 1) * sizeof(short (*)[3]));
	if (sphere_points == NULL)
		MPI_Abort(MPI_COMM_WORLD,errno); 

	tti1 = scale_info.max_scale + 5;

	for (k = 0; k < scale_info.max_scale; k++) 
	{
		sphere_no_points[k] = 0;
		for (i = -k - 2; i <= k + 2; i++)
			for (j = -k - 2; j <= k + 2; j++)
				for (l = -k - 2; l <= k + 2; l++)
					if (ppptti1[tti1 + i][tti1 + j][tti1 + l] == 0)
					{
						tt1 = sqrt(pow(((double) i) * anisotropy_slice, 2.0) +
							pow(((double) j) * anisotropy_row,
							2.0) + pow(((double) l) * anisotropy_col, 2.0));
						if (tt1 <= ((double) k) + 0.5)
						{
							sphere_no_points[k] = sphere_no_points[k] + 1;
							ppptti1[tti1 + i][tti1 + j][tti1 + l] = 2;
						}
					}

					sphere_points[k] = (short (*)[3]) malloc(3 * sphere_no_points[k] * sizeof(short));

					if (sphere_points[k] == NULL)
						MPI_Abort(MPI_COMM_WORLD,errno); 

					tti2 = 0;
					for (i = -k - 2; i <= k + 2; i++)
						for (j = -k - 2; j <= k + 2; j++)
							for (l = -k - 2; l <= k + 2; l++)
								if (ppptti1[tti1 + i][tti1 + j][tti1 + l] == 2)
								{
									ppptti1[tti1 + i][tti1 + j][tti1 + l] = 1;
									sphere_points[k][tti2][0] = i;
									sphere_points[k][tti2][1] = j;
									sphere_points[k][tti2][2] = l;
									tti2 = tti2 + 1;
								}
	}

	free(ppptti1[0][0]);
	free(ppptti1[0]);
	free(ppptti1);
}



void compute_feature_scale()
{
	double     mask[3][3];   /* for 3 by 3 distance-weighted average */
	int        xx, yy, x, y,z,slice=0,row=0,col=0;      /* loop counter */
	int        begin_slice,end_slice; 
	int        mean_g,flag,tti2,tti5;
	double     count_obj, count_nonobj, tt1, tt2, tt3;
	int        i,k;
	double     mask_total;

	mask_total = 0;
	for (yy = -1; yy <= 1; yy++)
	{
		for (xx = -1; xx <= 1; xx++)
		{
			tt2 = pow( xx, 2.0);
			tt2 = tt2 + pow(yy, 2.0);
			tt2 = 1 / (1 + tt2);
			mask[yy+1][xx + 1] = tt2;
			mask_total = mask_total + tt2;
		}
	}

//	begin_slice = 0;
//	end_slice = chunk_info.slices+chunk_info.right_overlap; // +1,  error

	begin_slice = chunk_info.left_overlap;
	if(chunk_info.right_overlap>0)
		end_slice = chunk_info.slices+chunk_info.left_overlap+1;
	else
		end_slice = chunk_info.slices+chunk_info.left_overlap;

	printf("chunk_info.num_of_bits = %d, end_slice = %d, prow = %d, pcol = %d \n", chunk_info.num_of_bits, end_slice, prow, pcol);
	

	if (chunk_info.num_of_bits == 8)  
	{
		for (slice = begin_slice; slice < end_slice; slice++)
			for (row = 0; row < prow; row++)
				for (col = 0; col < pcol; col++)
					if(chunk_data8[slice* slice_size + row * pcol + col] < cutoff_threshold)
						feature_scale[slice *prow*pcol + row * pcol + col] = 1;
					else  
					{
						flag = 0;
						tt1 = 0.0;
						tt3 = 0.0;
						for (yy = -1; yy <= 1; yy++)
							for (xx = -1; xx <= 1; xx++)  
							{
								x = xx + col;
								y = yy + row;
								z = slice;
								if (x >= 0 && y >= 0 && x < pcol && y < prow)
									tt3 = tt3 + mask[yy + 1][xx + 1]	
								* (double) chunk_data8[z * slice_size + y * pcol + x];
								else
									tt3 = tt3 + mask[yy + 1][xx + 1] 
								* (double) chunk_data8[slice * slice_size + row * pcol + col];
							}
							mean_g = (int) (tt3 / mask_total + 0.5);

							for (k = 1; k < scale_info.max_scale && !flag; k++)  
							{
								count_obj = 0;
								count_nonobj = 0;
								for (i = 0; i < sphere_no_points[k]; i++)  
								{
									x = col + sphere_points[k][i][2];
									y = row + sphere_points[k][i][1];
									z = slice + sphere_points[k][i][0];
									if (x < 0 || x >= pcol)
										x = col;
									if (y < 0 || y >= prow)
										y = row;
									if (z < 0 || z >= pslice)
										z = slice;

									tti5 = (int) chunk_data8[z * slice_size + y * pcol + x];
									tti5 = tti5 - mean_g;
									if (tti5 < 0)
										tti5 = -tti5;
									count_obj = count_obj + scale_map[tti5];
									count_nonobj = count_nonobj + 1.0 - scale_map[tti5];
								}
								if (100.0 * count_nonobj >= scale_info.tolerance
									* (count_nonobj + count_obj)) {
										feature_scale[slice * slice_size + row * pcol + col] = k;
										flag = 1;
								}
							}
							if (!flag)
								feature_scale[slice * slice_size + row * pcol + col] = k;
					}
	}
	else if (chunk_info.num_of_bits == 16) 
	{
		for (slice = begin_slice; slice < end_slice; slice++)
			for (row = 0; row < prow; row++)
				for (col = 0; col < pcol; col++)
					if((int) chunk_data16[slice* slice_size + row * pcol + col] < cutoff_threshold)
						feature_scale[slice * slice_size + row * pcol + col] = 1;
					else  
					{
						flag = 0;
						tt1 = 0.0;
						tt3 = 0.0;
						for (yy = -1; yy <= 1; yy++)
							for (xx = -1; xx <= 1; xx++)  
							{
								x = xx + col;
								y = yy + row;
								z = slice;
								if (x >= 0 && y >= 0 && x < pcol && y < prow)
									tt3 = tt3 + mask[yy + 1][xx + 1] * (double) chunk_data16[z * slice_size + y * pcol + x];
								else
									tt3 = tt3 + mask[yy + 1][xx + 1] * (double) chunk_data16[slice * slice_size+ row * pcol + col];
							}
							mean_g = (int) (tt3 / mask_total + 0.5);

							for (k = 1; k < scale_info.max_scale && !flag; k++)
							{
								count_obj = 0;
								count_nonobj = 0;
								for (i = 0; i < sphere_no_points[k]; i++)  
								{
									x = col + sphere_points[k][i][2];
									y = row + sphere_points[k][i][1];
									z = slice + sphere_points[k][i][0];
									if (x < 0 || x >= pcol)
										x = col;
									if (y < 0 || y >= prow)
										y = row;
									if (z < 0 || z >= pslice)
										z = slice;

									tti5 = (int) chunk_data16[z * slice_size + y * pcol + x];  // chunk_data8
									tti5 = tti5 - mean_g;
									if (tti5 < 0)
										tti5 = -tti5;
									
									if (tti5 > chunk_info.largest_intensity )
										tti5 = chunk_info.largest_intensity-1;

									count_obj += 0; //scale_map[tti5];
									count_nonobj += 1.0 ; //- scale_map[tti5];
								}
								if (100.0 * count_nonobj >= scale_info.tolerance * (count_nonobj + count_obj))
								{
										feature_scale[slice * slice_size + row * pcol + col] = k;
										flag = 1;
								}
							} 
							if (!flag)
								feature_scale[slice * slice_size + row * pcol + col] = k;
					}
	}
	printf("scale computation is done...\n");
} /* end of compute_feature_scale */


/***************************************************
* FUNCTION: compute_homogeneity
* DESCRIPTION: Computes the homogeneity values for the entire volume and  
*        store in the three arrays x-, y-, and z-affinity.
* PARAMETERS: None
* SIDE EFFECTS: 
* FUNCTIONS CALEED: None
* ENTRY CONDITIONS: 1) homogeneity_map array is alloted and 
*           proper values are assigned, and 2) 
* RETURN VALUE: None
* EXIT CONDITIONS: Compute homogeneity values in the three arrays 
*          x-, y-, and z-affinity
* HISTORY:
*  Created: 02/24/00
*  Modified:07/26/00 by Ying Zhuge to deal with 24 bits color image
*
*****************************************************************************/
void compute_homogeneity()
{

	int i, j, k, tti1, tti2,tti3,tti4, xx, yy, zz, x1, x2, y1, y2, z1, z2, x, y, z, iscale,
		scale1, scale2;
	double tt1, tt2, tt3, tt4, count_pos, count_neg, sum_pos, sum_neg, temp_sum_pos,
		temp_sum_neg, inv_k, tt_pos, tt_neg, sum, count, temp_sum, inv_half_scale;
	int col=0, row=0, slice=0, col1, row1, slice1,begin_slice,end_slice;

	double *x_affn_temp, *y_affn_temp, *z_affn_temp;


	begin_slice = chunk_info.left_overlap;
	if(chunk_info.right_overlap>0)
		end_slice = chunk_info.slices+chunk_info.left_overlap+1;
	else
		end_slice = chunk_info.slices+chunk_info.left_overlap;


	printf("begin_slice = %d, end_slice = %d", begin_slice, end_slice );

	for (slice = begin_slice; slice < end_slice; slice++)
		for (row = 0; row < prow; row++)
			for (col = 0; col < pcol - 1; col++)
			{
				if(chunk_info.num_of_bits == 8)
				{
					tti3 = chunk_data8[slice * slice_size + row * pcol + col];
					tti4 = chunk_data8[slice * slice_size + row * pcol + col + 1];
				}
				else if(chunk_info.num_of_bits == 16)
				{
					tti3 = chunk_data16[slice * slice_size + row * pcol + col];
					tti4 = chunk_data16[slice * slice_size + row * pcol + col + 1];
				}

				if((tti3 < cutoff_threshold) || (tti4<cutoff_threshold))
					x_affinity[slice * slice_size + row * pcol + col] = 0;
				else {
					col1 = col + 1;
					row1 = row;
					slice1 = slice;
					sum_pos = 0.0;
					sum_neg = 0.0;
					count_pos = 0.00001;
					count_neg = 0.00001;
					scale1 = (int) feature_scale[slice * slice_size + row * pcol + col];
					scale2 = (int) feature_scale[slice1 * slice_size + row1 * pcol + col1];
					//iscale = get_min(scale1, scale2);
					iscale = scale1<scale2?scale1:scale2;

					for (k = 0; k < iscale; k++)  {
						temp_sum_pos = 0.0;
						temp_sum_neg = 0.0;

						/************************* EXPERIMENTS ************************/
						switch (space_function)  {
					case 0:  /***** GAUSSIAN   *******/
						tt1 = (double) iscale;
						tt1 = 0.5 * tt1;
						inv_half_scale = -0.5 / pow(tt1, 2.0);
						inv_k = exp(inv_half_scale * pow((double) k, 2.0));
						break;
					case 1:  /******  LINEAR   ******/
						tt1 = (double) iscale;
						inv_k = 1.0 - (double) k / tt1;
						break;
					case 2:	/* BOX WINDOW  *********** */
						inv_k = 1.0;
						break;
						}

						/********************* END EXPERIMENTS *************************/
						for (i = 0; i < sphere_no_points[k]; i++)
						{

							xx = sphere_points[k][i][2];
							yy = sphere_points[k][i][1];
							zz = sphere_points[k][i][0];

							x1 = col + xx;
							x2 = col1 + xx;
							y1 = row + yy;
							y2 = row1 + yy;
							z1 = slice + zz;
							z2 = slice1 + zz;
							if(x1 >= 0 && x2 >= 0 && y1 >= 0 && y2 >= 0
								&& z1 >= 0 && z2 >= 0 && x1 < pcol && x2 < pcol
								&& y1 < prow && y2 < prow && z1 < pslice && z2 < pslice)  
							{
								if(chunk_info.num_of_bits == 8)
								{
									tti3 = chunk_data8[z1 * slice_size + y1 * pcol + x1];
									tti4 = chunk_data8[z2 * slice_size + y2 * pcol + x2];
								}
								else if(chunk_info.num_of_bits == 16)
								{
									tti3 = chunk_data16[z1 * slice_size + y1 * pcol + x1];
									tti4 = chunk_data16[z2 * slice_size + y2 * pcol + x2];
								}
								tti1 =  tti3 - tti4;
								if (tti1 < 0)  
								{
									tti1 = -tti1;
									temp_sum_neg = temp_sum_neg + 1 - homogeneity_map[tti1];
									count_neg = count_neg + inv_k;
								}
								else  
								{
									temp_sum_pos = temp_sum_pos + 1 - homogeneity_map[tti1];
									count_pos = count_pos + inv_k;
								}
							}
						}
						sum_neg = sum_neg + temp_sum_neg * inv_k;
						sum_pos = sum_pos + temp_sum_pos * inv_k;
			  }
					if (sum_pos > sum_neg)
						tt1 = 1 - (sum_pos - sum_neg) / (count_pos + count_neg);
					else
						tt1 = 1 - (sum_neg - sum_pos) / (count_pos + count_neg);
					x_affinity[slice * slice_size + row * pcol + col] =    (unsigned) (CONN * tt1);
				}
			}


			for (slice = begin_slice; slice < end_slice; slice++)
				for (row = 0; row < prow - 1; row++)
					for (col = 0; col < pcol; col++)
					{
						if(chunk_info.num_of_bits == 8)
						{
							tti3 = chunk_data8[slice * slice_size + row * pcol + col];
							tti4 = chunk_data8[slice * slice_size + (row+1) * pcol + col ];
						}
						else if(chunk_info.num_of_bits == 16)
						{
							tti3 = chunk_data16[slice * slice_size + row * pcol + col];
							tti4 = chunk_data16[slice * slice_size + (row+1) * pcol + col ];
						}

						if((tti3 < cutoff_threshold) || (tti4<cutoff_threshold))
							y_affinity[slice * slice_size + row * pcol + col] = 0;
						else {
							col1 = col;
							row1 = row + 1;
							slice1 = slice;
							sum_pos = 0.0;
							sum_neg = 0.0;
							count_pos = 0.00001;
							count_neg = 0.00001;
							scale1 = (int) feature_scale[slice * slice_size + row * pcol + col];
							scale2 = (int) feature_scale[slice1 * slice_size + row1 * pcol + col1];
							//iscale = get_min(scale1, scale2);
							iscale = scale1<scale2?scale1:scale2;

							for (k = 0; k < iscale; k++)  {
								temp_sum_pos = 0.0;
								temp_sum_neg = 0.0;

								/************************* EXPERIMENTS ************************/
								switch (space_function)  {
					case 0:  /***** GAUSSIAN   *******/
						tt1 = (double) iscale;
						tt1 = 0.5 * tt1;
						inv_half_scale = -0.5 / pow(tt1, 2.0);
						inv_k = exp(inv_half_scale * pow((double) k, 2.0));
						break;
					case 1:  /******  LINEAR   ******/
						tt1 = (double) iscale;
						inv_k = 1.0 - (double) k / tt1;
						break;
					case 2:	/* BOX WINDOW  *********** */
						inv_k = 1.0;
						break;
								}

								/********************* END EXPERIMENTS *************************/
								for (i = 0; i < sphere_no_points[k]; i++)
								{

									xx = sphere_points[k][i][2];
									yy = sphere_points[k][i][1];
									zz = sphere_points[k][i][0];

									x1 = col + xx;
									x2 = col1 + xx;
									y1 = row + yy;
									y2 = row1 + yy;
									z1 = slice + zz;
									z2 = slice1 + zz;
									if(x1 >= 0 && x2 >= 0 && y1 >= 0 && y2 >= 0
										&& z1 >= 0 && z2 >= 0 && x1 < pcol && x2 < pcol
										&& y1 < prow && y2 < prow && z1 < pslice && z2 < pslice)  
					    {
							if(chunk_info.num_of_bits == 8)
							{
								tti3 = chunk_data8[z1 * slice_size + y1 * pcol + x1];
								tti4 = chunk_data8[z2 * slice_size + y2 * pcol + x2];
							}
							else if(chunk_info.num_of_bits == 16)
							{
								tti3 = chunk_data16[z1 * slice_size + y1 * pcol + x1];
								tti4 = chunk_data16[z2 * slice_size + y2 * pcol + x2];
							}
							tti1 =  tti3 - tti4;
							if (tti1 < 0)  
							{
								tti1 = -tti1;
								temp_sum_neg = temp_sum_neg + 1 - homogeneity_map[tti1];
								count_neg = count_neg + inv_k;
							}
							else  
							{
								temp_sum_pos = temp_sum_pos + 1 - homogeneity_map[tti1];
								count_pos = count_pos + inv_k;
							}
					    }
								}
								sum_neg = sum_neg + temp_sum_neg * inv_k;
								sum_pos = sum_pos + temp_sum_pos * inv_k;
							}
							if (sum_pos > sum_neg)
								tt1 = 1 - (sum_pos - sum_neg) / (count_pos + count_neg);
							else
								tt1 = 1 - (sum_neg - sum_pos) / (count_pos + count_neg);
							y_affinity[slice * slice_size + row * pcol + col] = (unsigned) (CONN * tt1);
						}
					} /* finish for y_affinity computation */

					if (chunk_info.right_overlap>0)
						end_slice = chunk_info.slices+chunk_info.left_overlap+1;
					else
						end_slice = chunk_info.slices+chunk_info.left_overlap-1;  /* last chunk data */

					for (slice = begin_slice; slice < end_slice; slice++)
						for (row = 0; row < prow; row++)
							for (col = 0; col < pcol; col++)
							{
								if(chunk_info.num_of_bits == 8)
								{
									tti3 = chunk_data8[slice * slice_size + row * pcol + col];
									tti4 = chunk_data8[(slice+1) * slice_size + row* pcol + col ];
								}
								else if(chunk_info.num_of_bits == 16)
								{
									tti3 = chunk_data16[slice * slice_size + row * pcol + col];
									tti4 = chunk_data16[(slice+1) * slice_size + row* pcol + col ];
								}

								if((tti3 < cutoff_threshold) || (tti4<cutoff_threshold))
									z_affinity[slice * slice_size + row * pcol + col] = 0;
								else {
									col1 = col;
									row1 = row;
									slice1 = slice +1 ;
									sum_pos = 0.0;
									sum_neg = 0.0;
									count_pos = 0.00001;
									count_neg = 0.00001;
									scale1 = (int) feature_scale[slice * slice_size + row * pcol + col];
									scale2 = (int) feature_scale[slice1 * slice_size + row1 * pcol + col1];

									//iscale = get_min(scale1, scale2);
									iscale = scale1<scale2?scale1:scale2;
									for (k = 0; k < iscale; k++)  {
										temp_sum_pos = 0.0;
										temp_sum_neg = 0.0;

										/************************* EXPERIMENTS ************************/
										switch (space_function)  {
					case 0:  /***** GAUSSIAN   *******/
						tt1 = (double) iscale;
						tt1 = 0.5 * tt1;
						inv_half_scale = -0.5 / pow(tt1, 2.0);
						inv_k = exp(inv_half_scale * pow((double) k, 2.0));
						break;
					case 1:  /******  LINEAR   ******/
						tt1 = (double) iscale;
						inv_k = 1.0 - (double) k / tt1;
						break;
					case 2:	/* BOX WINDOW  *********** */
						inv_k = 1.0;
						break;
										}

										/********************* END EXPERIMENTS *************************/
										for (i = 0; i < sphere_no_points[k]; i++)
										{

											xx = sphere_points[k][i][2];
											yy = sphere_points[k][i][1];
											zz = sphere_points[k][i][0];

											x1 = col + xx;
											x2 = col1 + xx;
											y1 = row + yy;
											y2 = row1 + yy;
											z1 = slice + zz;
											z2 = slice1 + zz;
											if(x1 >= 0 && x2 >= 0 && y1 >= 0 && y2 >= 0
												&& z1 >= 0 && z2 >= 0 && x1 < pcol && x2 < pcol
												&& y1 < prow && y2 < prow && z1 < pslice && z2 < pslice)  
											{
												if(chunk_info.num_of_bits == 8)
												{
													tti3 = chunk_data8[z1 * slice_size + y1 * pcol + x1];
													tti4 = chunk_data8[z2 * slice_size + y2 * pcol + x2];
												}
												else if(chunk_info.num_of_bits == 16)
												{
													tti3 = chunk_data16[z1 * slice_size + y1 * pcol + x1];
													tti4 = chunk_data16[z2 * slice_size + y2 * pcol + x2];
												}
												tti1 =  tti3 - tti4;
												if (tti1 < 0)  
												{
													tti1 = -tti1;
													temp_sum_neg = temp_sum_neg + 1 - homogeneity_map[tti1];
													count_neg = count_neg + inv_k;
												}
												else  
												{
													temp_sum_pos = temp_sum_pos + 1 - homogeneity_map[tti1];
													count_pos = count_pos + inv_k;
												}
											}
										}
										sum_neg = sum_neg + temp_sum_neg * inv_k;
										sum_pos = sum_pos + temp_sum_pos * inv_k;
									}
									if (sum_pos > sum_neg)
										tt1 = 1 - (sum_pos - sum_neg) / (count_pos + count_neg);
									else
										tt1 = 1 - (sum_neg - sum_pos) / (count_pos + count_neg);
									z_affinity[slice * slice_size + row * pcol + col] =    (unsigned) (CONN * tt1);
								}
							}

							//----------------------------------------
							/*
							if(pobject>1)
							{
							for(i = 1;i<pobject;i++)
							for(j = 0;j<volume_size;j++)
							{
							x_affinity[i][j] = x_affinity[0][j];
							y_affinity[i][j] = y_affinity[0][j];
							z_affinity[i][j] = z_affinity[0][j];
							}
							} */
							//----------------------------------------
							printf("\rHomogeneity computation is done.     \n");

}


/***************************************************
* FUNCTION: compute_material
* DESCRIPTION: Computes the affinity values for the entire volumein three arrays
*        x-, y-, and z-affinity.
* PARAMETERS: None
* SIDE EFFECTS: 
* FUNCTIONS CALEED: compute_homogeneity, compute_scale
* ENTRY CONDITIONS: 1) scale and affinity arrays are alloted, 2) scale 
*           and homogeneity values are computed for each voxel, 
*           3) object_map arrays are alloted and 
*           proper values are assigned
* RETURN VALUE: None
* EXIT CONDITIONS: Modified affinity arrays incorporating feature values
* HISTORY:
*  Created: 02/24/00
*
*****************************************************************************/
void compute_material()
{
	int slice=0, row=0, col=0, i, j, k,begin_slice,end_slice;
	int tti1, tti, tti2, tti3,xx, yy, zz, x, y, z, iscale, object;
	float max_material, largest_material=0.0;
	double tt1, tt2;
	double sum, count, temp_sum, inv_k, inv_half_scale;
	int max_affinity, index_max;
	int edge_flag;
	float material[MAX_OBJECTS];

	begin_slice = chunk_info.left_overlap;
	if(chunk_info.right_overlap>0)
		end_slice = chunk_info.slices+chunk_info.left_overlap+1;
	else
		end_slice = chunk_info.slices+chunk_info.left_overlap;

	printf("begin_slice = %d, end_slice = %d", begin_slice, end_slice );

	for (slice = begin_slice; slice < end_slice; slice++)
		for (row = 0; row < prow; row++)
			for (col = 0; col < pcol; col++)
			{
				if (chunk_info.num_of_bits == 8)
					tti2 = chunk_data8[slice* slice_size + row * pcol + col];
				else if (chunk_info.num_of_bits == 16)
					tti2 = chunk_data16[slice* slice_size + row * pcol + col];

				if(tti2 < cutoff_threshold)
					pt_material[slice* slice_size + row * pcol + col] = 0;
				else 
				{
					for(object = 0;object<object_info.num_objects;object++)
					{
						iscale = feature_scale[slice * slice_size + row * pcol + col];
						sum = 0.0;
						count = 0.0;
						for (k = 0; k < iscale; k++)
						{
							tt1 = (double) iscale;
							tt1 = 0.5 * tt1;
							inv_half_scale = -0.5 / pow(tt1, 2.0);
							inv_k = exp(inv_half_scale * pow((double) k, 2.0));

							for (i = 0; i < sphere_no_points[k]; i++)
							{
								xx = sphere_points[k][i][2];
								yy = sphere_points[k][i][1];
								zz = sphere_points[k][i][0];

								x = col + xx;
								y = row + yy;
								z = slice + zz;
								if (x >= 0 && y >= 0 && z >= 0 
									&& x < pcol && y < prow && z < pslice)  
								{
									if (chunk_info.num_of_bits == 8)
										tti1 = chunk_data8[z * slice_size  + y * pcol + x];
									else if (chunk_info.num_of_bits == 16)
										tti1 = chunk_data16[z * slice_size  + y * pcol + x];

									//tt2 = object_map[object][feature][tti1];
									tt2 = exp(-0.5*(pow((double)(tti1-object_info.mean_intensity[object]),2)/pow((double)object_info.sigma[object],2))); 
									sum = sum + tt2;
									count = count + 1.0;
								}
							}
						}
						material[object] = sum/count;
					}

					max_material = material[0];
					for (object = 1; object < object_info.num_objects; object++)
						if (max_material < material[object])  
							max_material = material[object];
					pt_material[slice * slice_size + row * pcol + col] = max_material;
					if(largest_material < max_material)
						largest_material = max_material;
				}/* end of else */
			}


			//for (slice = chunk_info.left_overlap; slice < chunk_info.slices+chunk_info.left_overlap; slice++)
			for (slice = begin_slice; slice < end_slice; slice++)
				for (row = 0; row < prow; row++)
					for (col = 0; col < pcol; col++)
						pt_material[slice * slice_size + row * pcol + col] = 
						(float) CONN * pt_material[slice * slice_size + row * pcol + col]/largest_material;

			printf("\rMaterial computation is done.     \n");; 
}


/***************************************************
* FUNCTION: compute_affinity
* DESCRIPTION: Computes the affinity values for the entire volumein three arrays
*        x-, y-, and z-affinity.
* PARAMETERS: None
* SIDE EFFECTS: 
* FUNCTIONS CALEED: compute_homogeneity, compute_scale
* ENTRY CONDITIONS: 1) scale and affinity arrays are alloted, 2) scale 
*           and homogeneity values are computed for each voxel, 
*           3) object_map arrays are alloted and 
*           proper values are assigned
* RETURN VALUE: None
* EXIT CONDITIONS: Modified affinity arrays incorporating feature values
* HISTORY:
*  Created: 02/24/00
*
*****************************************************************************/
void compute_affinity()
{
	int i,j,slice=0, row=0, col=0, slice1, row1, col1,begin_slice,end_slice;

//	begin_slice = 0;
//	end_slice = chunk_info.slices+chunk_info.right_overlap;  // +1

	begin_slice = chunk_info.left_overlap;
	if(chunk_info.right_overlap>0)
		end_slice = chunk_info.slices+chunk_info.left_overlap+1;
	else
		end_slice = chunk_info.slices+chunk_info.left_overlap;

	printf("begin_slice = %d, end_slice = %d", begin_slice, end_slice );

	for (slice = begin_slice; slice < end_slice; slice++)
		for (row = 0; row < prow; row++)
			for (col = 0; col < pcol - 1; col++)  
			{
				slice1 = slice;
				row1 = row;
				col1 = col + 1;
				for(i = 0;i<pobject;i++)
				{
					if(pt_material[slice * slice_size + row * pcol + col] < 
						pt_material[slice1 * slice_size + row1 * pcol + col1])
						x_affinity[slice * slice_size + row * pcol + col] = 
						(unsigned short) sqrt(pt_material[slice * slice_size + row * pcol + col] 
					* (double)x_affinity[slice*slice_size +row*pcol+col]+0.5);
					else										
						x_affinity[slice * slice_size + row * pcol + col] = 
						(unsigned short)sqrt(pt_material[slice1 * slice_size + row1 * pcol + col1] 
					* (double)x_affinity[slice*slice_size +row*pcol+col]+0.5);
				}
			}

	for (slice = begin_slice; slice < end_slice; slice++)
		for (row = 0; row < prow-1; row++)
			for (col = 0; col < pcol; col++)	
			{
				slice1 = slice;
				row1 = row + 1;
				col1 = col;
				for(i = 0;i<pobject;i++)
				{
					if(pt_material[slice * slice_size + row * pcol + col] < 
						pt_material[slice1 * slice_size + row1 * pcol + col1])
						y_affinity[slice * slice_size + row * pcol + col] = 
						(unsigned short)sqrt(pt_material[slice * slice_size  + row * pcol + col] 
					* (double)y_affinity[slice*slice_size +row*pcol+col]+0.5);
					else										
						y_affinity[slice * slice_size + row * pcol + col] = 
						(unsigned short)sqrt(pt_material[slice1 * slice_size  + row1 * pcol + col1] 
					* (double)y_affinity[slice*slice_size +row*pcol+col]+0.5);
				}
			}

	if (chunk_info.right_overlap>0)
		end_slice = chunk_info.slices+chunk_info.left_overlap+1;
	else
		end_slice = chunk_info.slices+chunk_info.left_overlap-1;  /* last chunk data */

	for (slice = begin_slice; slice < end_slice; slice++)
		for (row = 0; row < prow; row++)
			for (col = 0; col < pcol; col++) 
			{
				slice1 = slice+1;
				row1 = row;
				col1 = col;
				for(i = 0;i<pobject;i++)
				{
					if(pt_material[slice * slice_size + row * pcol + col] < 
						pt_material[slice1 * slice_size + row1 * pcol + col1])
						z_affinity[slice * slice_size + row * pcol + col] = 
						(unsigned short)sqrt(pt_material[slice * slice_size + row * pcol + col] 
					* (double)z_affinity[slice*slice_size + row*pcol+col]+0.5);
					else										
						z_affinity[slice * slice_size + row * pcol + col] = 
						(unsigned short)sqrt(pt_material[slice1 * slice_size  + row1 * pcol + col1] 
					* (double)z_affinity[slice*slice_size +row*pcol+col]+0.5);
				}
			}

	printf("\rAffinity computation is done.\n");
}

/****************************************************************************
* FUNCTION: compute_affinity_feature_only
* DESCRIPTION: Computes the affinity values for the entire volume
*              in three arrays x-, y-, and z-affinity.
* PARAMETERS: None
* SIDE EFFECTS: 
* FUNCTIONS CALEED: compute_homogeneity, compute_scale
* ENTRY CONDITIONS: 1) scale and affinity arrays are alloted, 2) scale 
*           and homogeneity values are computed for each voxel, 
*           3) object_map arrays are alloted and
*           proper values are assigned
* RETURN VALUE: None
* EXIT CONDITIONS: Modified affinity arrays incorporating feature values
* HISTORY:
*  Created: 02/24/00
*
*****************************************************************************/
void compute_affinity_feature_only()
{
	int slice=0,row=0,col=0,slice1,row1,col1,i,begin_slice,end_slice;

	begin_slice = 0;
	end_slice = chunk_info.slices+chunk_info.right_overlap+1;

	for (slice = begin_slice; slice < end_slice; slice++) 
		for (row = 0; row < prow; row++)
			for (col = 0; col < pcol - 1; col++)  
			{
				slice1 = slice;
				row1 = row;
				col1 = col + 1;
				for(i = 0;i<pobject;i++)
				{
					if(pt_material[slice * slice_size + row * pcol + col] < 
						pt_material[slice1 * slice_size + row1 * pcol + col1])
						x_affinity[slice * slice_size + row * pcol + col] = 
						(unsigned short)(pt_material[slice * slice_size + row * pcol + col] +0.5);
					else										
						x_affinity[slice * slice_size + row * pcol + col] = 
						(unsigned short)(pt_material[slice1 * slice_size + row1 * pcol + col1]+0.5);
				}
			}

			for (slice = begin_slice; slice < end_slice; slice++) 
				for (row = 0; row < prow-1; row++)
					for (col = 0; col < pcol; col++)	
					{
						slice1 = slice;
						row1 = row + 1;
						col1 = col;
						for(i = 0;i<pobject;i++)
						{
							if(pt_material[slice * slice_size + row * pcol + col] < 
								pt_material[slice1 * slice_size + row1 * pcol + col1])
								y_affinity[slice * slice_size + row * pcol + col] = 
								(unsigned short)(pt_material[slice * slice_size + row * pcol + col] +0.5);
							else										
								y_affinity[slice * slice_size + row * pcol + col] = 
								(unsigned short)(pt_material[slice1 * slice_size + row1 * pcol + col1]+0.5);
						}
					}

					if (chunk_info.right_overlap>0)
						end_slice = chunk_info.slices+chunk_info.left_overlap+1;
					else
						end_slice = chunk_info.slices+chunk_info.left_overlap-1;  /* last chunk data */

					for (slice = begin_slice; slice < end_slice; slice++) 
						for (row = 0; row < prow; row++)
							for (col = 0; col < pcol; col++) 
							{
								slice1 = slice+1;
								row1 = row;
								col1 = col;
								for(i = 0;i<pobject;i++)
								{
									if(pt_material[slice * slice_size + row * pcol + col] < 
										pt_material[slice1 * slice_size + row1 * pcol + col1])
										z_affinity[slice * slice_size + row * pcol + col] = 
										(unsigned short)(pt_material[slice * slice_size + row * pcol + col] +0.5);
									else										
										z_affinity[slice * slice_size + row * pcol + col] = 
										(unsigned short)(pt_material[slice1 * slice_size + row1 * pcol + col1]+0.5);
								}
							}

							printf("\r feature affinity only computation is done.\n");

}

/****************INCLUDE FOR FAST TRACKING******************/

/*****************************************************************************
* FUNCTION: push_xyz_chash
* DESCRIPTION: Stores a voxel in a hashed heap structure with one
*    bin per value.
* PARAMETERS:
*    x, y, z: Coordinates of the voxel.
*    w: Value of the voxel.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variable H must be set.
*    H must be returned by chash_create.
* RETURN VALUE: Zero if successful.
* EXIT CONDITIONS: Non-zero on failure.
* HISTORY:
*    Created: 1/1999 by Laszlo Nyul
*    Modified: 2/1/99 returns 1 on failure by Dewey Odhner
*
*****************************************************************************/
int push_xyz_chash(short x, short y, short z, unsigned short w)
{
	VoxelWithValue tmp;

	tmp.x = x;
	tmp.y = y;
	tmp.z = z;
	tmp.val = w;
	return chash_push((Chash *)H, (char *)(&tmp));
}

/*****************************************************************************
* FUNCTION: repush_xyz_chash
* DESCRIPTION: Updates a voxel in a hashed heap structure with one
*    bin per value.
* PARAMETERS:
*    x, y, z: Coordinates of the voxel.
*    w: New value of the voxel.
*    ow: Old value of the voxel.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variable H must be set.
*    H must be returned by chash_create.
* RETURN VALUE: Voxel index into the volume
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 1/1999 by Laszlo Nyul
*
*****************************************************************************/
int repush_xyz_chash(short x, short y, short z, unsigned short w, unsigned short ow)
{
	VoxelWithValue tmp;

	tmp.x = x;
	tmp.y = y;
	tmp.z = z;
	tmp.val = w;
	chash_repush((Chash *)H, (char *)(&tmp), ow);

	return 1;
}

/*****************************************************************************
* FUNCTION: pop_xyz_chash
* DESCRIPTION: Retrieves a voxel from a hashed heap structure with one
*    bin per value.
* PARAMETERS:
*    x, y, z: Coordinates of the popped voxel go here.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variables H, dimensions must be set.
*    H must be returned by chash_create.
* RETURN VALUE: Voxel index into the volume
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 1/1999 by Laszlo Nyul
*
*****************************************************************************/
long pop_xyz_chash(short *x, short *y, short *z)
{
	VoxelWithValue tmp;
	long kkk;

	chash_pop((Chash *)H, (char *)&tmp);
	*x = tmp.x;
	*y = tmp.y;
	*z = tmp.z;
	kkk = tmp.z * slice_size + tmp.y * pcol + tmp.x;
	return kkk;
}

/*****************************************************************************
* FUNCTION: push_xyz_chash2
* DESCRIPTION: Stores a voxel in a hashed heap structure with one
*    bin per value and doubly linked lists.
* PARAMETERS:
*    x, y, z: Coordinates of the voxel.
*    w: Value of the voxel.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variable H must be returned by chash_create2.
* RETURN VALUE: Zero if successful.
* EXIT CONDITIONS: Non-zero on failure.
* HISTORY:
*    Created: 2/3/99 by Dewey Odhner
*
*****************************************************************************/
int push_xyz_chash2(short x, short y, short z, unsigned short w)
{
	VoxelWithValue tmp;

	tmp.x = x;
	tmp.y = y;
	tmp.z = z;
	tmp.val = w;
	return chash_push2((Chash *)H, (char *)(&tmp));
}

/*****************************************************************************
* FUNCTION: repush_xyz_chash2
* DESCRIPTION: Updates a voxel in a hashed heap structure with one
*    bin per value and doubly linked lists.
* PARAMETERS:
*    x, y, z: Coordinates of the voxel.
*    w: New value of the voxel.
*    ow: Old value of the voxel.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variable H must be returned by chash_create2.
* RETURN VALUE: Voxel index into the volume
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 2/3/99 by Dewey Odhner
*
*****************************************************************************/
int repush_xyz_chash2(short x, short y, short z, unsigned short w, unsigned short ow)
{
	VoxelWithValue tmp;

	tmp.x = x;
	tmp.y = y;
	tmp.z = z;
	tmp.val = w;
	chash_repush2((Chash *)H, (char *)(&tmp), ow);

	return 1;
}

/*****************************************************************************
* FUNCTION: pop_xyz_chash2
* DESCRIPTION: Retrieves a voxel from a hashed heap structure with one
*    bin per value and doubly linked lists.
* PARAMETERS:
*    x, y, z: Coordinates of the popped voxel go here.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variables H, dimensions must be set.
*    H must be returned by chash_create2.
* RETURN VALUE: Voxel index into the volume
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 2/3/99 by Dewey Odhner
*
*****************************************************************************/
long pop_xyz_chash2(short *x, short *y, short *z)
{
	VoxelWithValue tmp;
	long kkk;

	chash_pop2((Chash *)H, (char *)(&tmp));
	*x = tmp.x;
	*y = tmp.y;
	*z = tmp.z;
	kkk = tmp.z * slice_size + tmp.y * pcol + tmp.x;
	return kkk;
}

/*****************************************************************************
* FUNCTION: voxel_value
* DESCRIPTION: Returns the value of a voxel.
* PARAMETERS:
*    v: Pointer to the voxel
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: Value of the voxel.
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 2/1/99 by Dewey Odhner
*
*****************************************************************************/
unsigned short voxel_value(VoxelWithValue *v)
{
	return v->val;
}

/*****************************************************************************
* FUNCTION: hheap_hash_0
* DESCRIPTION: Returns a hash value for a voxel.
* PARAMETERS:
*    hashsize: The number of hash bins
*    v: The voxel
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variable dimensions must be set.
* RETURN VALUE: Hash value for the voxel.
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 1998 by Laszlo Nyul
*
*****************************************************************************/
long hheap_hash_0(long hashsize, VoxelWithValue *v)
{
	return (v->z * slice_size + v->y * pcol + v->x) % hashsize;
}

/*****************************************************************************
* FUNCTION: push_xyz_hheap
* DESCRIPTION: Stores a voxel in a hashed heap structure.
* PARAMETERS:
*    x, y, z: Coordinates of the voxel.
*    w: Value of the voxel.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variable H must be set.  H must be of type Hheap *.
* RETURN VALUE: Zero if successful.
* EXIT CONDITIONS: Non-zero on failure.
* HISTORY:
*    Created: 1998 by Laszlo Nyul
*    Modified: 1/19/99 returns 1 on failure by Dewey Odhner
*
*****************************************************************************/
int push_xyz_hheap(short x, short y, short z,unsigned short w)  
{
	VoxelWithValue tmp;

	tmp.x = x;
	tmp.y = y;
	tmp.z = z;
	tmp.val = w;
	return hheap_push((Hheap *)H, (char *)(&tmp));
}

/*****************************************************************************
* FUNCTION: repush_xyz_hheap
* DESCRIPTION: Updates a voxel in a hashed heap structure.
* PARAMETERS:
*    x, y, z: Coordinates of the voxel.
*    w: New value of the voxel.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variable H must be set.  H must be of type Hheap *.
* RETURN VALUE: Voxel index into the volume
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 1998 by Laszlo Nyul
*
*****************************************************************************/
int repush_xyz_hheap(short x,  short y, short z, unsigned short w)  
{
	VoxelWithValue tmp;

	tmp.x = x;
	tmp.y = y;
	tmp.z = z;
	tmp.val = w;
	hheap_repush((Hheap *)H, (char *)(&tmp));

	return 1;
}

/*****************************************************************************
* FUNCTION: pop_xyz_hheap
* DESCRIPTION: Retrieves a voxel from a hashed heap structure.
* PARAMETERS:
*    x, y, z: Coordinates of the popped voxel go here.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: The variables H, dimensions must be set.  H must be of
*    type Hheap *.
* RETURN VALUE: Voxel index into the volume
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 1998 by Laszlo Nyul
*
*****************************************************************************/
long pop_xyz_hheap(short *x, short *y, short *z)
{
	VoxelWithValue tmp;
	long kkk;

	hheap_pop((Hheap *)H, (char *)(&tmp));
	*x = tmp.x;
	*y = tmp.y;
	*z = tmp.z;
	kkk = tmp.z * slice_size + tmp.y * pcol + tmp.x;
	return kkk;
}

/*****************************************************************************
* FUNCTION: voxel_value_cmp
* DESCRIPTION: Returns relative ordering of two voxels by value
* PARAMETERS:
*    v, vv: Pointers to the voxels.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: -1, 0, or 1
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 1/27/99 by Dewey Odhner
*
*****************************************************************************/
int voxel_value_cmp(VoxelWithValue *v, VoxelWithValue * vv)
{
	return v->val>vv->val? 1: v->val<vv->val? -1: 0;
}

/*****************************************************************************
* FUNCTION: voxel_cmp
* DESCRIPTION: Returns relative ordering of two voxels in a volume
* PARAMETERS:
*    v, vv: Pointers to the voxels.  A null pointer is smaller than any voxel.
* SIDE EFFECTS: None
* ENTRY CONDITIONS: None
* RETURN VALUE: -1, 0, or 1
* EXIT CONDITIONS: None
* HISTORY:
*    Created: 1/20/99 by Dewey Odhner
*
*****************************************************************************/
int voxel_cmp(VoxelWithValue *v, VoxelWithValue *vv)
{
	if (v==NULL && vv==NULL)
		return 0;
	if (v == NULL)
		return -1;
	if (vv == NULL)
		return 1;
	if (v->x > vv->x)
		return 1;
	if (v->x < vv->x)
		return -1;
	if (v->y > vv->y)
		return 1;
	if (v->y < vv->y)
		return -1;
	if (v->z > vv->z)
		return 1;
	if (v->z < vv->z)
		return -1;
	return 0;
}


/*****************************************************************************
* FUNCTION: fuzzy_track
* DESCRIPTION: Computes the connectedness values and stores them at out_data.
* PARAMETERS:
*    current_volume: volume number from zero in the subscene.
* SIDE EFFECTS: Messages may be written to stdout.
* ENTRY CONDITIONS: The variables feature_status, function_level,
*    function_width, weight, function_selected, file_info, vh_in, in_data,
*    out_affinity_flag, fuzzy_adjacency_flag, slice_spacing,
*    points_filename must be properly set.
* RETURN VALUE: None
* EXIT CONDITIONS: On error, writes a message to stderr and exits with code 1.
* HISTORY:
*    Created: 4/3/96 by Dewey Odhner
*    Modified: 4/15/96 PushNeighbors fixed by Dewey Odhner
*    Modified: 4/25/97 affinity image stored by Dewey Odhner
*    Modified: 1/27/98 direction-specific affinity image stored
*       by Dewey Odhner
*    Modified: 2/24/98 points loaded from binary scene by Dewey Odhner
*    Modified: 1/21/99 hashed heap used instead of queue by Dewey Odhner
*    Modified: 2/1/99 connectivity used for hash value by Dewey Odhner
*    Modified: 3/26/99 affinity computed during tracking by Dewey Odhner
*
*****************************************************************************/

void fuzzy_track()
{

	int i,j, k, counter, toggle;
	int ei, x, y, z;
	Voxel cur;
	unsigned char *in_points_data;
	OutCellType *affp[6];
	unsigned pmin, Pmax, afn;
	float x_adjacency, y_adjacency, z_adjacency, e_adjacency[6];
	FILE *fp_pts, *fp_points;

	H = chash_create2(CONN+1L, sizeof(VoxelWithValue), voxel_value, NULL);
	if (H == NULL)
	{
		printf("Heap creation error...\n");
		exit(-1);
	}

	memset(flag_data,0,volume_size*sizeof(char));

	for(i = 0;i<slave_numseeds;i++)
	{
		/*
		cur.x = x;
		cur.y = y;
		cur.z = z;  */
		k = slave_seeds[i].z*slice_size + slave_seeds[i].y * pcol + slave_seeds[i].x;
		out_data[k] = slave_seeds[i].val;
		if (Push_xyz(slave_seeds[i].x, slave_seeds[i].y, slave_seeds[i].z, slave_seeds[i].val))
		{
			printf("Heap operation error...\n");
			exit(-1);
		}
		flag_data[k] = 1;
	}

	affp[0] = x_affinity;
	affp[1] = y_affinity;
	affp[2] = z_affinity;
	affp[3] = x_affinity-1;
	affp[4] = y_affinity-pcol;
	affp[5] = z_affinity-slice_size;
	counter = 0;

	while ( !chash_isempty((Chash*)H) ) 
        {
		/*
		if (counter--==0)
		{
		counter = 100000;
		if (toggle = !toggle)
		printf("\rTracking.  ");
		else
		printf("\rTracking.. ");
		fflush(stdout);
		} */
		Pmax = out_data[Pop_xyz(&cur.x, &cur.y, &cur.z)];
		for (ei = 0; ei < 6; ei++)
		{
			x = cur.x + nbor[ei].x;
			y = cur.y + nbor[ei].y;
			z = cur.z + nbor[ei].z;
			if (x >= 0 && x < pcol &&
				y >= 0 && y < prow &&
				z >= 0 && z < pslice)
			{
				j= cur.z*slice_size + cur.y*pcol + cur.x;
				k = z * slice_size + y * pcol + x;
				afn = affp[ei][j];
				pmin = Pmax<afn?Pmax:afn; 
				if(pmin > out_data[k])
				{
					//if (out_data[k] == 0)
					if(flag_data[k]==0)
					{
						if (Push_xyz(x, y, z, pmin))
						{
							printf("Heap operation error...\n");
							exit(-1);
						}
						flag_data[k]=1;
					}
					else
						Repush_xyz(x, y, z, pmin, out_data[k]);
					out_data[k] = pmin;
				}
			}
		}
	}

	printf("\rTracking done.\n");
	fflush(stdout);    
}
