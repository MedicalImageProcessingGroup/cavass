/*
  Copyright 1993-2015 Medical Image Processing Group
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

#include <stdio.h>
#include <math.h>
#include "cavass.h"
#include "auto_register.h"

#define SIGN(x) ((x>0.000000001) ? 1 : (x<-0.000000001) ? -1 : 0)
#define SQR(x) (x)*(x)
#define BEST_PI acos(-1.0) 

#define POSITIVE (unsigned short)(0x8000)
#define C_MASK (unsigned short)(0x7FFF)

#define NUM_FEATURE_LINES 11
//#define MSG_WIDTH  200

static double get_axis_length(REG_SEQ *inst, Surf_Struct *str, int axis, KMMATRIX transf );
static double get_volume(Surf_Struct *str);

Surf_Info *surf;
int num_surf;
int CURRENT_SURF,CURRENT_MODE_LEVEL;
int CURRENT_REAL_OBJ;
REG_SEQ **seq,**out_seq;
int time_inst,out_time_inst,REL_OBJECT;
int num_objs,CURRENT_INST,LAST_INST,LAST_LOC;
int CURRENT_OBJ;
int ref_time;
char feature_text[NUM_FEATURE_LINES][MSG_WIDTH];

REG_SEQ static_inst[2];

extern DEFS Defaults;

int Register_Intra(Surf_Info *InSurf, wxArrayString*  pstrOutPut, int *nInstancesNum)
{
	Surf_Tree *ptr;
	int i,j,error;
	KMMATRIX M;	

	surf = InSurf;

	CURRENT_SURF=0;

	for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
		surf[CURRENT_SURF].tree_index[i]=0;

	ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
	num_objs=ptr->children;
	if (num_objs<1) return(-1);
	seq=(REG_SEQ **)malloc(sizeof(REG_SEQ *)*num_objs);
	if (seq==NULL) 
	{
		wxLogMessage("Memory allocation error");
		return(-1);
	}
#ifdef SCRIPTABLE_VERSION
	if (rel_flag>=0) {
		if (rel_flag > num_objs-1) {
			DisplayMessage("Incorrect Relative flag.There arn't that many object");
			exit(-1);
		}
		else
			REL_OBJECT=rel_flag+1;
	}
	else 
		REL_OBJECT=0;
#endif
	out_seq=(REG_SEQ **)malloc(sizeof(REG_SEQ *)*num_objs);
	if (out_seq==NULL) 
	{
		wxLogMessage("Memory allocation error");
		return(-1);
	}

	error=0;
	time_inst=1;
	while (!error)
	{
		error=GetNextStructureIndex(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index);
		if (!error) 
		{
			ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
			if (ptr->children!=num_objs) {
				wxLogMessage("The system does not contain symmetric sequences");
				free(seq);
				return(-1);
			}
			else
				time_inst++;
		}
	}
	for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
		surf[CURRENT_SURF].tree_index[i]=0;
	for(i=0;i<num_objs;i++)
	{
		if ((seq[i]=(REG_SEQ *)malloc(sizeof(REG_SEQ)*time_inst))==NULL) 
		{
			wxLogMessage("Could Not Allocate Memory");
			for(j=0;j<i;j++) free(seq[j]);
			free(seq);
			return(-1);
		}
	}

	for(i=0;i<time_inst;i++) 
	{
		ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
		for(j=0;j<num_objs;j++)
		{
			Load_Data(CURRENT_SURF,(int)(ptr->child[j]->surf_index));
			//  VSelectCursor(ALL_WINDOWS,XC_watch);
			InitInstance(&seq[j][i],CURRENT_SURF,(int)(ptr->child[j]->surf_index),
				ptr->child[j]->transf);
			//   VSelectCursor(ALL_WINDOWS,DEFAULT_CURSOR);
		}
		error=GetNextStructureIndex(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index);
	}

#ifdef SCRIPTABLE_VERSION
	if (instance>1)
		out_time_inst=instance;
	else
		out_time_inst=time_inst;
#endif


	/* The Eigen analysis does not resolve the direction of the axis calculated         */
	/* Thus to make sure axis in each time instance is consistant sign of the direction */
	/* is flipped so as to give the smallest possible angular change between instances  */

	MakeAxisConsistant(); 

	/* Get Rotation and Translation */
	GetRotationAndTranslation();

#ifdef SCRIPTABLE_VERSION
	if (SaveIntraPlan(outfile)!=0)
		return(-1);
#else
	CURRENT_OBJ=0;
	CURRENT_INST=0;
	*nInstancesNum = time_inst;
	wxLogMessage("Displaying List. Wait...");
	DisplayListing(surf, CURRENT_OBJ,CURRENT_INST, pstrOutPut);
	wxLogMessage("Done Displaying List.");
	printf("After DispListing\n");

	// Intra_EventHandler();
#endif

	return(0);

}


void KinematicsIntraRelease()
{
	/*Surf_Tree *ptr;	
	KMMATRIX M;	
	ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);

	int i,j;
	for(i=0;i<time_inst;i++)
		for(j=0;j<num_objs;j++)
			ReleaseData(0,(int)ptr->child[j]->surf_index);*/

	surf = NULL;
	//for(i=0;i<num_objs;i++)
	//	free(seq[i]);
	//free(seq);
	//free(out_seq);
}
/************************************************************************
 *
 *      FUNCTION        : Load_Data
 *
 *      DESCRIPTION     : Given a current strucutre system and a
 *                        structure of interest in it, Load_Data
 *                        would check if the data has been read already 
 *                        and if not would read it in.
 *
 *      RETURN VALUE    : 1 - on success, -1 - on error.
 *
 *      PARAMETERS      : sf - current structure system index
 *                        st - current strucutre index
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : surf should be initialized.
 *
 *      EXIT CONDITIONS : memory allocation or read error.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int Load_Data(int sf, int st)
{
  int i,j,items;
  Surf_Struct *str;
  char msg[70];
  Cord_with_Norm *ptr;

 // VSelectCursor(ALL_WINDOWS,XC_watch);
  sprintf(msg,"Loading data of system %d structure %d",sf,st);
  wxLogMessage(wxString::Format("%s", msg )); 
  
  str=&surf[sf].surf_struct[st];
  if (str->DATA_LOADED) return(1);
  /* Move to the Current position */
  VSeekData(surf[sf].fp,str->start);
  
  ptr=(Cord_with_Norm *)malloc(sizeof(Cord_with_Norm)*surf[sf].vh.str.num_of_TSE[st]);
  if (ptr==NULL) 
  {
    wxLogMessage("Could not allocate memory for surface");
     return(-1);
  }
  if (VReadData((char *)ptr,2,2*surf[sf].vh.str.num_of_TSE[st],surf[sf].fp,&items)) 
  {
    wxLogMessage("Could not read data for surface");
    free(ptr);     
    return(-1);
  }
  for(i=0;i<str->slices;i++)
    for(j=0;j<str->rows;j++) 
	{
      str->node[i][j]=ptr;
      ptr+= str->count[i][j];
    }
  
  str->DATA_LOADED=1;
  wxLogMessage("done loading");
  //VSelectCursor(ALL_WINDOWS,XC_top_left_arrow);  

  return(1);
}


/* Modified: 12/1/99 inst->mu3 computed (third moment along principal axes)
 by Dewey Odhner. */
/* Modified: 8/23/00 inst->mu3 corrected by Dewey Odhner. */
void InitInstance(REG_SEQ *inst, int sf, int st, KMMATRIX M)
{
  int i,j,k;
  Surf_Struct *str;
  Cord_with_Norm *ptr;
  double sum[3],sum_sqr[3],cros_sum[3];
  double eval[3],evec[3][3];
  double m[3][3], len, vol=0;
  KMPOINT pta, ptb, pt;
  KMMATRIX TRANS;

  for(i=0;i<4;i++)
    for(j=0;j<4;j++)
      inst->M[i][j]=M[i][j];

  MultMat(TRANS,M,surf[sf].surf_struct[st].to_scanner);
 
  sum[0]=sum[1]=sum[2]=0.0;
  sum_sqr[0]=sum_sqr[1]=sum_sqr[2]=0.0;
  cros_sum[0]=cros_sum[1]=cros_sum[2]=0.0;
  str= &(surf[sf].surf_struct[st]);
  for (i=1; i<str->slices; i+=2)
	  for (j=1; j<str->rows; j+=2)
	  {
		  ptr=(Cord_with_Norm *)str->node[i][j];
		  for (k=0; k<str->count[i][j]; k+=2)
		  {
			  pt[0]=(double)(ptr[k][0]&C_MASK);
			  pt[1]=(double)j;
			  pt[2]=(double)i;
			  TransformPoint(pta,TRANS,pt);
			  pt[0]=(double)(ptr[k+1][0]&C_MASK);
			  TransformPoint(ptb,TRANS,pt);
			  len = sqrt((ptb[0]-pta[0])*(ptb[0]-pta[0])+
			             (ptb[1]-pta[1])*(ptb[1]-pta[1])+
						 (ptb[2]-pta[2])*(ptb[2]-pta[2]));
			  sum[0] += 0.5*len*(pta[0]+ptb[0]);
			  sum[1] += 0.5*len*(pta[1]+ptb[1]);
			  sum[2] += 0.5*len*(pta[2]+ptb[2]);
			  vol += len;
		  }
	  }
  inst->cent[0] = sum[0]/vol;
  inst->cent[1] = sum[1]/vol;
  inst->cent[2] = sum[2]/vol;

  if (surf[sf].vh.str.description_valid)
    get_isoshape_center(inst->cent, surf[sf].vh.str.description, st, M);

  m[0][0]=m[0][1]=m[0][2]=m[1][0]=m[1][1]=m[1][2]=m[2][0]=m[2][1]=m[2][2]=0.0;
  for (i=1; i<str->slices; i+=2)
	  for (j=1; j<str->rows; j+=2) 
	  {
		  ptr=(Cord_with_Norm *)str->node[i][j];
		  for (k=0; k<str->count[i][j]; k+=2) 
		  {
			  pt[0]=(double)(ptr[k][0]&C_MASK);
			  pt[1]=(double)j;
			  pt[2]=(double)i;
			  TransformPoint(pta,TRANS,pt);
			  pta[0] -= inst->cent[0];
			  pta[1] -= inst->cent[1];
			  pta[2] -= inst->cent[2];
			  pt[0]=(double)(ptr[k+1][0]&C_MASK);
			  TransformPoint(ptb,TRANS,pt);
			  ptb[0] -= inst->cent[0];
			  ptb[1] -= inst->cent[1];
			  ptb[2] -= inst->cent[2];
			  len = sqrt((ptb[0]-pta[0])*(ptb[0]-pta[0])+
			             (ptb[1]-pta[1])*(ptb[1]-pta[1])+
						 (ptb[2]-pta[2])*(ptb[2]-pta[2]));
			  for (int ii=0; ii<3; ii++)
			  	for (int jj=0; jj<3; jj++)
				  m[ii][jj] += len*(0.5*(pta[ii]*ptb[jj]+ptb[ii]*pta[jj])+
				    1/3.*(pta[ii]-ptb[ii])*(pta[jj]-ptb[jj]));
		  }
	  }
  for(i=0;i<3;i++)
    for(j=0;j<3;j++)
      m[i][j] /= vol;

  GtEval(m,eval);
  SortEval(eval); /* Sort and get primary secodary and ternary axis */
  inst->eval[0]=eval[0];  inst->eval[1]=eval[1];  inst->eval[2]=eval[2];
  GtEvec(m,eval,evec);
  for(i=0;i<3;i++) {
    inst->axis[0][i]=evec[0][i];
    inst->axis[1][i]=evec[1][i];
    inst->axis[2][i]=evec[2][i];
  }

  inst->mu3[0] = inst->mu3[1] = inst->mu3[2] = 0;
#ifdef COMPUTE_MU3
  unsigned long num=0;
  for (i=1; i<str->slices; i+=2)
	  for (j=1; j<str->rows; j+=2)
	  {
		  ptr=(Cord_with_Norm *)str->node[i][j];
		  for (k=0; k<str->count[i][j]; k+=2)
		  {
			  pt[1]=(double)j;
			  pt[2]=(double)i;
			  for (int ii=ptr[k][0]&C_MASK; ii<(ptr[k+1][0]&C_MASK); ii++)
			  {
				  pt[0] = ii+0.5;
				  TransformPoint(pta,TRANS,pt);
				  pta[0] -= inst->cent[0];
				  pta[1] -= inst->cent[1];
				  pta[2] -= inst->cent[2];
				  td = pta[0]*inst->axis[0][0]+pta[1]*inst->axis[0][1]+
					  pta[2]*inst->axis[0][2];
				  inst->mu3[0] += td*td*td;
				  td = pta[0]*inst->axis[1][0]+pta[1]*inst->axis[1][1]+
					  pta[2]*inst->axis[1][2];
				  inst->mu3[1] += td*td*td;
				  td = pta[0]*inst->axis[2][0]+pta[1]*inst->axis[2][1]+
					  pta[2]*inst->axis[2][2];
				  inst->mu3[2] += td*td*td;
			  }
			  num += (ptr[k+1][0]&C_MASK)-(ptr[k][0]&C_MASK);
		  }
	  }
  inst->mu3[0] /= num;
  inst->mu3[1] /= num;
  inst->mu3[2] /= num;
#endif
}

/*****************************************************************************
 * FUNCTION: get_isoshape_center
 * DESCRIPTION: Gets the reference point for a structure, if one is specified
 *    as isoshape center for it.
 * PARAMETERS:
 *    cent: The reference point is returned here if found and if cent non-NULL.
 *    description: The extraction parameters for the structure system.
 *    st: The structure number.
 *    M: Transformation to be applied to the point if found.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: TRUE if reference point is specified for the structure.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/14/02 by Dewey Odhner
 *
 *****************************************************************************/
int get_isoshape_center(KMPOINT cent, char *description, int st, KMMATRIX M)
{
	static char label[]="isoshape center:";
	int k;
	KMPOINT pt;

	for (k=0; k<st; description++)
	{
		if (*description == 0)
			return (FALSE);
		if (*description == '\\')
			k++;
	}
	for (k=0; k<(int)sizeof(label)-1; description++)
	{
		if (*description==0 || *description=='\\')
			return (FALSE);
		if (*description == label[k])
			k++;
		else
			k = 0;
	}
	if (sscanf(description, " %lf %lf %lf", pt, pt+1, pt+2) != 3)
		return (FALSE);
	if (cent)
		TransformPoint(cent, M, pt);
	return (TRUE);
}


/*****************************************************************************
 * FUNCTION: MakeAxisConsistent
 * DESCRIPTION: Makes sure two sets of axes are right-handed and aligned
 *    as much as possible by flipping axes.
 * PARAMETERS:
 *    axis2: The axis set to adjust
 *    axis1: The axis set to align to
 *    mu3_2, mu3_1: The third moments associated with axis2 and axis1
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 12/1/99 by Dewey Odhner
 *    Modified: 4/9/02 criterion for flipping axes changed by Dewey Odhner
 *
 *****************************************************************************/
void MakeAxisConsistent(KMPOINT axis2[3], KMPOINT axis1[3], KMPOINT mu3_2, KMPOINT mu3_1 )
{
	int j, k, m;
	double tfom, best_fom;

	if (Det3(axis1) < 0)
	{
		axis1[2][0] = -axis1[2][0];
		axis1[2][1] = -axis1[2][1];
		axis1[2][2] = -axis1[2][2];
		mu3_1[2] = -mu3_1[2];
	}
	if (Det3(axis2) < 0)
	{
		axis2[2][0] = -axis2[2][0];
		axis2[2][1] = -axis2[2][1];
		axis2[2][2] = -axis2[2][2];
		mu3_2[2] = -mu3_2[2];
	}
	best_fom = 0;
	for (j=0; j<3; j++)
		for (k=0; k<3; k++)
			best_fom += axis1[j][k]*axis2[j][k];
	for (j=0; j<3; j++)
	{
		tfom = 0;
		for (m=0; m<3; m++)
			for (k=0; k<3; k++)
				tfom+= m==j? axis1[j][k]*axis2[j][k]: -axis1[m][k]*axis2[m][k];
		if (tfom > best_fom)
		{
			for (k=0; k<3; k++)
				if (k != j)
				{
					axis2[k][0] = -axis2[k][0];
					axis2[k][1] = -axis2[k][1];
					axis2[k][2] = -axis2[k][2];
					mu3_2[k] = -mu3_2[k];
				}
			best_fom = tfom;
			if (j > 0)
				j = -1;
		}
	}
/* Was:
	for (j=0; j<3; j++)
		if (mu3_1[(j+1)%3]*mu3_2[(j+1)%3]+
			mu3_1[(j+2)%3]*mu3_2[(j+2)%3] < 0)
		{
			for (k=0; k<3; k++)
				if (k != j)
				{
					axis2[k][0] = -axis2[k][0];
					axis2[k][1] = -axis2[k][1];
					axis2[k][2] = -axis2[k][2];
					mu3_2[k] = -mu3_2[k];
				}
			if (j > 0)
				j = -1;
		}
*/
}


/*    Modified: 2/11/03 criterion for flipping axes changed by Dewey Odhner */
void MakeAxisConsistant()
{
  int i,j;

  for(j=0;j<num_objs;j++) 
  {
	if (seq[j][0].axis[1][1] > 0)
	{
		seq[j][0].axis[1][0] = -seq[j][0].axis[1][0];
		seq[j][0].axis[1][1] = -seq[j][0].axis[1][1];
		seq[j][0].axis[1][2] = -seq[j][0].axis[1][2];
		seq[j][0].mu3[1] = -seq[j][0].mu3[1];
	}
	if (seq[j][0].axis[2][2] > 0)
	{
		seq[j][0].axis[2][0] = -seq[j][0].axis[2][0];
		seq[j][0].axis[2][1] = -seq[j][0].axis[2][1];
		seq[j][0].axis[2][2] = -seq[j][0].axis[2][2];
		seq[j][0].mu3[2] = -seq[j][0].mu3[2];
	}
	if (Det3(seq[j][0].axis) < 0)
	{
		seq[j][0].axis[0][0] = -seq[j][0].axis[0][0];
		seq[j][0].axis[0][1] = -seq[j][0].axis[0][1];
		seq[j][0].axis[0][2] = -seq[j][0].axis[0][2];
		seq[j][0].mu3[0] = -seq[j][0].mu3[0];
	}
  }
  for(i=0;i<time_inst-1;i++)
    for(j=0;j<num_objs;j++) 
      MakeAxisConsistent(seq[j][i+1].axis, seq[j][i].axis,
	    seq[j][i+1].mu3, seq[j][i].mu3);
}


void GetRotationAndTranslation()
{
  int i,j,k,l,m;
  KMMATRIX M;
  double A[3][3],B[3][3],AInv[3][3],d,sin_val,cos_val;
  
  for(i=0;i<time_inst-1;i++)
    for(j=0;j<num_objs;j++) {
      MakeID(M);
      /* Get the Rotational KMMATRIX */
      for(k=0;k<3;k++)
	for(l=0;l<3;l++) {
	  A[k][l]=seq[j][i].axis[l][k];
	  B[k][l]=seq[j][i+1].axis[l][k];
	}
      Inv3Mat(A,AInv);
      for(k=0;k<3;k++)
	for(l=0;l<3;l++) {
	  M[k][l]=0.0;
	  for(m=0;m<3;m++)
	    M[k][l] += (B[k][m]*AInv[m][l]);
	}
	
      /* From Rotational KMMATRIX Get Inst. Axis and angle */
      d= sqrt(SQR((M[2][1] - M[1][2])) + 
              SQR((M[0][2] - M[2][0])) +
	      SQR((M[1][0] - M[0][1])) );
      
      sin_val=d/2;
      cos_val=(M[0][0] + M[1][1] + M[2][2] -1)/2;
      if (d!=0.0000) { /* any angle other than 0 or 180 deg */
        seq[j][i].inst_axis[0]=(M[2][1] - M[1][2])/d;
        seq[j][i].inst_axis[1]=(M[0][2] - M[2][0])/d;
        seq[j][i].inst_axis[2]=(M[1][0] - M[0][1])/d;
      }
      else if (cos_val==1.0) { /* 0 deg */
        seq[j][i].inst_axis[0]=seq[j][i].inst_axis[1]=0.0;
        seq[j][i].inst_axis[2]=1.0;
        seq[j][i].rot=0.0;
      }
      else {  /* 108 deg */
	seq[j][i].inst_axis[0]=sqrt((M[0][0]+1)/2);
	seq[j][i].inst_axis[1]=sqrt((M[1][1]+1)/2);
	seq[j][i].inst_axis[2]=sqrt((M[2][2]+1)/2);
	if (seq[j][i].inst_axis[0]*seq[j][i].inst_axis[1]*M[1][0] < 0 )
	  seq[j][i].inst_axis[1]= -seq[j][i].inst_axis[1];
	if (seq[j][i].inst_axis[1]*seq[j][i].inst_axis[2]*M[2][1] < 0 ) 
	  seq[j][i].inst_axis[2]= -seq[j][i].inst_axis[2];
      }
      seq[j][i].rot = atan2(sin_val,cos_val);                     
      if (seq[j][i].rot > PI) seq[j][i].rot = PI - seq[j][i].rot; 

      /*Get Translation vector */
      for(k=0;k<3;k++)
	seq[j][i].trans[k]= seq[j][i+1].cent[k]-seq[j][i].cent[k];
      DoTransform(seq[j][i+1].M,seq[j][i].M,
		  seq[j][i].cent,seq[j][i].inst_axis,
		  seq[j][i].rot,seq[j][i].trans);
    }
  
  for(j=0;j<num_objs;j++) {
    seq[j][time_inst-1].inst_axis[0]=1.0;
    seq[j][time_inst-1].inst_axis[1]=0.0;
    seq[j][time_inst-1].inst_axis[2]=0.0;
    seq[j][time_inst-1].rot=0.0;
    seq[j][time_inst-1].trans[0]=0.0;  
    seq[j][time_inst-1].trans[1]=0.0;  
    seq[j][time_inst-1].trans[2]=0.0;
  }
}


void DoTransform(KMMATRIX mtNew, KMMATRIX prev, KMPOINT cent, KMPOINT axis,double angle, KMPOINT translate)       
{
  
  
  KMMATRIX t1,t2,t3;
  double a,b,c,r1,r2,r3;
  
  /* Unitize axis */
  a= sqrt( SQR(axis[0]) + SQR(axis[1]) + SQR(axis[2]) );
  if (a>0) {
    axis[0] /= a;
    axis[1] /= a;
    axis[2] /= a;
  }
  
  a=axis[0];
  b=axis[1];
  c=axis[2];
  
  r1=sin(angle);
  r2=cos(angle);
  r3=1-r2;
  
  /* rotational KMMATRIX around an axis a,b,c through the origin by the angle */
  t1[0][0]=a*a + (1- a*a)*r2;
  t1[0][1]=a*b*r3 - c*r1;
  t1[0][2]=a*c*r3 + b*r1;
  t1[1][0]=a*b*r3 + c*r1;
  t1[1][1]=b*b + (1 - b*b ) *r2;
  t1[1][2]=b*c*r3 - a*r1;
  t1[2][0]=a*c*r3 - b*r1;
  t1[2][1]=b*c*r3 + a*r1;
  t1[2][2]=c*c + (1 - c*c)*r2;
  t1[3][0]=t1[3][1]=t1[3][2]=t1[0][3]=t1[1][3]=t1[2][3]=0;
  t1[3][3]=1;
  
  MakeID(t2);
  /* bring axis to the orgin */
  t2[0][3]= -cent[0]; t2[1][3]= -cent[1]; t2[2][3]= -cent[2];
  
  /* do rotation */
  MultMat(t3,t1,t2);
  
  /* translate back to the point and do the given translation */
  MakeID(t2);
  t2[0][3]= cent[0] + translate[0];
  t2[1][3]= cent[1] + translate[1]; 
  t2[2][3]= cent[2] + translate[2];
  
  MultMat(t1,t2,t3);
  
  /* add previous tranlation to the front of transformations */
  MultMat(mtNew,t1,prev);
  
}


void SortEval(double e[3] )
{
  int i,j;
  double temp;

  for(i=0;i<2;i++)
    for(j=i+1;j<3;j++)
      if (e[i]<e[j])
	  {
	temp=e[j]; e[j]=e[i]; e[i]=temp;
      }
}

void GtEval( double mat[3][3],double eval[3] )
{       
  double  p,q,r,a,b,m,theta,delta;
  
  p = -mat[0][0]-mat[1][1]-mat[2][2];
  q  = -mat[0][1]*mat[1][0]-mat[0][2]*mat[2][0]-mat[1][2]*mat[2][1]
                +mat[0][0]*mat[1][1]+mat[0][0]*mat[2][2]+mat[1][1]*mat[2][2];
  r = -Det3(mat);

  a= q - p*p/3.0; b= p*p*p/13.5 - p*q/3.0 + r;
  m= 2.0*sqrt(a/-3.0);
  delta=b*b/4.0+a*a*a/27.0;
  if (delta >0.0 || a*m==0)
  {
	theta = M_PI/3.0;
	m = fabs(b);
  }
  else
    theta=(acos((3.0*b)/(a*m)))/3.0;
  eval[0]= p/-3.0 + m*cos(theta);
  eval[1]= p/-3.0 + m*cos(theta+ 2.0*M_PI/3.0);
  eval[2]= p/-3.0 + m*cos(theta+ 4.0*M_PI/3.0);

} 

/*------       Computation of the Eigenvectors of a 3 by 3 matrix       ------*/
int GtEvec(double mat[3][3], double eval[3], double evec[3][3])
{ 
  int     i,j, vn[3], cm1, cm2;
  double  vec0[3],vec1[3],vec2[3],outvec[3], mm[3];

  if (eval[0] == eval[2])
  {
    for (j=0; j<3; j++)
      for (i=0; i<3; i++)
        evec[j][i] = i==j;
    return(0);
  }

  if (eval[1]-eval[2] > eval[0]-eval[1])
  {
    vn[0] = 2;
	vn[1] = 0;
	vn[2] = 1;
  }
  else
  {
    vn[0] = 0;
	vn[1] = 1;
	vn[2] = 2;
  }
  for (i=0; i<3; i++)
  {
    vec0[i] = (i==0) ? mat[0][i]-eval[vn[0]] : mat[0][i];
    vec1[i] = (i==1) ? mat[1][i]-eval[vn[0]] : mat[1][i];
	vec2[i] = (i==2) ? mat[2][i]-eval[vn[0]] : mat[2][i];
  }
  mm[0] = vec0[0]*vec0[0]+vec0[1]*vec0[1]+vec0[2]*vec0[2];
  mm[1] = vec1[0]*vec1[0]+vec1[1]*vec1[1]+vec1[2]*vec1[2];
  mm[2] = vec2[0]*vec2[0]+vec2[1]*vec2[1]+vec2[2]*vec2[2];
  cm1 = mm[0]<mm[1]? 0:1;
  if (mm[2] < mm[cm1])
    cm1 = 2;
  switch (cm1)
  {
    case 0:
	  break;
	case 1:
	  memcpy(vec1, vec0, sizeof(vec0));
	  mm[1] = mm[0];
	  break;
	case 2:
	  memcpy(vec2, vec0, sizeof(vec0));
	  mm[2] = mm[0];
	  break;
  }
  if (CrossP(vec1, vec2, outvec) != 0) {
    fprintf(stderr, "Cannot compute Eigenvectors\n");
	exit(-1);
  }
  for (i=0; i<3; i++)
    evec[vn[0]][i] = outvec[i];
  // One Eigenvector has been computed.

  if (eval[vn[1]] == eval[vn[2]])
  {
    if (CrossP(outvec, vec2, vec1) != 0) {
	  fprintf(stderr, "Cannot compute Eigenvectors\n");
	  exit(-1);
	}
	for (i=0; i<3; i++)
	  evec[vn[1]][i] = vec1[i];
  }
  else
  {
	int nm1=cm1==0?1:0, nm2=cm1==2?!nm1:2;
    for (i=0; i<3; i++)
	{
	  vec1[i] = (i==nm1) ? mat[nm1][i]-eval[vn[1]] : mat[nm1][i];
	  vec2[i] = (i==nm2) ? mat[nm2][i]-eval[vn[1]] : mat[nm2][i];
	}
	mm[1] = vec1[0]*vec1[0]+vec1[1]*vec1[1]+vec1[2]*vec1[2];
	mm[2] = vec2[0]*vec2[0]+vec2[1]*vec2[1]+vec2[2]*vec2[2];
	cm2 = mm[1]<mm[2]? nm1:nm2;
	if (cm2 == nm2)
	  memcpy(vec2, vec1, sizeof(vec0));
	for (i=0; i<3; i++)
	{
	  vec1[i] = evec[vn[0]][i];
	}
	if (CrossP(vec1, vec2, outvec) != 0) {
	  fprintf(stderr, "Cannot compute Eigenvectors\n");
	  exit(-1);
	}
	for (i=0; i<3; i++)
	  evec[vn[1]][i] = outvec[i];
  }
  // Two Eigenvectors have been computed.

  /* calculate the third Eigenvector as a cross product of the first ones */
  CrossP(evec[vn[0]], evec[vn[1]], outvec);
  for (i=0; i<3; i++)
    evec[vn[2]][i] = outvec[i];
  return(0);
}

int CrossP(double vec1[], double vec2[], double res[] )
{
  double len;

  res[0] = vec1[1]*vec2[2]-vec1[2]*vec2[1];
  res[1] = vec1[2]*vec2[0]-vec1[0]*vec2[2];
  res[2] = vec1[0]*vec2[1]-vec1[1]*vec2[0];
  len = sqrt(pow(res[0],2.)+pow(res[1],2.)+pow(res[2],2.));
  if (len <= 0.0)
    return(-1);
  else {
    res[0] = res[0]/len;
    res[1] = res[1]/len;
    res[2] = res[2]/len;
  }
  return(0);
}       


#ifndef SCRIPTABLE_VERSION

/* Modified: 6/25/98 axis angles reported instead of direction cosines
 *    by Dewey Odhner. */
void DisplayListing(Surf_Info *s, int obj,int inst, wxArrayString*  pstrOutPut)
{  
  char msg[2][100];
  int i; 
  double rot;  

  CURRENT_SURF = 0;
 sprintf(msg[0],"%s",s->file_name);
  pstrOutPut->Add(msg[0]);
 
  sprintf(msg[0],"(%d)", obj+1); //obj_real
 
  //strcpy(pchDisplayMsg+100, msg[0]);
  pstrOutPut->Add(msg[0]);
  
 for(i=inst; i<time_inst-1; i++) 
  {
    sprintf(msg[0],"%d to %d",i+1,i+2);
	pstrOutPut->Add(msg[0]);
    sprintf(msg[0],"%-3.5lf  %-3.5lf  %-3.5lf", seq[obj][i].trans[0],seq[obj][i].trans[1],seq[obj][i].trans[2]);
	pstrOutPut->Add(msg[0]);
    sprintf(msg[0],"%-3.5lf  %-3.5lf  %-3.5lf", seq[obj][i].cent[0],seq[obj][i].cent[1],seq[obj][i].cent[2]);
	pstrOutPut->Add(msg[0]);
    sprintf(msg[0],"%-3.5lf  %-3.5lf  %-3.5lf",
	    180.0/PI*acos(seq[obj][i].inst_axis[0]),
		180.0/PI*acos(seq[obj][i].inst_axis[1]),
		180.0/PI*acos(seq[obj][i].inst_axis[2]));
	pstrOutPut->Add(msg[0]);
    rot= seq[obj][i].rot*180.0/PI;
    sprintf(msg[0],"%-3.5lf",rot);	    
	pstrOutPut->Add(msg[0]);
  } 

}
#endif


/************************************************************************
 *
 *      FUNCTION        : ReleaseData
 *
 *      DESCRIPTION     : Given a structure system and a structure of
 *                        interest check if the data is loaded and
 *                        if so free up the space.
 *
 *      RETURN VALUE    : 0 - freed data. 1 - data already freed
 *
 *      PARAMETERS      : sf - current structure system index
 *                        st - current strucutre index
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : surf should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int ReleaseData(int sf,int st)     
{  
  if (!surf[sf].surf_struct[st].DATA_LOADED) 
	  return(1);
  free(surf[sf].surf_struct[st].node[0][0]);
  surf[sf].surf_struct[st].DATA_LOADED=0;
  return(0);
}

     
void Inv3Mat(  double in[3][3],double out[3][3] )
{
  double t;

  t=Det3(in);
  if (t) {
    out[0][0]=(in[1][1]*in[2][2]-in[1][2]*in[2][1])/t;
    out[0][1]=(in[0][2]*in[2][1]-in[0][1]*in[2][2])/t;
    out[0][2]=(in[0][1]*in[1][2]-in[0][2]*in[1][1])/t;
    out[1][0]=(in[1][2]*in[2][0]-in[1][0]*in[2][2])/t;
    out[1][1]=(in[0][0]*in[2][2]-in[0][2]*in[2][0])/t;
    out[1][2]=(in[0][2]*in[1][0]-in[0][0]*in[1][2])/t;
    out[2][0]=(in[1][0]*in[2][1]-in[1][1]*in[2][0])/t;
    out[2][1]=(in[0][1]*in[2][0]-in[0][0]*in[2][1])/t;
    out[2][2]=(in[0][0]*in[1][1]-in[0][1]*in[1][0])/t;
  } 
  else printf("Cannot find inverse 3\n");
}


//Register_Inter()
int Register_Inter(Surf_Info *InSurf, wxArrayString*  pstrOutPut, int *nInstancesNum, int num_surf)
{
	Surf_Tree *ptr,*ptr1;
	int i,j,error,level,obj;
	short *order;
	KMMATRIX M;

	surf = InSurf;

	if (num_surf<2) 
	{
		wxLogMessage("You have to specify at least 2 files");
		return(-1);
	}
	level=surf[0].tree->level;
	for(i=0;i<num_surf;i++)
	{
		if (surf[i].tree->level!=level) 
		{
			wxLogMessage("Files Do not have the same tree structure");
			return(-1);
		}
	}

	for(CURRENT_SURF=0;CURRENT_SURF<num_surf;CURRENT_SURF++)
		for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
			surf[CURRENT_SURF].tree_index[i]=0;

	/* Check for consistancy in the tree structure and the number of objects */
	num_objs=0;
	ptr=GetSurfPointer(&surf[0],surf[0].tree_index,M);
	while (ptr!=NULL) 
	{
		for(CURRENT_SURF=1;CURRENT_SURF<num_surf;CURRENT_SURF++) 
		{
			if ((ptr1=GetSurfPointer(&surf[CURRENT_SURF],surf[0].tree_index,M))==NULL)
			{
				wxLogMessage("Files Do not have the same tree structure");
				return(-1);
			}
			if (ptr1->children!=ptr->children) 
			{
				wxLogMessage("Files Do not have the same tree structure");
				return(-1);
			}
		}
		num_objs += ptr->children;
		error=GetNextStructureIndex(&surf[0],surf[0].tree_index);
		if (error)
			ptr=NULL;
		else
			ptr=GetSurfPointer(&surf[0],surf[0].tree_index,M);
	}

	if (num_objs < 1) return(-1);
#ifdef SCRIPTABLE_VERSION

	if (rel_flag>=0) {
		if (rel_flag > num_objs-1) 
			DisplayMessage("Incorrect Relative flag. There aren't that many objects");
		else
			REL_OBJECT=rel_flag+1;
	}
	else 
		REL_OBJECT=0;

	if (instance>1)
		out_time_inst=instance;
	else
		out_time_inst=num_surf;

#endif
	seq=(REG_SEQ **)malloc(sizeof(REG_SEQ *)*num_objs);
	if (seq==NULL) {
		wxLogMessage("Memory allocation error");
		return(-1);
	}

	out_seq=(REG_SEQ **)malloc(sizeof(REG_SEQ *)*num_objs);
	if (out_seq==NULL) {
		wxLogMessage("Memory allocation error");
		return(-1);
	}

	time_inst=num_surf;
	for(i=0;i<num_objs;i++)
	{
		if ((seq[i]=(REG_SEQ *)malloc(sizeof(REG_SEQ)*time_inst))==NULL) 
		{
			wxLogMessage("Could Not Allocate Memory");
			for(j=0;j<i;j++) free(seq[j]);
			free(seq);
			return(-1);
		}
	}

		order=(short *)malloc(sizeof(short)*time_inst);
		for(i=0;i<num_surf;i++)
			order[i]=i;
		/*
		GetFileOrder(order); 
		*/

		for(i=0;i<time_inst;i++) 
		{
			CURRENT_SURF=order[i];
			for(j=0;j<surf[i].tree->level-1;j++)
				surf[CURRENT_SURF].tree_index[j]=0;

			ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
			obj=0;
			while (ptr!=NULL) 
			{
				for(j=0;j<ptr->children;j++) 
				{
					Load_Data(CURRENT_SURF,(int)(ptr->child[j]->surf_index));
				//	VSelectCursor(ALL_WINDOWS,XC_watch);
					wxLogMessage("Calcualting the axis system");
					InitInstance(&seq[obj][CURRENT_SURF],CURRENT_SURF,(int)(ptr->child[j]->surf_index),
						ptr->child[j]->transf);
				//	VSelectCursor(ALL_WINDOWS,DEFAULT_CURSOR);
					obj++;
				}
				error=GetNextStructureIndex(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index);
				if (error) 
					ptr=NULL;
				else
					ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
			}
		}


		/* The Eigen analysis does not resolve the direction of the axis calculated         */
		/* Thus to make sure axis in each time instance is consistant sign of the direction */
		/* is flipped so as to give the smallest possible angular change between instances  */
		MakeAxisConsistant(); 

		GetRotationAndTranslation();

		CURRENT_SURF=order[0];
		for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
			surf[CURRENT_SURF].tree_index[i]=0;  
#ifdef SCRIPTABLE_VERSION

		if (SaveInterPlan(outfile)!=0) 
			return(-1);

#else
		CURRENT_REAL_OBJ=0;
		CURRENT_OBJ=0;
		CURRENT_INST=0;

		*nInstancesNum = time_inst;
		wxLogMessage("Displaying List. Wait...");
		DisplayInterListing(&surf[CURRENT_SURF],CURRENT_OBJ,CURRENT_REAL_OBJ,CURRENT_INST, pstrOutPut );
		wxLogMessage("Done Displaying List.");

		//Inter_EventHandler();
		//VClearWindow(img,0,0,0,0);
#endif	

		return(0);

}


void KinematicsInterRelease()
{
	/*Surf_Tree *ptr;
	KMMATRIX M;
	int error;

	short *order=(short *)malloc(sizeof(short)*time_inst);
	for(i=0;i<time_inst;i++)
		order[i]=i;

	for(i=0;i<time_inst-1;i++) 
	{
		CURRENT_SURF=order[i];
		for(j=0;j<surf[i].tree->level-1;j++)
			surf[CURRENT_SURF].tree_index[j]=0;
		ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
		while (ptr!=NULL) {
			for(j=0;j<ptr->children;j++)
				ReleaseData(CURRENT_SURF,(int)ptr->child[j]->surf_index);
			error=GetNextStructureIndex(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index);
			if (error) 
				ptr=NULL;
			else
				ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
		}
	}
	free(order);*/

	surf = NULL;
	/*for(i=0;i<num_objs;i++)
		free(seq[i]);
	free(seq);
	free(out_seq);*/
}

/* Modified: 6/25/98 axis angles reported instead of direction cosines
 *    by Dewey Odhner. */
void DisplayInterListing(Surf_Info *s, int obj, int obj_real, int inst, wxArrayString*  pstrOutPut )
{  
  char msg[2][100];
  int i;  
  double rot;

  CURRENT_SURF = 0;

  sprintf(msg[0],"%s",s->file_name);
  pstrOutPut->Add(msg[0]);
  strcpy(msg[0],"(");
  for(i=0;i<s->tree->level-1;i++) 
  {
    sprintf(msg[1],"%d,",surf[CURRENT_SURF].tree_index[i]+1);
    strcat(msg[0],msg[1]);
  }
  sprintf(msg[1],"%d)",obj_real+1);
  strcat(msg[0],msg[1]);
  //strcpy(pchDisplayMsg+100, msg[0]);
  pstrOutPut->Add(msg[0]);


  for(i=inst; i<time_inst-1; i++) 
  {
    sprintf(msg[0],"%d to %d",i+1,i+2);
	pstrOutPut->Add(msg[0]);
    sprintf(msg[0],"%-3.5lf  %-3.5lf  %-3.5lf", seq[obj][i].trans[0],seq[obj][i].trans[1],seq[obj][i].trans[2]);
	pstrOutPut->Add(msg[0]);
    sprintf(msg[0],"%-3.5lf  %-3.5lf  %-3.5lf", seq[obj][i].cent[0],seq[obj][i].cent[1],seq[obj][i].cent[2]);
	pstrOutPut->Add(msg[0]);
    sprintf(msg[0],"%-3.5lf  %-3.5lf  %-3.5lf",
	    180.0/PI*acos(seq[obj][i].inst_axis[0]),
		180.0/PI*acos(seq[obj][i].inst_axis[1]),
		180.0/PI*acos(seq[obj][i].inst_axis[2]));
	pstrOutPut->Add(msg[0]);
    rot= seq[obj][i].rot*180.0/PI;
    sprintf(msg[0],"%-3.5lf",rot);	    
	pstrOutPut->Add(msg[0]);
  } 
}


int SaveInterPlan(Surf_Info *InSurf, char *outfile, int timeInstances, int nRefFrame)
{

  int i,j, k[3], n,obj, m;
  FILE *fp;
  Surf_Tree *ptr;

  KMMATRIX M,InvM;
  KMPOINT cent,axis[4], rel_cent[3], t_axis;
  REG_SEQ *rel_seq;
  static KMPOINT zero={0, 0, 0};
  double a, len1, axis_length[3], volume;  
  int mode = 0; 
  int error;

  surf = InSurf;
  CURRENT_SURF = 0;

  if( NULL != strstr(outfile, "PLN") || NULL != strstr(outfile, "pln") )
	  mode = 0;
  else  if( NULL != strstr(outfile, "TXT") || NULL != strstr(outfile, "txt") )
	  mode = 1;
  else  if( NULL != strstr(outfile, "PAR") || NULL != strstr(outfile, "par") )
	  mode = 2;
  else  if( NULL != strstr(outfile, "TRN") || NULL != strstr(outfile, "trn") )
	  mode = 3;
  else
	  mode = 0;

//#ifdef SCRIPTABLE_VERSION
//  ind=rindex(outfile,'.');
//  if (!strcmp(ind,".PLN"))
//    mode = 0;
//  else if (!strcmp(ind,".TXT"))
//    mode = 1;
//  else if (!strcmp(ind,".PAR"))
//    mode = 2;
//#else
//  VGetSaveSwitchValue(&mode);
//#endif
  out_time_inst = timeInstances;

  switch (mode) {
   case 0:
#ifndef SCRIPTABLE_VERSION
    wxLogMessage("Writing Plan. Wait....");
#endif
    fp=fopen(outfile,"w");
    if (fp==NULL) {
      wxLogMessage("Could not Open Plan for writing ....");      
    }
    fprintf(fp,"%s\n\n",surf[CURRENT_SURF].file_name);
    fprintf(fp,"AXIS %d {\n",num_objs);
    for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
      surf[CURRENT_SURF].tree_index[i]=0;

    ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
    for(i=0;i<ptr->children;i++) { 
      Inv4Mat(ptr->child[i]->transf,InvM);
      TransformPoint(cent,InvM,seq[i][0].cent);
      TransformVector(axis[0],InvM,seq[i][0].axis[0]);
      TransformVector(axis[1],InvM,seq[i][0].axis[1]);
      TransformVector(axis[2],InvM,seq[i][0].axis[2]);
      
      fprintf(fp,"%d { %lf %lf %lf } { %lf %lf %lf } { %lf %lf %lf } { %lf %lf %lf }\n",
	      ptr->child[i]->surf_index,
	      cent[0], cent[1], cent[2],
	      axis[0][0],axis[0][1],axis[0][2],
	      axis[1][0],axis[1][1],axis[1][2],
	      axis[2][0],axis[2][1],axis[2][2]);    
      
    }
    fprintf(fp,"}\n\n");
    
    
    fprintf(fp,"TREE 3 {\n");
    fprintf(fp,"3 %d ID\n",out_time_inst);
    for(i=0;i<out_time_inst;i++) 
      fprintf(fp,"2 %d ID\n",num_objs);
    
    if (ComputeOutSequence()!=-1) {

      for(i=0;i<out_time_inst;i++) {
        for(j=0;j<surf[CURRENT_SURF].tree->level-1;j++)
          surf[CURRENT_SURF].tree_index[j]=0;      
        ptr =
          GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
        obj=0;
        while (obj < num_objs) {
          for(j=0;j<ptr->children;j++) {
            fprintf(fp,"1 %d\n",ptr->child[j]->surf_index);
            print4mat(out_seq[obj][i].M,fp);
            obj++;
          }
          error = GetNextStructureIndex(&surf[CURRENT_SURF],
            surf[CURRENT_SURF].tree_index);
          if (!error || surf[CURRENT_SURF].tree_index==NULL)
            ptr=NULL;
          else 
            ptr = GetSurfPointer(&surf[CURRENT_SURF],
              surf[CURRENT_SURF].tree_index,M);
        }

      }
      ReleaseOutSequence();
      
    }
    else
	{ 
      wxLogMessage("Error in writing plan");
      fclose(fp);
      return(-1);      
    }
    
    fprintf(fp,"}\n");
    fclose(fp);
    wxLogMessage("Done writing plan.");
    break;

   case 1:
#ifndef SCRIPTABLE_VERSION
    wxLogMessage("Writing report file. Wait....");
#endif
    fp=fopen(outfile,"w");
    if (fp==NULL) {
      wxLogMessage("Could not Open TXT for writing ....");      
    }   
    rel_seq=(REG_SEQ *)malloc(sizeof(REG_SEQ)*time_inst);
    if (rel_seq==NULL) {
      wxLogMessage("Memory allocation error");      
    }   

	fprintf(fp, "PATIENT-RELATED INFORMATION\n\n");
	fprintf(fp, "Study # : %s\n", surf[CURRENT_SURF].vh.gen.study_valid?
	  surf[CURRENT_SURF].vh.gen.study: "not given.");
	fprintf(fp, "Name : %s\n", surf[CURRENT_SURF].vh.gen.patient_name_valid?
	  surf[CURRENT_SURF].vh.gen.patient_name:"not given.");

	fprintf(fp, "MORPHOLOGY-RELATED INFORMATION\n\n");
    for(i=0; i<num_objs; i++) {
	  fprintf(fp, "Object_number_%d:\n", i+1);
	  for (j=0; j<time_inst; j++)
	  {
	    fprintf(fp, "  Time_instance %d\n", j+1);
	    fprintf(fp, "   Centroid_coordinates %0.4lf %0.4lf %0.4lf\n",
	      seq[i][j].cent[0], seq[i][j].cent[1], seq[i][j].cent[2]);
	    fprintf(fp, "   Major_axis_vector %0.6lf %0.6lf %0.6lf\n",
	      seq[i][j].axis[0][0], seq[i][j].axis[0][1], seq[i][j].axis[0][2]);
	    fprintf(fp, "   2nd_principal_axis_vector %0.6lf %0.6lf %0.6lf\n",
	      seq[i][j].axis[1][0], seq[i][j].axis[1][1], seq[i][j].axis[1][2]);
	    fprintf(fp, "   3rd_principal_axis_vector %0.6lf %0.6lf %0.6lf\n",
	      seq[i][j].axis[2][0], seq[i][j].axis[2][1], seq[i][j].axis[2][2]);
        ptr = GetSurfPointer(&surf[j], surf[j].tree_index, M );
	    for (m=0; m<3; m++)
	      axis_length[m] = get_axis_length(seq[i]+j,
			&surf[j].surf_struct[ptr->child[i]->surf_index], m,
			 ptr->child[i]->transf);
	    fprintf(fp, "   Eigenvalues %f %f %f\n", seq[i][j].eval[0],
	      seq[i][j].eval[1], seq[i][j].eval[2]);
	    volume = get_volume(&surf[j].surf_struct[ptr->child[i]->surf_index]);
	    volume *= surf[j].Px*surf[j].Py*surf[j].Pz;
	    fprintf(fp, "   Volume %0.4lf\n", volume);
		fprintf(fp, "   Cube_root_volume %f\n", pow(volume, 1/3.));
		fprintf(fp, "   Bounding_box_diagonal %f\n", sqrt(
		  (surf[j].max_x-surf[j].min_x)*(surf[j].max_x-surf[j].min_x)+
		  (surf[j].max_y-surf[j].min_y)*(surf[j].max_y-surf[j].min_y)+
		  (surf[j].max_z-surf[j].min_z)*(surf[j].max_z-surf[j].min_z)));
	    fprintf(fp, "   Root_lambda_1 %f\n", sqrt(seq[i][j].eval[0]));
	    if (surf[j].vh.str.surface_area_valid)
	      fprintf(fp, "   Surface_area %f\n",
		    surf[j].vh.str.surface_area[ptr->child[i]->surf_index]);
	    else
	      fprintf(fp, "   Surface_area not_given.\n");
	    fprintf(fp, "   Axis_lengths %0.4lf %0.4lf %0.4lf\n", axis_length[0],
	      axis_length[1], axis_length[2]);
	    len1 = axis_length[0]*axis_length[1]*axis_length[2];
	    if (len1 > 0)
	      fprintf(fp, "   Volume_fraction_of_axis_box %0.4lf\n", volume/len1);
	    else
		  fprintf(fp, "   Volume_of_axis_box_is zero!\n");
	    len1 = 2*(axis_length[0]*axis_length[1]+axis_length[0]*axis_length[2]+
	      axis_length[1]*axis_length[2]);
	    if (len1 <= 0)
	      fprintf(fp, "   Area_of_axis_box_is zero!\n");
	    else if (surf[j].vh.str.surface_area_valid)
	      fprintf(fp, "   Area_fraction_of_axis_box %0.4lf\n",
		    surf[j].vh.str.surface_area[ptr->child[i]->surf_index]/len1);
	  }
	  fprintf(fp, "\n");
    }

	fprintf(fp, "ARCHITECTURE-RELATED INFORMATION\n\n");

    if (ComputeOutSequence()!=-1) {
      fprintf(fp, "DISTANCE BETWEEN CENTROIDS\n\n");
	  for (k[1]=1; k[1]<num_objs; k[1]++)
	    for (k[0]=0; k[0]<k[1]; k[0]++)
		{
		  fprintf(fp, "  Objects %d--%d:\n", k[0]+1, k[1]+1);
		  fprintf(fp, "  time distance\n");
		  for (i=0; i<out_time_inst; i++)
		  {
		    TransformPoint(rel_cent[0], out_seq[k[0]][i].M, seq[k[0]][0].cent);
			TransformPoint(rel_cent[1], out_seq[k[1]][i].M, seq[k[1]][0].cent);
			fprintf(fp, "%6d %f\n", i+1, sqrt(
			  (rel_cent[0][0]-rel_cent[1][0])*(rel_cent[0][0]-rel_cent[1][0])+
			  (rel_cent[0][1]-rel_cent[1][1])*(rel_cent[0][1]-rel_cent[1][1])+
			  (rel_cent[0][2]-rel_cent[1][2])*(rel_cent[0][2]-rel_cent[1][2]))
			  );
		  }
		}
      fprintf(fp, "\nANGLE BETWEEN MAJOR AXES\n");
	  for (k[1]=1; k[1]<num_objs; k[1]++)
	    for (k[0]=0; k[0]<k[1]; k[0]++)
		{
		  fprintf(fp, "  Object %d--%d angle\n", k[0]+1, k[1]+1);
		  fprintf(fp, "  time degrees\n");
		  for (i=0; i<out_time_inst; i++)
		  {
            TransformVector(axis[0], out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
            TransformVector(axis[1], out_seq[k[1]][i].M, seq[k[1]][0].axis[0]);
			fprintf(fp, "%6d %f\n", i+1, angle(axis[0], zero, axis[1]));
		  }
		}
      fprintf(fp, "\nANGLE BETWEEN CENTROIDS\n");
	  for (k[2]=2; k[2]<num_objs; k[2]++)
	    for (k[1]=1; k[1]<k[2]; k[1]++)
		  for (k[0]=0; k[0]<k[1]; k[0]++)
		    for (m=0; m<3; m++)
			{
		      fprintf(fp, "  Object %d--Object %d--Object %d angle\n",
			    k[m]+1, k[(m+1)%3]+1, k[(m+2)%3]+1);
			  fprintf(fp, "  time degrees\n");
			  for (i=0; i<out_time_inst; i++) {
			    TransformPoint(rel_cent[0], out_seq[k[m]][i].M,
				  seq[k[m]][0].cent);
			    TransformPoint(rel_cent[1], out_seq[k[(m+1)%3]][i].M,
				  seq[k[(m+1)%3]][0].cent);
			    TransformPoint(rel_cent[2], out_seq[k[(m+2)%3]][i].M,
				  seq[k[(m+2)%3]][0].cent);
			    fprintf(fp, "%6d %f\n", i+1,
				  angle(rel_cent[0], rel_cent[1], rel_cent[2]));
		      }
		    }
      ReleaseOutSequence();
    }
    else {
      wxLogMessage("Error in writing parameters");
      fclose(fp);
      return(-1);
      
    }

	fprintf(fp, "\nKINEMATICS-RELATED INFORMATION\n\n");
    j = REL_OBJECT;
	for (REL_OBJECT=0; REL_OBJECT<=num_objs; REL_OBJECT++)
	{
	  if (REL_OBJECT == 0)
	    fprintf(fp, "MOTION RELATIVE TO SCANNER\n\n");
	  else
	    fprintf(fp, "MOTION RELATIVE TO OBJECT %d\n\n", REL_OBJECT);
	  for(i=0;i<num_objs;i++) {
	    if (i+1 == REL_OBJECT)
		  continue;
        fprintf(fp, "Motion of object %d\n", i+1);
		Compute_Relative(i,rel_seq);
        Print_Txt(fp,i,rel_seq);
      }
	}
	REL_OBJECT = j;

    fclose(fp);
    free(rel_seq);
    wxLogMessage("Done writing text file.");
	break;

   case 2:
#ifndef SCRIPTABLE_VERSION
    wxLogMessage("Writing parameters. Wait....");   
#endif
    fp=fopen(outfile,"w");
    if (fp==NULL) {
      wxLogMessage("Could not open file for writing parameters.");      
      return(-1);
    }
    fprintf(fp,"%s\n\n",surf[CURRENT_SURF].file_name);
    
    for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
      surf[CURRENT_SURF].tree_index[i]=0;

    if (ComputeOutSequence()!=-1) {
      
      fprintf(fp, "Angle between centroids\n");
	  for (k[2]=2; k[2]<num_objs; k[2]++)
	    for (k[1]=1; k[1]<k[2]; k[1]++)
		  for (k[0]=0; k[0]<k[1]; k[0]++)
		    for (n=0; n<3; n++)
			{
		      fprintf(fp, "\nObjects %d--%d--%d:\ntime degrees\n",
			    k[n]+1, k[(n+1)%3]+1, k[(n+2)%3]+1);
			  for (i=0; i<out_time_inst; i++) {
			    TransformPoint(rel_cent[0], out_seq[k[n]][i].M,
				  seq[k[n]][0].cent);
			    TransformPoint(rel_cent[1], out_seq[k[(n+1)%3]][i].M,
				  seq[k[(n+1)%3]][0].cent);
			    TransformPoint(rel_cent[2], out_seq[k[(n+2)%3]][i].M,
				  seq[k[(n+2)%3]][0].cent);
			    fprintf(fp, "%d %f\n", i+1,
				  angle(rel_cent[0], rel_cent[1], rel_cent[2]));
		      }
		    }
      fprintf(fp, "\n\n\nDistance between centroids\n");
	  for (k[1]=1; k[1]<num_objs; k[1]++)
	    for (k[0]=0; k[0]<k[1]; k[0]++)
		{
		  fprintf(fp, "\nObjects %d--%d:\ntime distance\n", k[0]+1, k[1]+1);
		  for (i=0; i<out_time_inst; i++)
		  {
		    TransformPoint(rel_cent[0], out_seq[k[0]][i].M, seq[k[0]][0].cent);
			TransformPoint(rel_cent[1], out_seq[k[1]][i].M, seq[k[1]][0].cent);
			fprintf(fp, "%d %f\n", i+1, sqrt(
			  (rel_cent[0][0]-rel_cent[1][0])*(rel_cent[0][0]-rel_cent[1][0])+
			  (rel_cent[0][1]-rel_cent[1][1])*(rel_cent[0][1]-rel_cent[1][1])+
			  (rel_cent[0][2]-rel_cent[1][2])*(rel_cent[0][2]-rel_cent[1][2]))
			  );
		  }
		}
      fprintf(fp, "\n\n\nAngle between major axes\n");
	  for (k[1]=1; k[1]<num_objs; k[1]++)
	    for (k[0]=0; k[0]<k[1]; k[0]++)
		{
		  fprintf(fp, "\nObjects %d--%d:\ntime degrees\n", k[0]+1, k[1]+1);
		  for (i=0; i<out_time_inst; i++)
		  {
            TransformVector(axis[0], out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
            TransformVector(axis[1], out_seq[k[1]][i].M, seq[k[1]][0].axis[0]);
			fprintf(fp, "%d %f\n", i+1, angle(axis[0], zero, axis[1]));
		  }
		}
      ReleaseOutSequence();
    }
    else {
      wxLogMessage("Error in writing parameters");
      fclose(fp);
      return(-1);
      
    }    

    fclose(fp);
    wxLogMessage("Done writing parameters.");
	break;

#ifndef SCRIPTABLE_VERSION
   case 3:
    if (time_inst < 2)
      break;
    wxLogMessage("Writing turn angles. Wait....");   
    fp=fopen(outfile,"w");
    if (fp==NULL) {
      wxLogMessage("Could not open file for writing parameters.");      
      return(-1);
    }
    fprintf(fp,"%s\n\n",surf[CURRENT_SURF].file_name);
    for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
      surf[CURRENT_SURF].tree_index[i]=0;
    ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
   // VUndisplayScale("INSTANCES");
   // VSetScaleInformation("REF. FRAME", 0, 0, 1., (float)out_time_inst, 0, 0.5);
	ref_time = nRefFrame -1;
	/*val = ref_time+1;
	if (val > out_time_inst)
	  val = out_time_inst;*/
   // VDisplayScale(val, "REF. FRAME");
	//VDisplayDialogMessage("Select reference frame.");
	//VDisplayButtonAction("SELECT", "", "DONE");
	/*do
	{
      VNextEvent(&event);
	  if (VCheckScaleEvent(&event, "REF. FRAME", NULL, NULL, &val) == 0)
        ref_time = val-1;
	} while (event.type!=ButtonPress || event.xbutton.button!=Button3);
	*/
	if (ComputeOutSequence()!=-1) 
	{
      MultMat(M, out_seq[CURRENT_OBJ][ref_time].M, surf[CURRENT_SURF].
		surf_struct[ptr->child[CURRENT_OBJ]->surf_index].to_scanner);
	  Inv4Mat(M, InvM);
      fprintf(fp, "\n\nDirection vector of major axis\n");
	  for (k[0]=0; k[0]<num_objs; k[0]++)
		{
		  fprintf(fp, "\nObject %d:\ntime x-angle y-angle z-angle\n", k[0]+1);
		  for (i=0; i<out_time_inst; i++)
		  {
            TransformVector(t_axis, out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[0], InvM, t_axis);
			j = FALSE;
			if (i == 0)
			  j = axis[0][0]+axis[0][1]+axis[0][2]<0;
			a = 1/sqrt(axis[0][0]*axis[0][0]+axis[0][1]*axis[0][1]+
			  axis[0][2]*axis[0][2]);
			if (j)
			  a = -a;
			fprintf(fp, "%d %f %f %f\n", i+1, acos(axis[0][0]*a)*(180/PI),
			  acos(axis[0][1]*a)*(180/PI), acos(axis[0][2]*a)*(180/PI));
		  }
		}
      fprintf(fp, "\n\nAngle of major axis motion about x-axis\n");
	  for (k[0]=0; k[0]<num_objs; k[0]++)
		{
		  fprintf(fp, "\nObject %d:\ntime degrees\n", k[0]+1);
		  for (i=1; i<out_time_inst; i++)
		  {
            TransformVector(t_axis, out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[0], InvM, t_axis);
            TransformVector(t_axis, out_seq[k[0]][i-1].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[1], InvM, t_axis);
			axis[0][0] = 0;
			axis[1][0] = 0;
			a = angle(axis[0], zero, axis[1]);
			if (axis[0][1]*axis[1][2] < axis[0][2]*axis[1][1])
			  a = -a;
			fprintf(fp, "%d %f\n", i+1, a);
		  }
		}
      fprintf(fp, "\n\nAngle of major axis motion about y-axis\n");
	  for (k[0]=0; k[0]<num_objs; k[0]++)
		{
		  fprintf(fp, "\nObject %d:\ntime degrees\n", k[0]+1);
		  for (i=1; i<out_time_inst; i++)
		  {
            TransformVector(t_axis, out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[0], InvM, t_axis);
            TransformVector(t_axis, out_seq[k[0]][i-1].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[1], InvM, t_axis);
			axis[0][1] = 0;
			axis[1][1] = 0;
			a = angle(axis[0], zero, axis[1]);
			if (axis[0][2]*axis[1][0] < axis[0][0]*axis[1][2])
			  a = -a;
			fprintf(fp, "%d %f\n", i+1, a);
		  }
		}
      fprintf(fp, "\n\nAngle of major axis motion about z-axis\n");
	  for (k[0]=0; k[0]<num_objs; k[0]++)
		{
		  fprintf(fp, "\nObject %d:\ntime degrees\n", k[0]+1);
		  for (i=1; i<out_time_inst; i++)
		  {
            TransformVector(t_axis, out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[0], InvM, t_axis);
            TransformVector(t_axis, out_seq[k[0]][i-1].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[1], InvM, t_axis);
			axis[0][2] = 0;
			axis[1][2] = 0;
			a = angle(axis[0], zero, axis[1]);
			if (axis[0][0]*axis[1][1] < axis[0][1]*axis[1][0])
			  a = -a;
			fprintf(fp, "%d %f\n", i+1, a);
		  }
		}
      ReleaseOutSequence();
    }
    else {
      wxLogMessage("Error in writing parameters");
      fclose(fp);
      return(-1);
      
    }    
//    VUndisplayScale("REF. FRAME");
//    VDeleteScale("REF. FRAME");
 //   VDisplayScale((float)out_time_inst,"INSTANCES");
    fclose(fp);
    wxLogMessage("Done writing parameters.");
	break;
#endif
  }
  return 1;
}

int ComputeOutSequence()
{
  int i,j,nearest_loc;
  double time,dt,time_incr=0;
  KMPOINT trans;
  KMMATRIX CMat,IMat,TMat;


  for(i=0;i<num_objs;i++) 
  {
    if ((out_seq[i]=(REG_SEQ *)malloc(sizeof(REG_SEQ)*out_time_inst))==NULL)
	{
      wxLogMessage("Could Not Allocate Memory");
      for(j=0;j<i;j++) free(out_seq[j]);
      return(-1);
    }
  }

  if (out_time_inst > 1)
    time_incr= (double)(time_inst-1.0)/(out_time_inst-1.0);

  time=0.0;

  for(j=0;j<out_time_inst;j++) {
    nearest_loc=(int)floor(time);
    dt= time-nearest_loc;
    for(i=0;i<num_objs;i++) {
      trans[0]=seq[i][nearest_loc].trans[0]*dt;
      trans[1]=seq[i][nearest_loc].trans[1]*dt;
      trans[2]=seq[i][nearest_loc].trans[2]*dt;
      DoTransform(out_seq[i][j].M,seq[i][nearest_loc].M,seq[i][nearest_loc].cent,
		  seq[i][nearest_loc].inst_axis,seq[i][nearest_loc].rot*dt,
		  trans);
      
    }
    time += time_incr;
  }
  
  if (REL_OBJECT!=0) {
    for(i=0;i<4;i++) {
      CMat[i][0]=out_seq[REL_OBJECT-1][0].M[i][0];    CMat[i][1]=out_seq[REL_OBJECT-1][0].M[i][1];
      CMat[i][2]=out_seq[REL_OBJECT-1][0].M[i][2];    CMat[i][3]=out_seq[REL_OBJECT-1][0].M[i][3];
    }

    for(j=0;j<out_time_inst;j++) {
      Inv4Mat(out_seq[REL_OBJECT-1][j].M,IMat);
      for(i=0;i<num_objs;i++) {
	MultMat(TMat,IMat,out_seq[i][j].M);
	MultMat(out_seq[i][j].M,CMat,TMat);
      }
    }
  }

  return(0);

}


void ReleaseOutSequence()
{
  int i;

  for(i=0;i<num_objs;i++) 
    free(out_seq[i]);

}

/*****************************************************************************
 * FUNCTION: TransformVector
 * DESCRIPTION: Applies a transformation KMMATRIX to a vector.
 * PARAMETERS:
 *    new: The transformed vector is stored here.
 *    M: The transformation KMMATRIX
 *    old: The original vector
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/7/97 by Dewey Odhner
 *
 *****************************************************************************/
void TransformVector(KMPOINT kmNew, KMMATRIX M, KMPOINT old)
{
  kmNew[0]= M[0][0]*old[0]+M[0][1]*old[1]+M[0][2]*old[2];
  kmNew[1]= M[1][0]*old[0]+M[1][1]*old[1]+M[1][2]*old[2];
  kmNew[2]= M[2][0]*old[0]+M[2][1]*old[1]+M[2][2]*old[2];
}


void PrintLevel(int level, Surf_Tree *ptr, FILE *fp)
{
  int i;
  if (ptr->level==level)
    fprintf(fp,"%d %d ID\n",level+1,ptr->children);
  else if (ptr->level> level)
    for(i=0;i<ptr->children;i++)
      PrintLevel(level,ptr->child[i],fp);
}



void print4mat(KMMATRIX M,FILE *fp)
{
  int k,l;

  fprintf(fp,"\t{\n");
  for(k=0;k<4;k++) {
    fprintf(fp,"\t  ");
    for(l=0;l<4;l++) 
      fprintf(fp,"%-6.6lf ",M[k][l]);
    fprintf(fp,"\n");
  }
  fprintf(fp,"\t}\n");

}

/*****************************************************************************
 * FUNCTION: angle
 * DESCRIPTION: Returns the angle in degrees from three points.
 * PARAMETERS:
 *    A, C, B: Points on the angle, C is the vertex.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: the angle in degrees
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 12/9/92 by Dewey Odhner
 *
 *****************************************************************************/
double angle(double A[3], double C[3], double B[3] )
{
	double aa, bb, cc, a, b, ab, cosine;

	aa=(B[0]-C[0])*(B[0]-C[0])+(B[1]-C[1])*(B[1]-C[1])+(B[2]-C[2])*(B[2]-C[2]);
	bb=(A[0]-C[0])*(A[0]-C[0])+(A[1]-C[1])*(A[1]-C[1])+(A[2]-C[2])*(A[2]-C[2]);
	cc=(A[0]-B[0])*(A[0]-B[0])+(A[1]-B[1])*(A[1]-B[1])+(A[2]-B[2])*(A[2]-B[2]);
	a = sqrt(aa);
	b = sqrt(bb);
	ab = a*b;
	if (ab == 0)
		return (0);
	cosine = (aa+bb-cc)/(2*ab);
	return
	(	cosine>=1
		?	0
		:	cosine<=-1
			?	180
			:	acos(cosine)*(180/PI)
	);
}

/*****************************************************************************
 * FUNCTION: get_axis_length
 * DESCRIPTION: Finds the length of the longest segment of the principal axis
 *    within the object.
 * PARAMETERS:
 *    inst: Information about the object/time instance.  Must be initialized
 *       as with a call to InitInstance.
 *    str: Information about the structure.
 *    axis: Which axis to compute: 0, 1, or 2.
 *    transf: The rigid transformation applied to the object.
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variable display must be appropriately initialized.
 * RETURN VALUE: The length of the longest segment of the principal axis
 *    within the object.
 * EXIT CONDITIONS: If an error occurs, an error message will be displayed.
 * HISTORY:
 *    Created: 8/29/97 by Dewey Odhner
 *    Modified: 9/5/97 KMMATRIX initialization corrected by Dewey Odhner
 *    Modified: 9/24/97 KMMATRIX computation corrected by Dewey Odhner
 *    Modified: 10/25/02 computation corrected by Dewey Odhner
 *
 *****************************************************************************/
static double get_axis_length(REG_SEQ *inst, Surf_Struct *str, int axis, KMMATRIX transf )
{
	int j, k, slice, row, m, face_axis, order[3], start[3], finish[3], tp;
	Cord_with_Norm *ptr;
	double g[3], pcoeff[3][2] /* Note: pcoeff[i][0] is coefficient
		of p[1] in x[i]; pcoeff[i][1] is coefficient of p[2] */;
	KMPOINT tpt, pt;
	KMMATRIX to_plan, plan_to_axis, str_to_axis;
	double *intersection_list, absg[3], p[3], r;
	unsigned char *start_flag_list;
	int nintersections;

	intersection_list =
		(double *)malloc((str->rows+str->slices+str->col_max)*4*sizeof(float));
	if (intersection_list == NULL)
	{
		wxLogMessage("Out of memory.");
		return (-1);
	}
	start_flag_list =
		(unsigned char *)malloc(str->rows+str->slices+str->col_max);
	if (start_flag_list == NULL)
	{
		free(intersection_list);
		wxLogMessage("Out of memory.");
		return (-1);
	}
	MultMat(to_plan, transf, str->to_scanner);
	for (j=0; j<3; j++)
		for (k=0; k<3; k++)
			plan_to_axis[j][k] = inst->axis[(j+axis)%3][k];
	for (j=0; j<3; j++)
	{	plan_to_axis[j][3] = 0;
		for (k=0; k<3; k++)
			plan_to_axis[j][3] -= plan_to_axis[j][k]*inst->cent[k];
		plan_to_axis[3][j] = 0;
	}
	plan_to_axis[3][3] = 1;
	MultMat(str_to_axis, plan_to_axis, to_plan);
	order[0] = str_to_axis[0][0]<0? 1: -1;
	if (str_to_axis[0][1] < 0)
	{
		start[1] = 0;
		finish[1] = str->rows;
		order[1] = 1;
	}
	else
	{
		start[1] = str->rows-1;
		finish[1] = -1;
		order[1] = -1;
	}
	if (str_to_axis[0][2] < 0)
	{
		start[2] = 0;
		finish[2] = str->slices;
		order[2] = 1;
	}
	else
	{
		start[2] = str->slices-1;
		finish[2] = -1;
		order[2] = -1;
	}
	for (m=0; m<3; m++)
	{	j = (m+1)%3;
		k = (j+1)%3;
		g[k] = 2*(str_to_axis[2][j]*str_to_axis[1][m]-
			str_to_axis[1][j]*str_to_axis[2][m]);
		absg[k] = g[k]<0? -g[k]: g[k];
	}
	for (m=0; m<3; m++)
	{	j = (m+1)%3;
		k = (j+1)%3;
		if (g[m])
		{	pcoeff[m][0] = (str_to_axis[2][k]*str_to_axis[0][k]-
				str_to_axis[2][j]*str_to_axis[0][j])/g[m];
			pcoeff[m][1] = (str_to_axis[1][j]*str_to_axis[0][j]-
				str_to_axis[1][k]*str_to_axis[0][k])/g[m];
		}
	}
	nintersections = 0;
	for (slice=start[2]; slice!=finish[2]; slice+=order[2])
		for (row=start[1]; row!=finish[1]; row+=order[1])
		{
			face_axis = slice%2==0? 2: row%2==0;
			j = face_axis+1;
			if (j == 3)
				j = 0;
			k = j+1;
			if (k == 3)
				k = 0;
			if (order[0] < 0)
			{
				start[0] = str->count[slice][row]-1;
				finish[0] = -1;
			}
			else
			{
				start[0] = 0;
				finish[0] = str->count[slice][row];
			}
			ptr = (Cord_with_Norm *)str->node[slice][row];
			for (m=start[0]; m!=finish[0]; m+=order[0])
			{
        		pt[0] = (double)(ptr[m][0]&C_MASK);
        		pt[1] = (double)row;
        		pt[2] = (double)slice;
				TransformPoint(tpt, str_to_axis, pt);
				p[2] = -tpt[2];
				p[1] = -tpt[1];
				r = 2*(str_to_axis[2][j]*p[1]-str_to_axis[1][j]*p[2]);
				if (r<-absg[face_axis] || r>absg[face_axis])
					continue;
				r = 2*(str_to_axis[2][k]*p[1]-str_to_axis[1][k]*p[2]);
				if (r<-absg[face_axis] || r>absg[face_axis])
					continue;
				for (tp=nintersections; tp&&intersection_list[tp-1]<
				    pcoeff[face_axis][0]*p[1]+pcoeff[face_axis][1]*p[2]+tpt[0];
					tp--)
				{
				  intersection_list[tp] = intersection_list[tp-1];
				  start_flag_list[tp] = start_flag_list[tp-1];
				}
				intersection_list[tp] =
					pcoeff[face_axis][0]*p[1]+pcoeff[face_axis][1]*p[2]+tpt[0];
				start_flag_list[tp] =
					(order[face_axis]>0)==((ptr[m][0]&POSITIVE)==0);
				nintersections++;
			}
		}
	r = 0;
	for (j=0; j<nintersections-1; j++)
		if (start_flag_list[j])
		{
			for (k=j+1; k<nintersections&&start_flag_list[k]; k++)
				;
			if (start_flag_list[k])
				break;
			for (; k+1<nintersections&&!start_flag_list[k+1]; k++)
				;
			if (intersection_list[j]-intersection_list[k] > r)
				r = intersection_list[j]-intersection_list[k];
			j = k;
		}
	free(intersection_list);
	free(start_flag_list);
	return (r);
}

/*****************************************************************************
 * FUNCTION: get_volume
 * DESCRIPTION: Finds the volume enclosed by a surface.
 * PARAMETERS:
 *    str: Information about the structure.
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variable display must be appropriately initialized.
 * RETURN VALUE: The volume of the object in structure system units.
 * EXIT CONDITIONS: If an error occurs, an error message will be displayed.
 * HISTORY:
 *    Created: 9/3/97 by Dewey Odhner
 *
 *****************************************************************************/
static double get_volume(Surf_Struct *str)	
{
	int slice, row, m;
	Cord_with_Norm *ptr;
	double r;

	r = 0;
	for (slice=1; slice<str->slices; slice+=2)
		for (row=1; row<str->rows; row+=2)
		{
			ptr = (Cord_with_Norm *)str->node[slice][row];
			for (m=0; m<str->count[slice][row]; m+=2)
			{
				assert((ptr[m][0]&POSITIVE) == 0);
        		assert(ptr[m+1][0]&POSITIVE);
				r += (ptr[m+1][0]&C_MASK)-(ptr[m][0]&C_MASK);
			}
		}
	return (r*4);
}

void Print_Txt(FILE *fp, int obj, REG_SEQ *rel_seq )
{
  int i;

  for(i=0;i<time_inst-1;i++) {
    fprintf(fp, "Time instance %d to %d\n", i+1, i+2);
    fprintf(fp, "  Translation %f %f %f\n",
	    rel_seq[i].trans[0], rel_seq[i].trans[1], rel_seq[i].trans[2]);
    fprintf(fp, "  Axis_of_Rotation_unit_vector %f %f %f\n",
	    rel_seq[i].inst_axis[0], rel_seq[i].inst_axis[1],
	    rel_seq[i].inst_axis[2]);
    fprintf(fp, "  Angle_of_Rotation %f\n", rel_seq[i].rot*180.0/PI);
  }
  fprintf(fp, "\n");
}   

/* Modified: 9/20/02 object-relative transformation corrected by Dewey Odhner. */
void Compute_Relative(int ob,REG_SEQ *rel_seq)
{
  KMMATRIX ROTATION,TRANSLATION,TRANSROT;
  KMMATRIX M, REL_OBJ_TRANSF, REL_OBJ_ROT;
  double A[3][3],B[3][3],AInv[3][3],d,sin_val,cos_val;
  int i,j,k,l,m;


  MakeID(TRANSLATION);
  MakeID(REL_OBJ_ROT);
  if (REL_OBJECT) {
    for(j=0;j<3;j++) {
      for(k=0;k<3;k++)
        REL_OBJ_ROT[j][k] = seq[REL_OBJECT-1][0].axis[k][j];
      TRANSLATION[j][3] = seq[REL_OBJECT-1][0].cent[j];
    }
    MultMat(REL_OBJ_TRANSF, TRANSLATION, REL_OBJ_ROT);
  }    

  for(i=0;i<time_inst;i++) {
    MakeID(TRANSLATION);
    MakeID(ROTATION);
    MakeID(TRANSROT);
    
    if (REL_OBJECT) {
      for(j=0;j<3;j++) {
        for(k=0;k<3;k++)
          ROTATION[j][k]=seq[REL_OBJECT-1][i].axis[j][k];
        TRANSLATION[j][3]= -seq[REL_OBJECT-1][i].cent[j];
      }
      MultMat(M, ROTATION, TRANSLATION);
      MultMat(TRANSROT, REL_OBJ_TRANSF, M);
	  MultMat(M, REL_OBJ_ROT, ROTATION);
	  memcpy(ROTATION, M, sizeof(M));
    }    
    rel_seq[i].eval[0]=seq[ob][i].eval[0];
    rel_seq[i].eval[1]=seq[ob][i].eval[1];
    rel_seq[i].eval[2]=seq[ob][i].eval[2];
    TransformPoint(rel_seq[i].cent,TRANSROT,seq[ob][i].cent);
    TransformPoint(rel_seq[i].axis[0],ROTATION,seq[ob][i].axis[0]);
    TransformPoint(rel_seq[i].axis[1],ROTATION,seq[ob][i].axis[1]);
    TransformPoint(rel_seq[i].axis[2],ROTATION,seq[ob][i].axis[2]);
  }
    
  
  for(i=0;i<time_inst-1;i++) {
    MakeID(M);
    /* Get the Rotational KMMATRIX */
    for(k=0;k<3;k++)
      for(l=0;l<3;l++) {
	A[k][l]=rel_seq[i].axis[l][k];
	B[k][l]=rel_seq[i+1].axis[l][k];
      }
    Inv3Mat(A,AInv);
    for(k=0;k<3;k++)
      for(l=0;l<3;l++) {
	M[k][l]=0.0;
	for(m=0;m<3;m++)
	  M[k][l] += (B[k][m]*AInv[m][l]);
      }
    
    /* From Rotational KMMATRIX Get Inst. Axis and angle */
    d= sqrt(SQR((M[2][1] - M[1][2])) + 
	    SQR((M[0][2] - M[2][0])) +
	    SQR((M[1][0] - M[0][1])) );
    
    sin_val=d/2;
    cos_val=(M[0][0] + M[1][1] + M[2][2] -1)/2;
    if (d!=0.0000) { /* any angle other than 0 or 180 deg */
      rel_seq[i].inst_axis[0]=(M[2][1] - M[1][2])/d;
      rel_seq[i].inst_axis[1]=(M[0][2] - M[2][0])/d;
      rel_seq[i].inst_axis[2]=(M[1][0] - M[0][1])/d;
    }
    else if (cos_val==1.0) { /* 0 deg */
      rel_seq[i].inst_axis[0]=rel_seq[i].inst_axis[1]=0.0;
      rel_seq[i].inst_axis[2]=1.0;
      rel_seq[i].rot=0.0;
    }
    else {  /* 108 deg */
      rel_seq[i].inst_axis[0]=sqrt((M[0][0]+1)/2);
      rel_seq[i].inst_axis[1]=sqrt((M[1][1]+1)/2);
      rel_seq[i].inst_axis[2]=sqrt((M[2][2]+1)/2);
      if (rel_seq[i].inst_axis[0]*rel_seq[i].inst_axis[1]*M[1][0] < 0 )
	rel_seq[i].inst_axis[1]= -rel_seq[i].inst_axis[1];
      if (rel_seq[i].inst_axis[1]*rel_seq[i].inst_axis[2]*M[2][1] < 0 ) 
	rel_seq[i].inst_axis[2]= -rel_seq[i].inst_axis[2];
    }
    rel_seq[i].rot = atan2(sin_val,cos_val);                     
    if (rel_seq[i].rot > PI) rel_seq[i].rot = PI - rel_seq[i].rot; 
    
    /*Get Translation vector */
    for(k=0;k<3;k++)
      rel_seq[i].trans[k]= rel_seq[i+1].cent[k]-rel_seq[i].cent[k];
    DoTransform(rel_seq[i+1].M,rel_seq[i].M,
		rel_seq[i].cent,rel_seq[i].inst_axis,
		rel_seq[i].rot,rel_seq[i].trans);
  }
  
  rel_seq[time_inst-1].inst_axis[0]=1.0;
  rel_seq[time_inst-1].inst_axis[1]=0.0;
  rel_seq[time_inst-1].inst_axis[2]=0.0;
  rel_seq[time_inst-1].rot=0.0;
  rel_seq[time_inst-1].trans[0]=0.0;  
  rel_seq[time_inst-1].trans[1]=0.0;  
  rel_seq[time_inst-1].trans[2]=0.0;
 
}


int SaveIntraPlan(Surf_Info *InSurf, char *outfile, int timeInstances, int nRefFrame )
{
  int i,j, k[3], m, mode;
  FILE *fp;
  Surf_Tree *ptr;
  KMMATRIX M,InvM;
  KMPOINT cent, axis[3], rel_cent[3], t_axis;
  REG_SEQ *rel_seq;
  static KMPOINT zero={0, 0, 0};
  double a, len1, axis_length[3], volume;  
  

  surf = InSurf;
  CURRENT_SURF = 0;

  if( NULL != strstr(outfile, "PLN") || NULL != strstr(outfile, "pln") )
	  mode = 0;
  else  if( NULL != strstr(outfile, "TXT") || NULL != strstr(outfile, "txt") )
	  mode = 1;
  else  if( NULL != strstr(outfile, "PAR") || NULL != strstr(outfile, "par") )
	  mode = 2;
  else  if( NULL != strstr(outfile, "TRN") || NULL != strstr(outfile, "trn") )
	  mode = 3;
  else
	  mode = 0;

  out_time_inst = timeInstances;

  switch (mode) {
   case 0:
#ifndef SCRIPTABLE_VERSION
    wxLogMessage("Writing Plan. Wait....");   
#endif
    fp=fopen(outfile,"w");
    if (fp==NULL) {
      wxLogMessage("Could not Open Plan for writing ....");      
      return(-1);
    }
    fprintf(fp,"%s\n\n",surf[CURRENT_SURF].file_name);

    fprintf(fp,"AXIS %d {\n",num_objs);
    for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
      surf[CURRENT_SURF].tree_index[i]=0;
    
    ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
    for(i=0;i<ptr->children;i++) {
      Inv4Mat(ptr->child[i]->transf,InvM);
      TransformPoint(cent,InvM,seq[i][0].cent);
      TransformVector(axis[0],InvM,seq[i][0].axis[0]);
      TransformVector(axis[1],InvM,seq[i][0].axis[1]);
      TransformVector(axis[2],InvM,seq[i][0].axis[2]);
      
      fprintf(fp,
	    "%d { %lf %lf %lf } { %lf %lf %lf } { %lf %lf %lf } { %lf %lf %lf }\n",
	      ptr->child[i]->surf_index,
	      cent[0], cent[1], cent[2],
	      axis[0][0],axis[0][1],axis[0][2],
	      axis[1][0],axis[1][1],axis[1][2],
	      axis[2][0],axis[2][1],axis[2][2]);    
    }
    fprintf(fp,"}\n\n");
    
    fprintf(fp,"TREE 3 {\n");
    fprintf(fp,"3 %d ID\n",out_time_inst);
    for(i=0;i<out_time_inst;i++) 
      fprintf(fp,"2 %d ID\n",num_objs);
    
    if (ComputeOutSequence()!=-1) {
      
      for(i=0;i<out_time_inst;i++)
		for(j=0;j<num_objs;j++) {
		  fprintf(fp,"1 %d\n",ptr->child[j]->surf_index);
		  print4mat(out_seq[j][i].M,fp);
		}
      ReleaseOutSequence();
      
    }
    else {
      wxLogMessage("Error in writing plan");
      fclose(fp);
      return(-1);
      
    }   
    
    fprintf(fp,"}\n");
    fclose(fp);
    wxLogMessage("Done writing plan.");
	break;

   case 1:
#ifndef SCRIPTABLE_VERSION
    wxLogMessage("Writing report file. Wait....");
#endif
    fp=fopen(outfile,"w");
    if (fp==NULL) {
      wxLogMessage("Could not Open TXT for writing ....");      
    }   
    rel_seq=(REG_SEQ *)malloc(sizeof(REG_SEQ)*time_inst);
    if (rel_seq==NULL) {
      wxLogMessage("Memory allocation error");      
    }   

	fprintf(fp, "PATIENT-RELATED INFORMATION\n\n");
	fprintf(fp, "Study # : %s\n", surf->vh.gen.study_valid? surf->vh.gen.study:
	  "not given.");
	fprintf(fp, "Name : %s\n", surf->vh.gen.patient_name_valid?
	  surf->vh.gen.patient_name:"not given.");

	fprintf(fp, "MORPHOLOGY-RELATED INFORMATION\n\n");
    for(i=0; i<num_objs; i++) {
	  fprintf(fp, "Object_number_%d:\n", i+1);
      for(j=0; j<surf[CURRENT_SURF].tree->level-1; j++)
        surf[CURRENT_SURF].tree_index[j] = 0;
	  for (j=0; j<time_inst; j++)
	  {
	    fprintf(fp, "  Time_instance %d\n", j+1);
	    fprintf(fp, "   Centroid_coordinates %0.4lf %0.4lf %0.4lf\n",
	      seq[i][j].cent[0], seq[i][j].cent[1], seq[i][j].cent[2]);
	    fprintf(fp, "   Major_axis_vector %0.6lf %0.6lf %0.6lf\n",
	      seq[i][j].axis[0][0], seq[i][j].axis[0][1], seq[i][j].axis[0][2]);
	    fprintf(fp, "   2nd_principal_axis_vector %0.6lf %0.6lf %0.6lf\n",
	      seq[i][j].axis[1][0], seq[i][j].axis[1][1], seq[i][j].axis[1][2]);
	    fprintf(fp, "   3rd_principal_axis_vector %0.6lf %0.6lf %0.6lf\n",
	      seq[i][j].axis[2][0], seq[i][j].axis[2][1], seq[i][j].axis[2][2]);
        ptr = GetSurfPointer(&surf[CURRENT_SURF],
		  surf[CURRENT_SURF].tree_index, M );
	    for (m=0; m<3; m++)
	      axis_length[m] = get_axis_length(seq[i]+j,
			&surf[0].surf_struct[ptr->child[i]->surf_index], m,
			 ptr->child[i]->transf);
	    fprintf(fp, "   Eigenvalues %f %f %f\n", seq[i][j].eval[0],
	      seq[i][j].eval[1], seq[i][j].eval[2]);
	    volume = get_volume(&surf[0].surf_struct[ptr->child[i]->surf_index]);
	    volume *= surf[0].Px*surf[0].Py*surf[0].Pz;
	    fprintf(fp, "   Volume %0.4lf\n", volume);
		fprintf(fp, "   Cube_root_volume %f\n", pow(volume, 1/3.));
		fprintf(fp, "   Bounding_box_diagonal %f\n", sqrt(
		  (surf[j].max_x-surf[j].min_x)*(surf[j].max_x-surf[j].min_x)+
		  (surf[j].max_y-surf[j].min_y)*(surf[j].max_y-surf[j].min_y)+
		  (surf[j].max_z-surf[j].min_z)*(surf[j].max_z-surf[j].min_z)));
	    fprintf(fp, "   Root_lambda_1 %f\n", sqrt(seq[i][j].eval[0]));
	    if (surf->vh.str.surface_area_valid)
	      fprintf(fp, "   Surface_area %f\n",
		    surf->vh.str.surface_area[ptr->child[i]->surf_index]);
	    else
	      fprintf(fp, "   Surface_area not_given.\n");
	    fprintf(fp, "   Axis_lengths %0.4lf %0.4lf %0.4lf\n", axis_length[0],
	      axis_length[1], axis_length[2]);
	    len1 = axis_length[0]*axis_length[1]*axis_length[2];
	    if (len1 > 0)
	      fprintf(fp, "   Volume_fraction_of_axis_box %0.4lf\n", volume/len1);
	    else
		  fprintf(fp, "   Volume_of_axis_box_is zero!\n");
	    len1 = 2*(axis_length[0]*axis_length[1]+axis_length[0]*axis_length[2]+
	      axis_length[1]*axis_length[2]);
	    if (len1 <= 0)
	      fprintf(fp, "   Area_of_axis_box_is zero!\n");
	    else if (surf->vh.str.surface_area_valid)
	      fprintf(fp, "   Area_fraction_of_axis_box %0.4lf\n",
		    surf->vh.str.surface_area[ptr->child[i]->surf_index]/len1);
		GetNextStructureIndex(&surf[CURRENT_SURF],
		  surf[CURRENT_SURF].tree_index);
	  }
	  fprintf(fp, "\n");
    }

	fprintf(fp, "ARCHITECTURE-RELATED INFORMATION\n\n");

    if (ComputeOutSequence()!=-1) {
      fprintf(fp, "DISTANCE BETWEEN CENTROIDS\n\n");
	  for (k[1]=1; k[1]<num_objs; k[1]++)
	    for (k[0]=0; k[0]<k[1]; k[0]++)
		{
		  fprintf(fp, "  Objects %d--%d:\n", k[0]+1, k[1]+1);
		  fprintf(fp, "  time distance\n");
		  for (i=0; i<out_time_inst; i++)
		  {
		    TransformPoint(rel_cent[0], out_seq[k[0]][i].M, seq[k[0]][0].cent);
			TransformPoint(rel_cent[1], out_seq[k[1]][i].M, seq[k[1]][0].cent);
			fprintf(fp, "%6d %f\n", i+1, sqrt(
			  (rel_cent[0][0]-rel_cent[1][0])*(rel_cent[0][0]-rel_cent[1][0])+
			  (rel_cent[0][1]-rel_cent[1][1])*(rel_cent[0][1]-rel_cent[1][1])+
			  (rel_cent[0][2]-rel_cent[1][2])*(rel_cent[0][2]-rel_cent[1][2]))
			  );
		  }
		}
      fprintf(fp, "\nANGLE BETWEEN MAJOR AXES\n");
	  for (k[1]=1; k[1]<num_objs; k[1]++)
	    for (k[0]=0; k[0]<k[1]; k[0]++)
		{
		  fprintf(fp, "  Object %d--%d angle\n", k[0]+1, k[1]+1);
		  fprintf(fp, "  time degrees\n");
		  for (i=0; i<out_time_inst; i++)
		  {
            TransformVector(axis[0], out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
            TransformVector(axis[1], out_seq[k[1]][i].M, seq[k[1]][0].axis[1]);
			fprintf(fp, "%6d %f\n", i+1, angle(axis[0], zero, axis[1]));
		  }
		}
      fprintf(fp, "\nANGLE BETWEEN CENTROIDS\n");
	  for (k[2]=2; k[2]<num_objs; k[2]++)
	    for (k[1]=1; k[1]<k[2]; k[1]++)
		  for (k[0]=0; k[0]<k[1]; k[0]++)
		    for (m=0; m<3; m++)
			{
		      fprintf(fp, "  Object %d--Object %d--Object %d angle\n",
			    k[m]+1, k[(m+1)%3]+1, k[(m+2)%3]+1);
			  fprintf(fp, "  time degrees\n");
			  for (i=0; i<out_time_inst; i++) {
			    TransformPoint(rel_cent[0], out_seq[k[m]][i].M,
				  seq[k[m]][0].cent);
			    TransformPoint(rel_cent[1], out_seq[k[(m+1)%3]][i].M,
				  seq[k[(m+1)%3]][0].cent);
			    TransformPoint(rel_cent[2], out_seq[k[(m+2)%3]][i].M,
				  seq[k[(m+2)%3]][0].cent);
			    fprintf(fp, "%6d %f\n", i+1,
				  angle(rel_cent[0], rel_cent[1], rel_cent[2]));
		      }
		    }
      ReleaseOutSequence();
    }
    else {
      wxLogMessage("Error in writing parameters");
      fclose(fp);
      return(-1);
      
    }

	fprintf(fp, "\nKINEMATICS-RELATED INFORMATION\n\n");
    j = REL_OBJECT;
	for (REL_OBJECT=0; REL_OBJECT<=num_objs; REL_OBJECT++)
	{
	  if (REL_OBJECT == 0)
	    fprintf(fp, "MOTION RELATIVE TO SCANNER\n\n");
	  else
	    fprintf(fp, "MOTION RELATIVE TO OBJECT %d\n\n", REL_OBJECT);
	  for(i=0;i<num_objs;i++) {
	    if (i+1 == REL_OBJECT)
		  continue;
        fprintf(fp, "Motion of object %d\n", i+1);
		Compute_Relative(i,rel_seq);
        Print_Txt(fp,i,rel_seq);
      }
	}
	REL_OBJECT = j;

    fclose(fp);
    free(rel_seq);
    wxLogMessage("Done writing text file.");
	break;

   case 2:
#ifndef SCRIPTABLE_VERSION
    wxLogMessage("Writing parameters. Wait....");    
#endif
    fp=fopen(outfile,"w");
    if (fp==NULL) 
	{
      wxLogMessage("Could not open file for writing parameters.");      
      return(-1);
    }
    fprintf(fp,"%s\n\n",surf[CURRENT_SURF].file_name);
    
    for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
      surf[CURRENT_SURF].tree_index[i]=0;

    if (ComputeOutSequence()!=-1) {
      
      fprintf(fp, "Angle between centroids\n");
	  for (k[2]=2; k[2]<num_objs; k[2]++)
	    for (k[1]=1; k[1]<k[2]; k[1]++)
		  for (k[0]=0; k[0]<k[1]; k[0]++)
		    for (m=0; m<3; m++)
			{
		      fprintf(fp, "\nObjects %d--%d--%d:\ntime degrees\n",
			    k[m]+1, k[(m+1)%3]+1, k[(m+2)%3]+1);
			  for (i=0; i<out_time_inst; i++) {
			    TransformPoint(rel_cent[0], out_seq[k[m]][i].M,
				  seq[k[m]][0].cent);
			    TransformPoint(rel_cent[1], out_seq[k[(m+1)%3]][i].M,
				  seq[k[(m+1)%3]][0].cent);
			    TransformPoint(rel_cent[2], out_seq[k[(m+2)%3]][i].M,
				  seq[k[(m+2)%3]][0].cent);
			    fprintf(fp, "%d %f\n", i+1,
				  angle(rel_cent[0], rel_cent[1], rel_cent[2]));
		      }
		    }
      fprintf(fp, "\n\n\nDistance between centroids\n");
	  for (k[1]=1; k[1]<num_objs; k[1]++)
	    for (k[0]=0; k[0]<k[1]; k[0]++)
		{
		  fprintf(fp, "\nObjects %d--%d:\ntime distance\n", k[0]+1, k[1]+1);
		  for (i=0; i<out_time_inst; i++)
		  {
		    TransformPoint(rel_cent[0], out_seq[k[0]][i].M, seq[k[0]][0].cent);
			TransformPoint(rel_cent[1], out_seq[k[1]][i].M, seq[k[1]][0].cent);
			fprintf(fp, "%d %f\n", i+1, sqrt(
			  (rel_cent[0][0]-rel_cent[1][0])*(rel_cent[0][0]-rel_cent[1][0])+
			  (rel_cent[0][1]-rel_cent[1][1])*(rel_cent[0][1]-rel_cent[1][1])+
			  (rel_cent[0][2]-rel_cent[1][2])*(rel_cent[0][2]-rel_cent[1][2]))
			  );
		  }
		}
      fprintf(fp, "\n\n\nAngle between major axes\n");
	  for (k[1]=1; k[1]<num_objs; k[1]++)
	    for (k[0]=0; k[0]<k[1]; k[0]++)
		{
		  fprintf(fp, "\nObjects %d--%d:\ntime degrees\n", k[0]+1, k[1]+1);
		  for (i=0; i<out_time_inst; i++)
		  {
            TransformVector(axis[0], out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
            TransformVector(axis[1], out_seq[k[1]][i].M, seq[k[1]][0].axis[1]);
			fprintf(fp, "%d %f\n", i+1, angle(axis[0], zero, axis[1]));
		  }
		}
      ReleaseOutSequence();
    }
    else {
      wxLogMessage("Error in writing parameters");
      fclose(fp);
      return(-1);
      
    }

    fclose(fp);
    wxLogMessage("Done writing parameters.");
	break;   

#ifndef SCRIPTABLE_VERSION
   case 3:
    if (time_inst < 2)
      break;
    wxLogMessage("Writing turn angles. Wait....");
    
    fp=fopen(outfile,"w");
    if (fp==NULL) {
      wxLogMessage("Could not open file for writing parameters.");      
      return(-1);
    }
    fprintf(fp,"%s\n\n",surf[CURRENT_SURF].file_name);
    for(i=0;i<surf[CURRENT_SURF].tree->level-1;i++)
      surf[CURRENT_SURF].tree_index[i]=0;
    ptr=GetSurfPointer(&surf[CURRENT_SURF],surf[CURRENT_SURF].tree_index,M);
  //  VUndisplayScale("INSTANCES");
  //  VSetScaleInformation("REF. FRAME", 0, 0, 1., (float)out_time_inst, 0, 0.5);
	ref_time = nRefFrame-1;
	/*val = ref_time+1;
	if (val > out_time_inst)
	  val = out_time_inst;*/
  //  VDisplayScale(val, "REF. FRAME");
	//DisplayMessage("Select reference frame.", FALSE);
	//VDisplayButtonAction("SELECT", "", "DONE");
	/*do
	{
      VNextEvent(&event);
	  if (VCheckScaleEvent(&event, "REF. FRAME", NULL, NULL, &val) == 0)
        ref_time = val-1;
	} while (event.type!=ButtonPress || event.xbutton.button!=Button3);
	*/
	if (ComputeOutSequence()!=-1) 
	{
      MultMat(M, out_seq[CURRENT_OBJ][ref_time].M, surf[CURRENT_SURF].
		surf_struct[ptr->child[CURRENT_OBJ]->surf_index].to_scanner);
	  Inv4Mat(M, InvM);
      fprintf(fp, "\n\nDirection vector of major axis\n");
	  for (k[0]=0; k[0]<num_objs; k[0]++)
		{
		  fprintf(fp, "\nObject %d:\ntime x-angle y-angle z-angle\n", k[0]+1);
		  for (i=0; i<out_time_inst; i++)
		  {
            TransformVector(t_axis, out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[0], InvM, t_axis);
			j = FALSE;
			if (i == 0)
			  j = axis[0][0]+axis[0][1]+axis[0][2]<0;
			a = 1/sqrt(axis[0][0]*axis[0][0]+axis[0][1]*axis[0][1]+
			  axis[0][2]*axis[0][2]);
			if (j)
			  a = -a;
			fprintf(fp, "%d %f %f %f\n", i+1, acos(axis[0][0]*a)*(180/PI),
			  acos(axis[0][1]*a)*(180/PI), acos(axis[0][2]*a)*(180/PI));
		  }
		}
      fprintf(fp, "\n\nAngle of major axis motion about x-axis\n");
	  for (k[0]=0; k[0]<num_objs; k[0]++)
		{
		  fprintf(fp, "\nObject %d:\ntime degrees\n", k[0]+1);
		  for (i=1; i<out_time_inst; i++)
		  {
            TransformVector(t_axis, out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[0], InvM, t_axis);
            TransformVector(t_axis, out_seq[k[0]][i-1].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[1], InvM, t_axis);
			axis[0][0] = 0;
			axis[1][0] = 0;
			a = angle(axis[0], zero, axis[1]);
			if (axis[0][1]*axis[1][2] < axis[0][2]*axis[1][1])
			  a = -a;
			fprintf(fp, "%d %f\n", i+1, a);
		  }
		}
      fprintf(fp, "\n\nAngle of major axis motion about y-axis\n");
	  for (k[0]=0; k[0]<num_objs; k[0]++)
		{
		  fprintf(fp, "\nObject %d:\ntime degrees\n", k[0]+1);
		  for (i=1; i<out_time_inst; i++)
		  {
            TransformVector(t_axis, out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[0], InvM, t_axis);
            TransformVector(t_axis, out_seq[k[0]][i-1].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[1], InvM, t_axis);
			axis[0][1] = 0;
			axis[1][1] = 0;
			a = angle(axis[0], zero, axis[1]);
			if (axis[0][2]*axis[1][0] < axis[0][0]*axis[1][2])
			  a = -a;
			fprintf(fp, "%d %f\n", i+1, a);
		  }
		}
      fprintf(fp, "\n\nAngle of major axis motion about z-axis\n");
	  for (k[0]=0; k[0]<num_objs; k[0]++)
		{
		  fprintf(fp, "\nObject %d:\ntime degrees\n", k[0]+1);
		  for (i=1; i<out_time_inst; i++)
		  {
            TransformVector(t_axis, out_seq[k[0]][i].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[0], InvM, t_axis);
            TransformVector(t_axis, out_seq[k[0]][i-1].M, seq[k[0]][0].axis[0]);
			TransformVector(axis[1], InvM, t_axis);
			axis[0][2] = 0;
			axis[1][2] = 0;
			a = angle(axis[0], zero, axis[1]);
			if (axis[0][0]*axis[1][1] < axis[0][1]*axis[1][0])
			  a = -a;
			fprintf(fp, "%d %f\n", i+1, a);
		  }
		}
      ReleaseOutSequence();
    }
    else {
      wxLogMessage("Error in writing parameters");
      fclose(fp);
      return(-1);
      
    }    

 /*   VUndisplayScale("REF. FRAME");
    VDeleteScale("REF. FRAME");
    VDisplayScale((float)out_time_inst,"INSTANCES");
 */   
	fclose(fp);
    wxLogMessage("Done writing parameters.");
	break;
#endif
  }

  return 1;
}

int StaticAnalyze(Surf_Info *InSurf, char DisplayString[NUM_FEATURE_LINES][MSG_WIDTH])
{
	/*int obj_num;  
	char out_filename[100];  */

	surf = InSurf;

	load_static_data();
	/*strcpy(out_filename, surf[0].file_name);
	if (strlen(out_filename) > MAX_DEFAULT_CHAR-2)
		strcpy(out_filename+MAX_DEFAULT_CHAR-6, ".");
	strcpy(strrchr(out_filename, '.'), ".STAT");*/
//	display_output_prompt(output_stats, out_filename);
	DisplaySurfaceStatistics(0, DisplayString);

	return 1;
}

/*****************************************************************************
 * FUNCTION: load_static_data
 * DESCRIPTION: Loads structure information into the variable surf[0].
 * PARAMETERS: None
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: The variables surf[0].tree, surf[0].tree_index,
 *    surf (parts; InitPlan, VReadHeader, BuildSurfTree must be called) must
 *    be initialized.  A successful call to VCreateColormap must be made first.
 * RETURN VALUE: 0 if successful.
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 8/13/97 by Dewey Odhner
 *
 *****************************************************************************/
int load_static_data()
{
  int j;
  Surf_Tree *ptr;
  KMMATRIX M; 

  ptr=GetSurfPointer(&surf[0] ,surf[0].tree_index, M);
  for(j=0;j<ptr->children;j++) 
    if (Load_Data(0,ptr->child[j]->surf_index)==-1) 
	{
      printf("Could not read struct %d\n", ptr->child[j]->surf_index);
      return(-1);
    }

  return(0);
}


/*****************************************************************************
 * FUNCTION: DisplaySurfaceStatistics
 * DESCRIPTION: Displays the static measures of a surface in the image window.
 * PARAMETERS:
 *    obj_num: The structure number from zero.
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variables display, img, dial, butt, argv0, surf (parts;
 *    InitPlan, VReadHeader, BuildSurfTree must be called), num_surf
 *    must be appropriately initialized.
 *    The input file must not change between calls.
 * RETURN VALUE: 0 if successful.
 * EXIT CONDITIONS: If an error occurs, an error message will be displayed.
 * HISTORY:
 *    Created: 9/3/97 by Dewey Odhner
 *
 *****************************************************************************/
int DisplaySurfaceStatistics(int obj_num, char DisplayStr[NUM_FEATURE_LINES][MSG_WIDTH])	
{
  int  error;
  char msg[40];

  wxLogMessage("Doing surface analysis. Wait ....");
//  XFlush(display);

  error = GetSurfaceStatistics(feature_text, obj_num);
  if (error)
  {
    sprintf(msg,"Could not get object %d statistics.",obj_num+1);
    wxLogMessage(msg);   
    return (error);
  }

  for( int i=0; i<NUM_FEATURE_LINES; i++ )
  {
	  strcpy(DisplayStr[i], feature_text[i]);
  }
  //VGetWindowInformation(img,&junk,&junk,&width,&junk,&font_w,&font_h,&junk,
  //  &junk);
  //VClearWindow(img, 0, 0, width, font_h*(NUM_FEATURE_LINES+1));
  //VDisplayImageMessage(img, feature_text, NUM_FEATURE_LINES, 10, font_h);

  wxLogMessage("Object statistics displayed.");
  return(0);
}

/*****************************************************************************
 * FUNCTION: GetSurfaceStatistics
 * DESCRIPTION: Computes the static measures of a surface.
 * PARAMETERS:
 *    msg: The output goes here.
 *    obj_num: The structure number from zero.
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variable surf (parts;
 *    InitPlan, VReadHeader, BuildSurfTree must be called)
 *    must be appropriately initialized.
 *    The input file must not change between calls.
 * RETURN VALUE: 0 if successful.
 * EXIT CONDITIONS: If an error occurs, an error message will be displayed.
 * HISTORY:
 *    Created: 9/4/97 by Dewey Odhner
 *    Modified: 6/8/99 angle cosines reported by Dewey Odhner
 *
 *****************************************************************************/
int GetSurfaceStatistics(char msg[NUM_FEATURE_LINES][MSG_WIDTH], int obj_num)    
{
  int j,k,l, surf_index;
  double len1, len2, axis_length[3], volume;
  Surf_Tree *ptr;
  KMMATRIX M;


  if (sscanf(msg[0], "Object number: %d", &j)==1 && j==obj_num+1)
    return (0);
  static_inst[0].obj_index = obj_num;
  ptr = GetSurfPointer(&surf[0], surf[0].tree_index, M);
  surf_index = -1;
  for(j=0; j<ptr->children; j++) 
    if (obj_num == ptr->child[j]->surf_index) 
      InitInstance(&static_inst[0], 0, surf_index=obj_num, ptr->child[j]->transf);
  if (surf_index < 0)
    return (1);
  
  /* Make Axis Consistent */
  for(k=0; k<2; k++) {
    len1= 0;
	j = 0;
	for (l=0; l<3; l++)
	{
		if (static_inst[0].axis[k][l] > len1)
			len1=static_inst[0].axis[k][l], j=l;
		if (-static_inst[0].axis[k][l] > len1)
			len1= -static_inst[0].axis[k][l], j=l;
	}
	if (static_inst[0].axis[k][j] < 0)
		for (l=0; l<3; l++)
			static_inst[0].axis[k][l] = -static_inst[0].axis[k][l];
  }
  for (l=0; l<3; l++)
    static_inst[0].axis[2][l] = static_inst[0].axis[0][(l+1)%3]*static_inst[0].axis[1][(l+2)%3]-
		static_inst[0].axis[1][(l+1)%3]*static_inst[0].axis[0][(l+2)%3];

  for (j=0; j<3; j++)
    axis_length[j] = get_axis_length_static(static_inst,
		&surf[0].surf_struct[obj_num], j);
  volume = get_volume(&surf[0].surf_struct[obj_num]);
  volume *= surf[0].Px*surf[0].Py*surf[0].Pz;

  sprintf(msg[0], "Object number: %d", obj_num+1);
  sprintf(msg[1], "Centroid coordinates: (%0.4lf, %0.4lf, %0.4lf)",
    static_inst[0].cent[0], static_inst[0].cent[1], static_inst[0].cent[2]);
  len2 = sqrt(static_inst[0].cent[0]*static_inst[0].cent[0]+static_inst[0].cent[1]*static_inst[0].cent[1]+
	static_inst[0].cent[2]*static_inst[0].cent[2]);
  sprintf(msg[2], "Centroid magnitude: %0.4lf", len2);
  sprintf(msg[3], "Centroid angles: %0.4lf, %0.4lf, %0.4lf",
    acos(static_inst[0].cent[0]/len2)*180/PI, acos(static_inst[0].cent[1]/len2)*180/PI,
	acos(static_inst[0].cent[2]/len2)*180/PI);
  sprintf(msg[4], "Major axis angles (& cosines): %0.4lf, %0.4lf, %0.4lf (%0.6lf, %0.6lf, %0.6lf)",
    acos(static_inst[0].axis[0][0])*180/PI, acos(static_inst[0].axis[0][1])*180/PI,
	acos(static_inst[0].axis[0][2])*180/PI,
    static_inst[0].axis[0][0], static_inst[0].axis[0][1], static_inst[0].axis[0][2]);
  sprintf(msg[5], "2nd principal axis angles (& cosines): %0.4lf, %0.4lf, %0.4lf (%0.6lf, %0.6lf, %0.6lf)",
    acos(static_inst[0].axis[1][0])*180/PI, acos(static_inst[0].axis[1][1])*180/PI,
	acos(static_inst[0].axis[1][2])*180/PI,
    static_inst[0].axis[1][0], static_inst[0].axis[1][1], static_inst[0].axis[1][2]);
  sprintf(msg[6], "3rd principal axis angles (& cosines): %0.4lf, %0.4lf, %0.4lf (%0.6lf, %0.6lf, %0.6lf)",
    acos(static_inst[0].axis[2][0])*180/PI, acos(static_inst[0].axis[2][1])*180/PI,
	acos(static_inst[0].axis[2][2])*180/PI,
    static_inst[0].axis[2][0], static_inst[0].axis[2][1], static_inst[0].axis[2][2]);
  sprintf(msg[7], "Axis lengths: %0.4lf, %0.4lf, %0.4lf", axis_length[0],
    axis_length[1], axis_length[2]);
  sprintf(msg[8], "Axis length ratios: %0.4lf, %0.4lf, %0.4lf",
    axis_length[1]/axis_length[0],
    axis_length[2]/axis_length[0], axis_length[2]/axis_length[1]);
  sprintf(msg[9], "Volume: %0.4lf", volume);
  len1 = axis_length[0]*axis_length[1]*axis_length[2];
  if (len1 > 0)
    sprintf(msg[10], "Volume fraction of axis box: %0.4lf", volume/len1);
  else
	strcpy(msg[10], "Volume of axis box is zero!");

  return(0);
}


/*****************************************************************************
 * FUNCTION: get_axis_length
 * DESCRIPTION: Finds the length of the longest segment of the principal axis
 *    within the object.
 * PARAMETERS:
 *    inst: Information about the object/time intannce.  Must me initianlized
 *       as with a call to InitInstance.
 *    str: Information about the structure.
 *    axis: Which axis to compute: 0, 1, or 2.
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variable display must be appropriately initialized.
 * RETURN VALUE: The length of the longest segment of the principal axis
 *    within the object.
 * EXIT CONDITIONS: If an error occurs, an error message will be displayed.
 * HISTORY:
 *    Created: 8/29/97 by Dewey Odhner
 *    Modified: 9/5/97 KMMATRIX initialization corrected by Dewey Odhner
 *    Modified: 9/24/97 KMMATRIX computation corrected by Dewey Odhner
 *
 *****************************************************************************/
double get_axis_length_static(REG_SEQ * inst, Surf_Struct * str, int axis)	
{
	int j, k, slice, row, m, face_axis, order[3], start[3], finish[3];
	Cord_with_Norm *ptr;
	double g[3], pcoeff[3][2] /* Note: pcoeff[i][0] is coefficient
		of p[1] in x[i]; pcoeff[i][1] is coefficient of p[2] */;
	KMPOINT tpt, pt;
	KMMATRIX to_plan, plan_to_axis, str_to_axis;
	double *intersection_list, absg[3], p[3], r;
	unsigned char *start_flag_list;
	int nintersections;

	intersection_list =
		(double *)malloc((str->rows+str->slices+str->col_max)*sizeof(float));
	if (intersection_list == NULL)
	{
		wxLogMessage("Out of memory.");    	
		return (-1);
	}
	start_flag_list =
		(unsigned char *)malloc(str->rows+str->slices+str->col_max);
	if (start_flag_list == NULL)
	{
		free(intersection_list);
		wxLogMessage("Out of memory.");    	
		return (-1);
	}
	MultMat(to_plan, inst->M, str->to_scanner);
	for (j=0; j<3; j++)
		for (k=0; k<3; k++)
			plan_to_axis[j][k] = inst->axis[(j+axis)%3][k];
	for (j=0; j<3; j++)
	{	plan_to_axis[j][3] = 0;
		for (k=0; k<3; k++)
			plan_to_axis[j][3] -= plan_to_axis[j][k]*inst->cent[k];
		plan_to_axis[3][j] = 0;
	}
	plan_to_axis[3][3] = 1;
	MultMat(str_to_axis, plan_to_axis, to_plan);
	order[0] = str_to_axis[0][0]<0? 1: -1;
	if (str_to_axis[0][1] < 0)
	{
		start[1] = 0;
		finish[1] = str->rows;
		order[1] = 1;
	}
	else
	{
		start[1] = str->rows-1;
		finish[1] = -1;
		order[1] = -1;
	}
	if (str_to_axis[0][2] < 0)
	{
		start[2] = 0;
		finish[2] = str->slices;
		order[2] = 1;
	}
	else
	{
		start[2] = str->slices-1;
		finish[2] = -1;
		order[2] = -1;
	}
	for (m=0; m<3; m++)
	{	j = (m+1)%3;
		k = (j+1)%3;
		g[k] = 2*(str_to_axis[2][j]*str_to_axis[1][m]-
			str_to_axis[1][j]*str_to_axis[2][m]);
		absg[k] = g[k]<0? -g[k]: g[k];
	}
	for (m=0; m<3; m++)
	{	j = (m+1)%3;
		k = (j+1)%3;
		if (g[m])
		{	pcoeff[m][0] = (str_to_axis[2][k]*str_to_axis[0][k]-
				str_to_axis[2][j]*str_to_axis[0][j])/g[m];
			pcoeff[m][1] = (str_to_axis[1][j]*str_to_axis[0][j]-
				str_to_axis[1][k]*str_to_axis[0][k])/g[m];
		}
	}
	nintersections = 0;
	for (slice=start[2]; slice!=finish[2]; slice+=order[2])
		for (row=start[1]; row!=finish[1]; row+=order[1])
		{
			face_axis = slice%2==0? 2: row%2==0;
			j = face_axis+1;
			if (j == 3)
				j = 0;
			k = j+1;
			if (k == 3)
				k = 0;
			if (order[0] < 0)
			{
				start[0] = str->count[slice][row]-1;
				finish[0] = -1;
			}
			else
			{
				start[0] = 0;
				finish[0] = str->count[slice][row];
			}
			ptr = (Cord_with_Norm *)str->node[slice][row];
			for (m=start[0]; m!=finish[0]; m+=order[0])
			{
        		pt[0] = (double)(ptr[m][0]&C_MASK);
        		pt[1] = (double)row;
        		pt[2] = (double)slice;
				TransformPoint(tpt, str_to_axis, pt);
				p[2] = -tpt[2];
				p[1] = -tpt[1];
				r = 2*(str_to_axis[2][j]*p[1]-str_to_axis[1][j]*p[2]);
				if (r<-absg[face_axis] || r>absg[face_axis])
					continue;
				r = 2*(str_to_axis[2][k]*p[1]-str_to_axis[1][k]*p[2]);
				if (r<-absg[face_axis] || r>absg[face_axis])
					continue;
				intersection_list[nintersections] =
					pcoeff[face_axis][0]*p[1]+pcoeff[face_axis][1]*p[2]+tpt[0];
				start_flag_list[nintersections] = (order[face_axis]>0)==
					((ptr[m][0]&POSITIVE)==0);
				nintersections++;
			}
		}
	r = 0;
	for (j=0; j<nintersections-1; j++)
		if (start_flag_list[j] && !start_flag_list[j+1] &&
				intersection_list[j]-intersection_list[j+1]>r)
			r = intersection_list[j]-intersection_list[j+1];
	free(intersection_list);
	free(start_flag_list);
	return (r);
}


/*****************************************************************************
 * FUNCTION: output_stats
 * DESCRIPTION: Writes the statistics to a file.
 * PARAMETERS: None
 * SIDE EFFECTS: The global variables feature_text may be set.
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variables static_inst[0].obj_index, surf[0], argv0,
 *    feature_text must be appropriately initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: If an error occurs, an error message will be displayed.
 * HISTORY:
 *    Created: 9/3/97 by Dewey Odhner
 *
 *****************************************************************************/
void output_stats(char* file_name)
{
	int error_code, j; //, overwrite_flag;
	char msg[124];
	FILE *fp;

	//error_code = VGetSaveFilename(file_name);
	//if (error_code)
	//{
	//	handle_error(argv0, "output_stats", error_code, 0);
	//	return;
	//}

//	VGetSaveSwitchValue(&overwrite_flag);
//	fp = fopen(file_name, overwrite_flag? "w":"a");
	fp = fopen(file_name, "a");
	if (fp == NULL)
	{
		wxLogMessage("Can not open output_stats file!");
		return;
	}
    error_code = GetSurfaceStatistics(feature_text, static_inst[0].obj_index);
    if (error_code)
    {
      sprintf(msg, "Could not get object %d statistics.", static_inst[0].obj_index+1);
      wxLogMessage(msg);
      return;
    }
	for (j=0; j<NUM_FEATURE_LINES; j++)
		if (fprintf(fp, "%s\n", feature_text[j]) <= 1)
		{
			wxLogMessage("output_stats error", 3, 0);
			return;
		}
	fprintf(fp, "\n");
	fclose(fp);
	sprintf(msg, "Output to file %s complete", file_name);
	wxLogMessage(msg);
}
