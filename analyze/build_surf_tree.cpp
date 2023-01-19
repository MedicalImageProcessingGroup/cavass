/*
  Copyright 1993-2012, 2018 Medical Image Processing Group
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
#include "build_surf_tree.h"

static int cmp_pv(const void *s1, const void * s2);

static short *ORDER;
static int DIM;

#define SQR(x) (x)*(x)

DEFS Defaults = {
  100, 100, 100,
  1.0,
  (float)0.2,3.0,1.0,
  1.0,2.0,
  0,0,0,
  0x27FF,0x27FF,0x27FF,
  0x7FFF,0xFFFF,0x7FFF
  };


/************************************************************************
 *
 *      FUNCTION        : BuildSurfTree
 *
 *      DESCRIPTION     : This function initializes the surf structure 
 *                        with the fields in the viewnix header. This
 *                        would build a tree hierarchy for the objects 
 *                        based on the parameter vectors. 
 *
 *      RETURN VALUE    : 0 - if successful, -1 otherwise.
 *
 *      PARAMETERS      : srf - surf structure to be initialized.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : fp should be opened for reading and vh
 *                        should be initialized in the srf structure.
 *
 *      EXIT CONDITIONS : Invalid header elements. Read Error.
 *
 *      RELATED FUNCS   : BuildOrder, GetBoxFromSurf.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int BuildSurfTree(Surf_Info *srf)
{

  int i,j,k,len,levels,ind,items;
  StructureInfo *str;
  Surf_Struct *st;
  short tmp_row;
  float *sorted_list;
  double val;

  srf->num_structs=srf->vh.str.num_of_structures;  
  
  str=&srf->vh.str;
  if (str->dimension!=3) {
    //VDisplayDialogMessage("This module only handles 3d structures ");
	  wxLogMessage("This module only handles 3d structures ");
    return(-1);
  } 
  if (str->num_of_components_in_TSE!=9 || str->num_of_bits_in_TSE!=32)
  {
    wxLogMessage("The TSE format is not handled by 3DVIEWNIX");    
    return(-1);
  }

  if (!srf->plan_on) {
    ORDER=(short *)malloc(sizeof(short)*str->num_of_elements);
    DIM=str->num_of_elements;
    ind=0;
    for(i=0;i<str->num_of_elements;i++)
      if (str->description_of_element[i]>=5) /* order by other category */
	ORDER[ind++]=i;
    for(i=0;i<str->num_of_elements;i++)      /* order by subject        */
      if (str->description_of_element[i]==4)
	ORDER[ind++]=i;
    for(i=0;i<str->num_of_elements;i++)      /* order by modality       */
      if (str->description_of_element[i]==2)
	ORDER[ind++]=i;
    for(i=0;i<str->num_of_elements;i++)      /* order by longitudinal time*/
      if (str->description_of_element[i]==3)
	ORDER[ind++]=i;
    for(i=0;i<str->num_of_elements;i++)      /* order by time           */
      if (str->description_of_element[i]==0)
	ORDER[ind++]=i;
    for(i=0;i<str->num_of_elements;i++)      /* order by object         */
      if (str->description_of_element[i]==1)
	ORDER[ind++]=i;
    if (str->description_of_element[ORDER[ind-1]]==1)
      levels=DIM;
    else 
      levels=DIM+1;
    sorted_list=(float *)malloc(sizeof(float)*(str->num_of_elements+1)*str->num_of_structures);
    for(i=0;i<str->num_of_structures;i++) {
      ind=(str->num_of_elements+1)*i;
      for(j=0;j<str->num_of_elements;j++)
	sorted_list[ind+j]=str->parameter_vectors[i*str->num_of_elements+j];
      sorted_list[ind+str->num_of_elements]=(float)i;
      }
    qsort(sorted_list,str->num_of_structures,sizeof(float)*(str->num_of_elements+1),cmp_pv);
    BuildOrder(NULL,&srf->tree,sorted_list,0,0,(int)(str->num_of_structures-1),levels);
    

    if (srf->tree->level>1) {
      srf->tree_index=(short *)calloc(srf->tree->level-1,sizeof(short));
      srf->index_lock=(char *)calloc(srf->tree->level-1,sizeof(char));
    }
    else {
      srf->tree_index=NULL;
      srf->index_lock=NULL;
    }
    free(ORDER);
  }


  /* Sampling scheme is half the voxel size in each direction */
  srf->Px=str->xysize[0]; srf->Py=str->xysize[1];
  srf->Pz=fabs(str->loc_of_samples[1]-str->loc_of_samples[0]);

  /* Read in the structures */
  srf->surf_struct=(Surf_Struct *)malloc(sizeof(Surf_Struct)*srf->num_structs);
  /* Point to the beginning of the data segment */
  if (VSeekData(srf->fp,0)) 
  {
    wxLogMessage("Seek failed in reading file");    
    return(-1);
  }
  len=0;
  for(st=srf->surf_struct,i=0;i<srf->num_structs;st++,i++) 
  {
    VReadData((char*)(&st->slices),sizeof(short),1,srf->fp,&items);
    VReadData((char*)(&st->rows),sizeof(short),1,srf->fp,&items);
    for(j=1;j<st->slices;j++) {
      VReadData((char*)(&tmp_row),sizeof(short),1,srf->fp,&items);
      if (st->rows!=tmp_row) {
	printf("The surface should have equal number of rows per slice\n"); 
	return(-1);
      }
    }
    st->count=(unsigned short **)malloc(sizeof(short *)*st->slices);
    st->node=(Cord_with_Norm ***)malloc(sizeof(short **)*st->slices);
    for(j=0;j<st->slices;j++) {
      st->count[j]=(unsigned short *)malloc(sizeof(short)*st->rows);
      st->node[j]=(Cord_with_Norm **)malloc(sizeof(short *)*st->rows);
      VReadData((char*)(st->count[j]),sizeof(short),st->rows,srf->fp,&items);
    }
    len += 2*(1+st->slices*(1+st->rows));
    st->start = len;
    for(j=0;j<st->slices;j++)
      for(k=0;k<st->rows;k++) {
	if (st->count[j][k]) {
	  len+= 4*st->count[j][k];
	  fseek(srf->fp,(long)(4*st->count[j][k]),1L);
	}
      }

    st->AXIS_FLAG=FALSE;
    st->DATA_LOADED=FALSE;

    st->r = Defaults.r;
    st->g = Defaults.g;
    st->b = Defaults.b;
    st->seg_map=NULL;
    st->seg_map_size=0;
    st->seg_plane_map=NULL;
    st->seg_plane_map_size=0;
    st->DISPLAY_FLAG=1;
    st->spec_ratio=Defaults.spec_ratio;
    st->diff_exp=Defaults.diff_exp;
    st->spec_exp=Defaults.spec_exp;
    st->opacity=Defaults.opacity;
    st->spec_div=Defaults.spec_div;
    st->diff_div=Defaults.diff_div;

    st->min_x= str->min_max_coordinates[6*i];
    st->min_y= str->min_max_coordinates[6*i+1];
    st->min_z= str->min_max_coordinates[6*i+2];
    st->max_x= str->min_max_coordinates[6*i+3];
    st->max_y= str->min_max_coordinates[6*i+4];
    st->max_z= str->min_max_coordinates[6*i+5];
    st->col_min=(unsigned short)rint(st->min_x/srf->Px);
    st->col_max=(unsigned short)rint(st->max_x/srf->Px);



    /* Initialize the KMMATRIX using min,domain,and voxel size. */
    st->to_scanner[3][3]=1.0;
    for(j=0;j<3;j++) {
      st->to_scanner[j][0]=srf->Px*str->domain[12*i+3+j];
      st->to_scanner[j][1]=srf->Py*str->domain[12*i+6+j];
      st->to_scanner[j][2]=srf->Pz*str->domain[12*i+9+j];
      st->to_scanner[j][3]=str->domain[12*i+j]+
	str->domain[12*i+6+j]*st->min_y+str->domain[12*i+9+j]*st->min_z;
      st->to_scanner[3][j]=0.0;
    }
    


  }
  srf->WINDOW_ON=FALSE;
  srf->ANTIALIAS_FLAG=FALSE;
  if (!srf->plan_on) GetBoxFromSurf(srf);

  srf->max_pix_size= 2.0*sqrt( SQR(srf->Px) + SQR(srf->Py) );
  val=2.0*sqrt( SQR(srf->Py) + SQR(srf->Pz) );
  if (val > srf->max_pix_size) srf->max_pix_size=val;
  val=2.0*sqrt( SQR(srf->Px) + SQR(srf->Pz) );
  if (val > srf->max_pix_size) srf->max_pix_size=val;
  

  srf->mpt=NULL;
  srf->num_mpt=0;
  srf->seg_length=srf->full_length=srf->angle=0.0;


  srf->txt_list=NULL;
  srf->num_tags=0;

  return(0);
}



/************************************************************************
 *
 *      FUNCTION        : InitPlan
 *
 *      DESCRIPTION     : If the structure system is a plan this would 
 *                        parse the plan file and build the surf structure.
 *
 *      RETURN VALUE    : 0 if successfull, -1 otherwise.
 *
 *      PARAMETERS      : s - current surface structure.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : plan file should be proper, and s should be
 *                        initalized with the plan file name.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : get_token,GetBBox,GetAxis,GetTree,GetBoxFromPlan.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int InitPlan(Surf_Info *s)
{
	int error;
  char str[200],group[6],elem[6];
  FILE *fp;
  short TREE_FLAG,AXIS_FLAG,BBOX_FLAG;

  if (!s->plan_on) return(-1);
  if ((fp=fopen(s->plan_file,"r"))==NULL) 
  {
    //VDisplayDialogMessage("Could Not Open PLN file");
	  wxLogMessage("Could Not Open PLN file");
  
    return(-1);
  }
  
  if (get_token(fp,str,200)==-1) 
  {
    //VDisplayDialogMessage("Error Reading PLN file");
	  wxLogMessage("Error Reading PLN file");
    return(-1);
  }
   
  /*char strPLN[200];
  strcpy(strPLN, s->plan_file);
  char *pstr = strrchr(strPLN, '\\');
  pstr++;
  strcpy(pstr,str);*/
  strcpy(group,"");
  strcpy(elem, "");

  strcpy(s->file_name,str);  
  if ((s->fp=fopen(s->file_name,"rb"))==NULL) 
  {
    wxString msg(wxString::Format("Could not open %s.Ignoring file",
      s->file_name));
	wxLogMessage(msg);    
    return(-1);
  }
  else 
  {
    error=VReadHeader(s->fp,&s->vh,group,elem);
    if (error) {
      wxLogMessage(wxString::Format("error %d group %s element %s\n",error,group,elem));
    }
    if (error==104) return(-1);
    if (BuildSurfTree(s)==-1) 
	{
      fprintf(stderr,"Could Not build data structure for %s\n",s->file_name);
      return(-1);
    }
  }
  
  BBOX_FLAG=AXIS_FLAG=TREE_FLAG=FALSE;
  while (get_token(fp,str,200)!=-1)
  {
	  if (!strcmp(str,"BBOX")) 
	  {
		  if (BBOX_FLAG) 
		  {
			  //VDisplayDialogMessage("Bounding Box Multiply defined");
			  wxLogMessage("Bounding Box Multiply defined");	
			  return(-1);
		  }
		  if (GetBBox(fp,s)==-1)
			  return(-1);
		  else 
			  BBOX_FLAG=TRUE;
	  }
	  else if (!strcmp(str,"AXIS")) 
	  {
		  if (AXIS_FLAG) 
		  {
			//  VDisplayDialogMessage("Axis Multiply defined");
			  wxLogMessage("Axis Multiply defined");			
			  return(-1);
		  }
		  if (GetAxis(fp,s)==-1)
			  return(-1);
		  else 
			  AXIS_FLAG=TRUE;
	  }
	  else if (!strcmp(str,"TREE")) 
	  {
		  if (TREE_FLAG) 
		  {
//			  VDisplayDialogMessage("Tree Multiply defined");
			  wxLogMessage("Tree Multiply defined");			
			  return(-1);
		  }
		  if (GetTree(fp,s)==-1)
			  return(-1);
		  else 
			  TREE_FLAG=TRUE;
	  }
  }
  if (!TREE_FLAG) 
  {
	  //VDisplayDialogMessage("The TREE structure is missing in the plan");
	  wxLogMessage("The TREE structure is missing in the plan");			

	  return(-1);
  }
  if (!BBOX_FLAG) 
	  GetBoxFromPlan(s);

  return(0);

}


/*****************************************************************************
 * FUNCTION: get_token
 * DESCRIPTION: Reads a space-delimited string from a file.
 * PARAMETERS:
 *    string: Where the string will be put.
 *    file: The file to read from, must be opened for reading.
 *    string_size: The size of the array at string.
 * SIDE EFFECTS: The file position is advanced to the end of the string.
 * ENTRY CONDITIONS: The parameters must be valid.
 * RETURN VALUE:
 *    0: no error
 *    2: read error
 *    235: White space was not found after string_size-1 characters.
 * EXIT CONDITIONS: If an error occurs, the file position will be left at the
 *    character where the error occurred.
 * HISTORY:
 *    Created: 11/2/92 by Dewey Odhner.
 *
 *****************************************************************************/
int get_token( FILE *file, char string[], int string_size)
{
	int n, c;

	for (c=getc(file);;c=getc(file))
	{	switch (c)
		{	case EOF:
				string[0] = 0;
				return (-1);
			case ' ':
			case '\n':
			case '\t':
				continue;
		}
		break;
	}
	for (n=0; n<string_size; n++)
		switch (c)
		{	case EOF:
				string[n] = 0;
				return (-1);
			case ' ':
			case '\n':
			case '\t':
				string[n] = 0;
				ungetc(c, file);
				return (0);
			default:
				string[n] = c;
				c = getc(file);
				continue;
		}
	string[n-1] = 0;
	ungetc(c, file);
	return (-1);
}


int GetBoxFromSurf(Surf_Info *s)
{
  StructureInfo *st;
  int i,j;
  KMMATRIX D;
  KMPOINT in,out;


  st=&s->vh.str;
  for(j=0;j<3;j++) {
    D[j][0]=st->domain[3+j];      
    D[j][1]=st->domain[6+j];
    D[j][2]=st->domain[9+j];
    D[j][3]=st->domain[j];
  }
  D[3][0]=D[3][1]=D[3][2]=0.0; D[3][3]=1;
  /* Consider all 8 vertices */
  in[0]=s->surf_struct[0].min_x;
  in[1]=s->surf_struct[0].min_y;
  in[2]=s->surf_struct[0].min_z;
  TransformPoint(out,D,in);
  s->min_x=s->max_x=out[0];
  s->min_y=s->max_y=out[1];
  s->min_z=s->max_z=out[2];

  for(i=0;i<s->num_structs;i++) {
    st=&s->vh.str;
    for(j=0;j<3;j++) {
      D[j][0]=st->domain[12*i+3+j];      
      D[j][1]=st->domain[12*i+6+j];
      D[j][2]=st->domain[12*i+9+j];
      D[j][3]=st->domain[12*i+j];
    }
    D[3][0]=D[3][1]=D[3][2]=0.0; D[3][3]=1;
    /* Consider all 8 vertices */
    in[0]=s->surf_struct[i].min_x;
    in[1]=s->surf_struct[i].min_y;
    in[2]=s->surf_struct[i].min_z;
    UpdateLimits(s,D,in);

    in[0]=s->surf_struct[i].min_x;
    in[1]=s->surf_struct[i].min_y;
    in[2]=s->surf_struct[i].max_z;
    UpdateLimits(s,D,in);

    in[0]=s->surf_struct[i].min_x;
    in[1]=s->surf_struct[i].max_y;
    in[2]=s->surf_struct[i].min_z;
    UpdateLimits(s,D,in);

    in[0]=s->surf_struct[i].min_x;
    in[1]=s->surf_struct[i].max_y;
    in[2]=s->surf_struct[i].max_z;
    UpdateLimits(s,D,in);

    in[0]=s->surf_struct[i].max_x;
    in[1]=s->surf_struct[i].min_y;
    in[2]=s->surf_struct[i].min_z;
    UpdateLimits(s,D,in);

    in[0]=s->surf_struct[i].max_x;
    in[1]=s->surf_struct[i].min_y;
    in[2]=s->surf_struct[i].max_z;
    UpdateLimits(s,D,in);

    in[0]=s->surf_struct[i].max_x;
    in[1]=s->surf_struct[i].max_y;
    in[2]=s->surf_struct[i].min_z;
    UpdateLimits(s,D,in);

    in[0]=s->surf_struct[i].max_x;
    in[1]=s->surf_struct[i].max_y;
    in[2]=s->surf_struct[i].max_z;
    UpdateLimits(s,D,in);
  }

  s->diag= sqrt(SQR(s->max_x-s->min_x) + SQR(s->max_y-s->min_y) +
		SQR(s->max_z-s->min_z) )+1 ;
  s->cx= (s->max_x+s->min_x)/2.0;
  s->cy= (s->max_y+s->min_y)/2.0;
  s->cz= (s->max_z+s->min_z)/2.0;

  return(0);
  
}

	
#define SHOW_BBOX_ERROR {wxLogMessage("Error in Plan file at BBOX"); return(-1);}

int GetBBox(FILE *fp, Surf_Info *s)
{
  char str[100];

  if (get_token(fp,str,100)==-1) SHOW_BBOX_ERROR;
  if (strcmp(str,"{")) SHOW_BBOX_ERROR;

  if (get_token(fp,str,100)==-1) SHOW_BBOX_ERROR;
  if (sscanf(str,"%lf",&s->min_x)!=1) SHOW_BBOX_ERROR;
  if (get_token(fp,str,100)==-1) SHOW_BBOX_ERROR;
  if (sscanf(str,"%lf",&s->max_x)!=1) SHOW_BBOX_ERROR;
  if (get_token(fp,str,100)==-1) SHOW_BBOX_ERROR;
  if (sscanf(str,"%lf",&s->min_y)!=1) SHOW_BBOX_ERROR;
  if (get_token(fp,str,100)==-1) SHOW_BBOX_ERROR;
  if (sscanf(str,"%lf",&s->max_y)!=1) SHOW_BBOX_ERROR;
  if (get_token(fp,str,100)==-1) SHOW_BBOX_ERROR;
  if (sscanf(str,"%lf",&s->min_z)!=1) SHOW_BBOX_ERROR;
  if (get_token(fp,str,100)==-1) SHOW_BBOX_ERROR;
  if (sscanf(str,"%lf",&s->max_z)!=1) SHOW_BBOX_ERROR;
    
  if (get_token(fp,str,100)==-1) SHOW_BBOX_ERROR;
  if (strcmp(str,"}")) SHOW_BBOX_ERROR;

  /*
    printf("minx %lf maxx %lf miny %lf maxy %lf minz %lf maxz %lf\n",
	 s->min_x,s->max_x,s->min_y,s->max_y,s->min_z,s->max_z); */
	 
  s->diag= sqrt(SQR(s->max_x-s->min_x) + SQR(s->max_y-s->min_y) +
		SQR(s->max_z-s->min_z) ) +1;
  /* printf("diag %f\n",s->diag); */
  s->cx= (s->max_x+s->min_x)/2.0;
  s->cy= (s->max_y+s->min_y)/2.0;
  s->cz= (s->max_z+s->min_z)/2.0;


  return(0);
}

#define SHOW_AXIS_ERROR {wxLogMessage("Error in Plan file at AXIS"); return(-1);}

int GetAxis(FILE *fp, Surf_Info *s)
{
  int i,objs,num_objs;
  char str[100];

  if (get_token(fp,str,100)==-1) SHOW_AXIS_ERROR;
  if (sscanf(str,"%d",&num_objs)!=1) SHOW_AXIS_ERROR;

  if (get_token(fp,str,100)==-1) SHOW_AXIS_ERROR;
  if (strcmp(str,"{")) SHOW_AXIS_ERROR;

  for(i=0;i<num_objs;i++) {
    if (get_token(fp,str,100)==-1) SHOW_AXIS_ERROR;
    if (sscanf(str,"%d",&objs)!=1) SHOW_AXIS_ERROR;
    if (objs<0 || objs>=s->num_structs) SHOW_AXIS_ERROR;

    if (GetPoint(fp,s->surf_struct[objs].orig)==-1) SHOW_AXIS_ERROR;
    if (GetPoint(fp,s->surf_struct[objs].axis[0])==-1) SHOW_AXIS_ERROR;
    if (GetPoint(fp,s->surf_struct[objs].axis[1])==-1) SHOW_AXIS_ERROR;
    if (GetPoint(fp,s->surf_struct[objs].axis[2])==-1) SHOW_AXIS_ERROR;
    s->surf_struct[objs].AXIS_FLAG=TRUE;
  }

  if (get_token(fp,str,100)==-1) SHOW_AXIS_ERROR;
  if (strcmp(str,"}")) SHOW_AXIS_ERROR;

  return(0);

}


#define SHOW_TREE_ERROR {wxLogMessage("Error in Plan file at TREE"); return(-1);}

int GetTree(FILE *fp, Surf_Info *s)
{
  char str[100];
  QList *Q;
  int i,num_levels,level,children;
  Surf_Tree *pt;
  KMMATRIX tmp_mat;



  if (get_token(fp,str,100)==-1) SHOW_TREE_ERROR;
  if (sscanf(str,"%d",&num_levels)!=1) SHOW_TREE_ERROR;
  if (num_levels<2) SHOW_TREE_ERROR;
  
  if (get_token(fp,str,100)==-1) SHOW_TREE_ERROR;
  if (strcmp(str,"{")) SHOW_TREE_ERROR;

  s->tree=(Surf_Tree *)malloc(sizeof(Surf_Tree));
  s->tree->level=num_levels-1;
  s->tree->parent=NULL;
  MakeID(s->tree->transf);
  if (num_levels>2)  {
    s->tree_index=(short *)calloc(num_levels-2,sizeof(short));
    s->index_lock=(char *)calloc(num_levels-2,sizeof(char));
  }
  else  {
    s->tree_index=NULL;
    s->index_lock=NULL;
  }
  Q=NULL;
  PushQ(&Q,s->tree);
  while ((pt=PopQ(&Q))!=NULL) {
    if (get_token(fp,str,100)==-1) SHOW_TREE_ERROR;
    if (sscanf(str,"%d",&level)!=1) SHOW_TREE_ERROR;
    if (pt->level!=level-1) SHOW_TREE_ERROR;
    if (pt->level!=0) {
      if (get_token(fp,str,100)==-1) SHOW_TREE_ERROR;
      if (sscanf(str,"%d",&children)!=1) SHOW_TREE_ERROR;
      pt->children=children;
      if (pt->children>0) {
	pt->child=(Surf_Tree **)malloc(sizeof(Surf_Tree *)*pt->children);
	for(i=0;i<pt->children;i++) {
	  pt->child[i]=(Surf_Tree *)malloc(sizeof(Surf_Tree));
	  pt->child[i]->parent=pt;
	  pt->child[i]->level=pt->level-1;
	  PushQ(&Q,pt->child[i]);
	}
      }
      
    }
    else {
      if (get_token(fp,str,100)==-1) SHOW_TREE_ERROR;
      if (sscanf(str,"%d",&children)!=1) SHOW_TREE_ERROR;
      pt->surf_index=children;
      if (pt->surf_index<0 || pt->surf_index>=s->num_structs) SHOW_TREE_ERROR;
      pt->children=0;
      pt->child=NULL;
    }
    if (pt->parent!=NULL) {
      if (GetMatrix(fp,tmp_mat)==-1) SHOW_TREE_ERROR;
      MultMat(pt->transf,pt->parent->transf,tmp_mat);
    }
    else
      if (GetMatrix(fp,pt->transf)==-1) SHOW_TREE_ERROR;

  }

  if (get_token(fp,str,100)==-1) SHOW_TREE_ERROR;
  if (strcmp(str,"}")) SHOW_TREE_ERROR;

  return(0);
}

void PushQ(QList **Q, Surf_Tree *pt)     
{
  QList *q,*List;
  
  q=(QList *)malloc(sizeof(QList));
  q->pt=pt;  q->next=NULL;
  if (*Q==NULL) 
    *Q=q;
  else  {
    List=*Q;
    while (List->next!=NULL)
      List=List->next;
    List->next=q;
  }
  
}

Surf_Tree *PopQ(QList **Q)     
{
  Surf_Tree *pt;
  QList *Q1;
  
  if (*Q==NULL) return(NULL);
  Q1=*Q;
  *Q=Q1->next;
  pt=Q1->pt;
  free(Q1);
  return(pt);
}

int GetBoxFromPlan(Surf_Info *s )
{

  KMMATRIX D,P,M;
  StructureInfo *st;
  int i,j,k;
  Surf_Tree *pt;
  KMPOINT in,out;

  for(i=0;i<s->tree->level-1;i++)
    s->tree_index[i]=0;
  pt=GetSurfPointer(s,s->tree_index,P);
  st=&s->vh.str;

  if (pt!=NULL) {
    i=pt->child[0]->surf_index;
    for(j=0;j<3;j++) {
      M[j][0]=st->domain[12*i+3+j];      
      M[j][1]=st->domain[12*i+6+j];
      M[j][2]=st->domain[12*i+9+j];
      M[j][3]=st->domain[12*i+j];
    }
    M[3][0]=M[3][1]=M[3][2]=0.0; M[3][3]=1;
    MultMat(D,pt->child[0]->transf,M);
    
    /* Consider all 8 vertices */
    in[0]=s->surf_struct[i].min_x;
    in[1]=s->surf_struct[i].min_y;
    in[2]=s->surf_struct[i].min_z;
    TransformPoint(out,D,in);
    s->min_x=s->max_x=out[0];
    s->min_y=s->max_y=out[1];
    s->min_z=s->max_z=out[2];
  }
  else {
    s->min_x=s->min_y=s->min_z=-1.0;
    s->max_x=s->max_y=s->max_z= 1.0;
  }
  while (pt!=NULL) {
    for(k=0;k<pt->children;k++) {
      i=pt->child[k]->surf_index;
      for(j=0;j<3;j++) {
	M[j][0]=st->domain[12*i+3+j];      
	M[j][1]=st->domain[12*i+6+j];
	M[j][2]=st->domain[12*i+9+j];
	M[j][3]=st->domain[12*i+j];
      }
      M[3][0]=M[3][1]=M[3][2]=0.0; M[3][3]=1;
      MultMat(D,pt->child[k]->transf,M);
      
      /* Consider all 8 vertices */
      in[0]=s->surf_struct[i].min_x;
      in[1]=s->surf_struct[i].min_y;
      in[2]=s->surf_struct[i].min_z;
      UpdateLimits(s,D,in);
      
      in[0]=s->surf_struct[i].min_x;
      in[1]=s->surf_struct[i].min_y;
      in[2]=s->surf_struct[i].max_z;
      UpdateLimits(s,D,in);
      
      in[0]=s->surf_struct[i].min_x;
      in[1]=s->surf_struct[i].max_y;
      in[2]=s->surf_struct[i].min_z;
      UpdateLimits(s,D,in);
      
      in[0]=s->surf_struct[i].min_x;
      in[1]=s->surf_struct[i].max_y;
      in[2]=s->surf_struct[i].max_z;
      UpdateLimits(s,D,in);
      
      in[0]=s->surf_struct[i].max_x;
      in[1]=s->surf_struct[i].min_y;
      in[2]=s->surf_struct[i].min_z;
      UpdateLimits(s,D,in);
      
      in[0]=s->surf_struct[i].max_x;
      in[1]=s->surf_struct[i].min_y;
      in[2]=s->surf_struct[i].max_z;
      UpdateLimits(s,D,in);
      
      in[0]=s->surf_struct[i].max_x;
      in[1]=s->surf_struct[i].max_y;
      in[2]=s->surf_struct[i].min_z;
      UpdateLimits(s,D,in);
      
      in[0]=s->surf_struct[i].max_x;
      in[1]=s->surf_struct[i].max_y;
      in[2]=s->surf_struct[i].max_z;
      UpdateLimits(s,D,in);
    }
    if (GetNextStructureIndex(s,s->tree_index))
      pt=NULL;
    else 
      pt=GetSurfPointer(s,s->tree_index,P);
  }

  s->diag= sqrt(SQR(s->max_x-s->min_x) + SQR(s->max_y-s->min_y) +
		SQR(s->max_z-s->min_z) ) +1 ;
  s->cx= (s->max_x+s->min_x)/2.0;
  s->cy= (s->max_y+s->min_y)/2.0;
  s->cz= (s->max_z+s->min_z)/2.0;

  for(i=0;i<s->tree->level-1;i++)
    s->tree_index[i]=0;

  return(0);
}


void TransformPoint(KMPOINT KmNew, KMMATRIX M, KMPOINT old)
{
  KmNew[0]= M[0][0]*old[0]+M[0][1]*old[1]+M[0][2]*old[2]+M[0][3];
  KmNew[1]= M[1][0]*old[0]+M[1][1]*old[1]+M[1][2]*old[2]+M[1][3];
  KmNew[2]= M[2][0]*old[0]+M[2][1]*old[1]+M[2][2]*old[2]+M[2][3];
}

void UpdateLimits(Surf_Info *s, KMMATRIX D, KMPOINT in)
{

  KMPOINT out;

  TransformPoint(out,D,in);
  if (s->min_x > out[0]) s->min_x=out[0];
  if (s->max_x < out[0]) s->max_x=out[0];
  if (s->min_y > out[1]) s->min_y=out[1];
  if (s->max_y < out[1]) s->max_y=out[1];
  if (s->min_z > out[2]) s->min_z=out[2];
  if (s->max_z < out[2]) s->max_z=out[2];
  
}

int GetPoint(FILE *fp, double p[3])
{
  char str[100];
  
  if (get_token(fp,str,100)==-1) return(-1);
  if (strcmp(str,"{")) return(-1);

  if (get_token(fp,str,100)==-1) return(-1);
  sscanf(str,"%lf",p);
  if (get_token(fp,str,100)==-1) return(-1);
  sscanf(str,"%lf",p+1);
  if (get_token(fp,str,100)==-1) return(-1);
  sscanf(str,"%lf",p+2);
  
  if (get_token(fp,str,100)==-1) return(-1);
  if (strcmp(str,"}")) return(-1);
  return(0);
}

int GetMatrix(FILE *fp, KMMATRIX M)
{
  int i,j;
  char str[100];
  
  if (get_token(fp,str,100)==-1) return(-1);
  if (!strcmp(str,"{"))  {
    for(i=0;i<4;i++)
      for(j=0;j<4;j++) {
	if (get_token(fp,str,100)==-1) return(-1);
	if (sscanf(str,"%lf",&M[i][j])!=1) return(-1);
      }
    if (get_token(fp,str,100)==-1) return(-1);
    if (strcmp(str,"}")) return(-1);
    return(0);
  }
  if (!strcmp(str,"ID")) {
    MakeID(M);
    return(0);
  }
  return(-1);
}


/************************************************************************
 *
 *      FUNCTION        : Inv4Mat
 *
 *      DESCRIPTION     : Compute the inverse of a 4x4 KMMATRIX.
 *
 *      RETURN VALUE    : -1 - Cannot find an inverse.
 *                         0 - Successful completion.
 *
 *      PARAMETERS      : inmat - input 4x4 KMMATRIX.
 *                        outmat- computed 4x4 inverse KMMATRIX
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : inmat should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : Det4,Det3
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int Inv4Mat(KMMATRIX inmat, KMMATRIX outmat)     
{
  int i, j, k, l, m, n, ntemp;
  double mat[3][3], indet, temp;
  
  indet = Det4(inmat);
  if (indet==0) return(-1);
  for (i=0;i<4;i++) {
    for (j=0;j<4;j++) {
      m = 0;
      for (k=0;k<4;k++)
	if (i != k) {
	  n = 0;
	  for (l=0;l<4;l++)
	    if (j != l) {
	      mat[m][n] = inmat[k][l];
	      n++;
	    }
	  m++;
	}
      temp = -1.;
      ntemp = (i +j ) %2;
      if( ntemp == 0)  temp = 1.;
      outmat[j][i] = temp * Det3(mat)/indet;
    }
  }
  return(0);
}



/************************************************************************
 *
 *      FUNCTION        : Det3
 *
 *      DESCRIPTION     : returns the determinant of a 3x3 martrix.
 *
 *      RETURN VALUE    : determinant of the KMMATRIX.
 *
 *      PARAMETERS      : m - input KMMATRIX
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : m should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
double Det3(double m[3][3])     
{
  double result;
  
  result = 
    m[0][0]*m[1][1]*m[2][2] + m[0][1]*m[1][2]*m[2][0] +
      m[0][2]*m[1][0]*m[2][1] - m[0][2]*m[1][1]*m[2][0] -
	m[0][0]*m[1][2]*m[2][1] - m[0][1]*m[1][0]*m[2][2];
  return(result);
}
 
/************************************************************************
 *
 *      FUNCTION        : Det4
 *
 *      DESCRIPTION     : returns the determinant of a 4x4 martrix.
 *
 *      RETURN VALUE    : determinant of the KMMATRIX.
 *
 *      PARAMETERS      : m - input KMMATRIX
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : m should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
double Det4(KMMATRIX m)
{
  double result;
  
  result=
    m[0][0]*m[1][1]*m[2][2]*m[3][3]-m[0][1]*m[1][0]*m[2][2]*m[3][3]+
      m[0][1]*m[1][2]*m[2][0]*m[3][3]-m[0][2]*m[1][1]*m[2][0]*m[3][3]+
	m[0][2]*m[1][0]*m[2][1]*m[3][3]-m[0][0]*m[1][2]*m[2][1]*m[3][3]+
	  m[0][0]*m[1][2]*m[2][3]*m[3][1]-m[0][2]*m[1][0]*m[2][3]*m[3][1]+
	    m[0][2]*m[1][3]*m[2][0]*m[3][1]-m[0][3]*m[1][2]*m[2][0]*m[3][1]+
	      m[0][3]*m[1][0]*m[2][2]*m[3][1]-m[0][0]*m[1][3]*m[2][2]*m[3][1]+
		m[0][0]*m[1][3]*m[2][1]*m[3][2]-m[0][3]*m[1][0]*m[2][3]*m[3][2]+
		  m[0][1]*m[1][0]*m[2][3]*m[3][2]-m[0][0]*m[1][1]*m[2][0]*m[3][2]+
		    m[0][3]*m[1][1]*m[2][0]*m[3][2]-m[0][1]*m[1][3]*m[2][1]*m[3][2]+
		      m[0][1]*m[1][3]*m[2][2]*m[3][0]-m[0][3]*m[1][1]*m[2][2]*m[3][0]+
			m[0][2]*m[1][1]*m[2][3]*m[3][0]-m[0][1]*m[1][2]*m[2][3]*m[3][0]+
			  m[0][3]*m[1][2]*m[2][1]*m[3][0]-m[0][2]*m[1][3]*m[2][1]*m[3][0];
  return(result);
}



int GetNextToken(FILE *fp, char *str)
{
  int c;
  
  
  while ((c=getc(fp))!=EOF && isspace((int)c)) ;
  if (c==EOF) return(-1);
  ungetc(c,fp);
  if (fscanf(fp,"%s",str)!=1)
    return(-1);
  /* printf("%s\n",str); */
 return(0);
  
}





/************************************************************************
 *
 *      FUNCTION        : ReleaseAllStructs
 *
 *      DESCRIPTION     : This would free up all the space allocated in 
 *                        the surf structures.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : None.
 *
 *      SIDE EFFECTS    : All image windows that were put up will be
 *                        destroyed.
 *
 *      ENTRY CONDITION : surf should be correctly initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : ReleaseAll,FreeTree
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/

//void ReleaseAllStructs()
//{
//  int i,j,k;
//  int num_surf=1;
//  
//  if (num_surf<1) return;
//  ReleaseAll();
//  for(i=0;i<num_surf;i++) {
//    for(j=0;j<surf[i].num_structs;j++){
//      for(k=0;k<surf[i].surf_struct[j].slices;k++){
//	free(surf[i].surf_struct[j].node[k]);
//	free(surf[i].surf_struct[j].count[k]);
//      }
//      free(surf[i].surf_struct[j].node);
//      free(surf[i].surf_struct[j].count);
//      if (surf[i].surf_struct[j].seg_map) free(surf[i].surf_struct[j].seg_map);
//      if (surf[i].surf_struct[j].seg_plane_map) free(surf[i].surf_struct[j].seg_plane_map);
//    }
//    free(surf[i].surf_struct);
//    if (surf[i].tree!=NULL) {
//      FreeTree(surf[i].tree);
//      free(surf[i].tree);
//    }
//    if (surf[i].WINDOW_ON) VDeleteImageSubwindow(surf[i].win);
//  }
//  free(surf);
//  
//}

/************************************************************************
 *
 *      FUNCTION        : FreeTree
 *
 *      DESCRIPTION     : This would traverse a tree and free up each
 *                        node bottom up.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : t - tree structure
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : the tree should be properly initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
void FreeTree(Surf_Tree *t)     
{
  int i;
  
  if (t->children==0) return;
  for(i=0;i<t->children;i++)
    FreeTree(t->child[i]);
  free(t->child);
  
  
}

/************************************************************************
 *
 *      FUNCTION        : BuildOrder
 *
 *      DESCRIPTION     : Given a partial tree and an ordered set of 
 *                        parameter vectors and the begining and the
 *                        end of this partial order it builds a partial set
 *                        of the tree that coresponds to the partial set 
 *                        described by  st and end. This calls it self
 *                        recursively to build a complete tree.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : parent - parent of the tree being generated.
 *                        tree   - the three being generated.
 *                        sorted_list - sorted list of parameter vectors.
 *                        dim    - size of each parameter vector.
 *                        st     - begining of the partial order list.
 *                        end    - end of the partial order list.
 *
 *      SIDE EFFECTS    : An extra level is added to the tree if the last
 *                        level does not correspond to objects.
 *
 *      ENTRY CONDITION : parameters should have meaningful values.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/

void BuildOrder(Surf_Tree *parent, Surf_Tree **tree, float * sorted_list, int dim, int st, int end, int level)
{
  int i,new_st,new_end,ind,children;
  float val;

  *tree=(Surf_Tree *)malloc(sizeof(Surf_Tree));
  (*tree)->parent=parent;
  (*tree)->surf_index=-1;
  MakeID((*tree)->transf);
  (*tree)->level=level;

  if (dim==DIM) {
    if (level==0) { /* last level is objects */
      (*tree)->children=0;
      (*tree)->child=NULL;
      (*tree)->surf_index=(int)sorted_list[st*(DIM+1)+DIM];
    }
    else {
      (*tree)->children=1;
      (*tree)->child=(Surf_Tree **)malloc(sizeof(Surf_Tree *));
      (*tree)->child[0]=(Surf_Tree *)malloc(sizeof(Surf_Tree));
      MakeID((*tree)->child[0]->transf);
      (*tree)->child[0]->parent= *tree;
      (*tree)->child[0]->children= 0;
      (*tree)->child[0]->child= NULL;
      (*tree)->child[0]->surf_index=(int)sorted_list[st*(DIM+1)+DIM];
    }      
    return;
  }
  if (st<=end) {
    if (dim!=DIM-1) {
      ind=st*(DIM+1);
      for(children=1,val=sorted_list[ind+ORDER[dim]],i=st+1;i<=end;i++) {
	ind=i*(DIM+1);
	if (val!=sorted_list[ind+ORDER[dim]]) {
	  children++;
	  val=sorted_list[ind+ORDER[dim]];
	}
      }
      (*tree)->children=children;
      (*tree)->child=(Surf_Tree **)malloc(sizeof(Surf_Tree *)*children);
      ind=st*(DIM+1);
      new_st=st;
      for(val=sorted_list[ind+ORDER[dim]],i=st+1,children=0;i<=end;i++) {
	ind=i*(DIM+1);
	if (val!=sorted_list[ind+ORDER[dim]]) {
	  new_end=i-1;
	  BuildOrder(*tree,(*tree)->child+children,sorted_list,dim+1,new_st,new_end,level-1);
	  children++;
	  new_st=i;
	  val=sorted_list[ind+ORDER[dim]];
	}
      }
    }
    else {
      ind=st*(DIM+1);
      children=end-st+1;
      (*tree)->children=children;
      (*tree)->child=(Surf_Tree **)malloc(sizeof(Surf_Tree *)*children);
      ind=st*(DIM+1);
      new_st=st;
      for(val=sorted_list[ind+ORDER[dim]],i=st+1,children=0;i<=end;i++) {
	ind=i*(DIM+1);
	new_end=i-1;
	BuildOrder(*tree,(*tree)->child+children,sorted_list,dim+1,new_st,new_end,level-1);
	children++;
	new_st=i;
	val=sorted_list[ind+ORDER[dim]];
      }
    }
    
    new_end=end;
    BuildOrder(*tree,(*tree)->child+children,sorted_list,dim+1,new_st,new_end,level-1);
  }
}
	


/************************************************************************
 *
 *      FUNCTION        : cmp_pv
 *
 *      DESCRIPTION     : This is used by the qsort function to obtain
 *                        the ordering of the parameter vectors. The ordering
 *                        is based on the parameter vector type and the 
 *                        parameter values of the coresspoing sets.
 *
 *      RETURN VALUE    : 0 - equal, 1 - greater, -1 - smaller.
 *
 *      PARAMETERS      : s1,s2 - parameter vectors to be compared.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : s1,s2 should be defined. gobal variables DIM
 *                        and ORDER should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
static int cmp_pv(const void *Ins1, const void * Ins2)
{
  int i;
  float *s1 = (float *)Ins1;
  float *s2 = (float *)Ins2;

  for(i=0;i<DIM;i++) 
  {
    if ((float)(s1[ORDER[i]]) > (float)(s2[ORDER[i]]))
      return(1);
    else if ((float)(s1[ORDER[i]]) < (float)(s2[ORDER[i]]) )
      return(-1);
  }
  return(0);

}



/************************************************************************
 *
 *      FUNCTION        : GetSurfPointer
 *
 *      DESCRIPTION     : Given a surface strucutre and an index set into
 *                        the tree heirarchy this would return a pointer to
 *                        the corresponding node of the tree and the 
 *                        transformation KMMATRIX associated with the node. If
 *                        any index value is -1 the tree is traversed only 
 *                        upto that level.
 *
 *      RETURN VALUE    : pointer to the node if one exists NULL otherwise.
 *
 *      PARAMETERS      : srf - surface strcutre of reference.
 *                        ind - index set of the current node.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : ind and srf should be properly initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : GetPt
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
Surf_Tree *GetSurfPointer(Surf_Info *srf, short *ind, KMMATRIX M)
{
  int i,j;
 

  if (ind==NULL) {
    if (srf->tree!=NULL)
      for(i=0;i<4;i++)
	for(j=0;j<4;j++)
	  M[i][j]=srf->tree->transf[i][j];
    else
      MakeID(M);
    return(srf->tree);
  }
  if (srf->tree==NULL) {
    MakeID(M);
    return(NULL);
  }
  return(GetPt(srf->tree,ind,M));

}





/************************************************************************
 *
 *      FUNCTION        : GetPt
 *
 *      DESCRIPTION     : Given a tree of level n and an index set of size
 *                        n this would recursively traverse the tree until
 *                        a leaf node is reached. At this point the function 
 *                        returns a pointer to this leaf node and initializes
 *                        the KMMATRIX M to contain the transformation KMMATRIX 
 *                        of that node. 
 *
 *      RETURN VALUE    : pointer to the leaf node if successfull NULL if not.
 *
 *      PARAMETERS      : tr - tree structure.
 *                        index - index set pointing to a node in the tree.
 *
 *      SIDE EFFECTS    : An index set can be terminated halfway through the
 *                        traversal by initializing the corresponding index
 *                        to -1. This would return the pointer to the current 
 *                        level in the tree and the corresponding KMMATRIX.
 *
 *      ENTRY CONDITION : tr and index should be properly inialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
Surf_Tree *GetPt(Surf_Tree *tr,short *index, KMMATRIX M)
{
  int i,j;
  Surf_Tree *pt;


  if (tr->level==1 || index[0]==-1) {
    for(i=0;i<4;i++)
      for(j=0;j<4;j++)
	M[i][j]= tr->transf[i][j];
    return(tr);
  }
  if (index[0]<0 || index[0] >=tr->children) {
    MakeID(M);
    return(NULL);
  }
  pt=GetPt(tr->child[index[0]],index+1,M);
  return(pt);

}
  

/************************************************************************
 *
 *      FUNCTION        : FindNextStructureIndex
 *
 *      DESCRIPTION     : Given a surface structure and an index set 
 *                        pointing to a current position into the tree
 *                        and levels in the tree that are locked it
 *                        finds the index set of the next instance.
 *
 *      RETURN VALUE    : 0 - if successfull
 *
 *      PARAMETERS      : srf - surface structure
 *                        index - current index set.
 *                        ilock - levels in the tree that are locked.
 *
 *      SIDE EFFECTS    :  if successful index is modified.
 *
 *      ENTRY CONDITION : index, srf, and ilock should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : GetNextStrucutreIndex.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int FindNextStructureIndex(Surf_Info *srf, short *index, char *ilock)
{

  short i,*temp_index;
  int error;

  temp_index=(short *)malloc(sizeof(short)*(srf->tree->level-1));
  for(i=0;i<srf->tree->level-1;i++)
    temp_index[i]=index[i];
  
  error=GetNextStructureIndex(srf,index);
  if (error) {
    for(i=0;i<srf->tree->level-1;i++)
      index[i]=temp_index[i];
    free(temp_index);
    return(error);
  }

  for(i=0;i<srf->tree->level-1;i++)
    if (ilock[i]==1 && temp_index[i]!=index[i]) {
      error=1;
    }
  while (error) {
    error=GetNextStructureIndex(srf,index);
    if (error) {
      for(i=0;i<srf->tree->level-1;i++)
	index[i]=temp_index[i];
      free(temp_index);
      return(error);
    }
    
    for(i=0;i<srf->tree->level-1;i++)
      if (ilock[i]==1 && temp_index[i]!=index[i]) {
	error=1;
      }
  }

  return(error);

}





/************************************************************************
 *
 *      FUNCTION        : GetNextStructureIndex
 *
 *      DESCRIPTION     : Given a surface structure and an index set 
 *                        pointing to a current position into the tree
 *                        finds the index set of the next instance 
 *                        irespective of whether a level is locked or not.
 *
 *      RETURN VALUE    : 0 - if successfull
 *
 *      PARAMETERS      : srf - surface structure
 *                        index - current index set.
 *
 *      SIDE EFFECTS    :  if successful index is modified.
 *
 *      ENTRY CONDITION : index, srf, and ilock should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : GetSurfPointer
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int GetNextStructureIndex(Surf_Info *srf, short *index)
{
  short i,*temp_index;
  Surf_Tree *ptr,*parent;
  KMMATRIX M;

  if (index==NULL ) return(-1);
  temp_index=(short *)malloc(sizeof(short)*(srf->tree->level-1));
  for(i=0;i<srf->tree->level-1;i++)
    temp_index[i]=index[i];
  ptr=GetSurfPointer(srf,index,M);
  parent=ptr->parent;
  while (parent!=NULL) {
    if (index[srf->tree->level-parent->level] < parent->children-1) {
      index[srf->tree->level-parent->level]++;
      free(temp_index);
      return(0);
    }
    index[srf->tree->level-parent->level]=0;
    parent=parent->parent;
  }
  for(i=0;i<srf->tree->level-1;i++)
    index[i]=temp_index[i];
  free(temp_index);
  return(-1);

}


/************************************************************************
 *
 *      FUNCTION        : FindPrevStructureIndex
 *
 *      DESCRIPTION     : Given a surface structure and an index set 
 *                        pointing to a current position into the tree
 *                        and levels in the tree that are locked it
 *                        finds the index set of the previous instance.
 *
 *      RETURN VALUE    : 0 - if successfull
 *
 *      PARAMETERS      : srf - surface structure
 *                        index - current index set.
 *                        ilock - levels in the tree that are locked.
 *
 *      SIDE EFFECTS    :  if successful index is modified.
 *
 *      ENTRY CONDITION : index, srf, and ilock should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : GetPrevStrucutreIndex.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int FindPrevStructureIndex(Surf_Info *srf,short * index, char *ilock)
{

  short i,*temp_index;
  int error;

  temp_index=(short *)malloc(sizeof(short)*(srf->tree->level-1));
  for(i=0;i<srf->tree->level-1;i++)
    temp_index[i]=index[i];
  
  error=GetPrevStructureIndex(srf,index);
  if (error) {
    for(i=0;i<srf->tree->level-1;i++)
      index[i]=temp_index[i];
    free(temp_index);
    return(error);
  }

  for(i=0;i<srf->tree->level-1;i++)
    if (ilock[i]==1 && temp_index[i]!=index[i]) {
      error=1;
    }
  while (error) {
    error=GetPrevStructureIndex(srf,index);
    if (error) {
      for(i=0;i<srf->tree->level-1;i++)
	index[i]=temp_index[i];
      free(temp_index);
      return(error);
    }
    
    for(i=0;i<srf->tree->level-1;i++)
      if (ilock[i]==1 && temp_index[i]!=index[i]) {
	error=1;
      }
  }

  return(error);

}





/************************************************************************
 *
 *      FUNCTION        : GetPrevStructureIndex
 *
 *      DESCRIPTION     : Given a surface structure and an index set 
 *                        pointing to a current position into the tree
 *                        finds the index set of the previous instance 
 *                        irespective of whether a level is locked or not.
 *
 *      RETURN VALUE    : 0 - if successfull
 *
 *      PARAMETERS      : srf - surface structure
 *                        index - current index set.
 *
 *      SIDE EFFECTS    :  if successful index is modified.
 *
 *      ENTRY CONDITION : index, srf, and ilock should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : GetSurfPointer
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int GetPrevStructureIndex(Surf_Info *srf, short *index)
{

  short i,*temp_index;
  Surf_Tree *ptr,*parent;
  KMMATRIX M;

  if (index==NULL) return(-1);
  temp_index=(short *)malloc(sizeof(short)*(srf->tree->level-1));
  for(i=0;i<srf->tree->level-1;i++)
    temp_index[i]=index[i];
  ptr=GetSurfPointer(srf,index,M);
  parent=ptr->parent;
  while (parent!=NULL) {
    if (index[srf->tree->level-parent->level] != 0) {
      index[srf->tree->level-parent->level]--;
      free(temp_index);
      return(0);
    }
    index[srf->tree->level-parent->level]=parent->children-1;
    parent=parent->parent;
  }
  for(i=0;i<srf->tree->level-1;i++)
    index[i]=temp_index[i];
  free(temp_index);
  return(-1);

}
  
/************************************************************************
 *
 *      FUNCTION        : MakeID
 *
 *      DESCRIPTION     : Given a KMMATRIX (4x4) initialized it be an
 *                        Identity KMMATRIX.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : m - KMMATRIX
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : None.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
void MakeID(KMMATRIX m)
{

  m[0][0]=m[1][1]=m[2][2]=m[3][3]=1.0;
  m[0][1]=m[0][2]=m[0][3]=m[1][0]=m[1][2]=m[1][3]=0.0;
  m[2][0]=m[2][1]=m[2][3]=m[3][0]=m[3][1]=m[3][2]=0.0;
}



/************************************************************************
 *
 *      FUNCTION        : MultMat
 *
 *      DESCRIPTION     : This multiplies two 4x4 matrices.
 *                        
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : out - the result KMMATRIX.
 *                        mat1,mat2 - matrices to be multiplied.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : mat1 and mat2 should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
void MultMat(KMMATRIX out, KMMATRIX mat1, KMMATRIX mat2)     
{
  
  int i,j;
  
  for(i=0;i<4;i++)
    for(j=0;j<4;j++)
      out[i][j]= mat1[i][0]*mat2[0][j] + mat1[i][1]*mat2[1][j]+
	mat1[i][2]*mat2[2][j]+mat1[i][3]*mat2[3][j];
}


int init_defaults()
{

  FILE *fp;
  DEFS t;
  int c;

  if ((fp=fopen("SURFACE.DEF","rb"))==NULL) {
    write_defaults();
    return(1);
  }
  
  if ((c=fscanf(fp,"%hu %hu %hu",&t.r,&t.g,&t.b))!=3) {
    printf("Could not Read 1 %d\n",c);;
    return(1);
  }
  
  if (fscanf(fp,"%f %f %f %f %f %f",&t.opacity,&t.spec_ratio,&t.spec_exp,&t.spec_div,
	     &t.diff_exp,&t.diff_div)!=6) {
    printf("Could not Read 2\n");;
    return(1);
  }
  
  if (fscanf(fp,"%hu %hu %hu",&t.amb_r,&t.amb_g,&t.amb_b)!=3) {
    printf("Could not Read 3\n");;
    return(1);
  }
  if (fscanf(fp,"%hu %hu %hu",&t.bg_r,&t.bg_g,&t.bg_b)!=3) {
    printf("Could not Read 4\n");;
    return(1);
  }
  if (fscanf(fp,"%hu %hu %hu",&t.box_r,&t.box_g,&t.box_b)!=3) {
    printf("Could not Read 5\n");;
    return(1);
  }

  if (t.r <=100 && t.g <=100  && t.b <=100 ) {
    Defaults.r=t.r; Defaults.g=t.g; Defaults.b=t.b;
  }
  if (t.spec_ratio >= 0.0 && t.spec_ratio <=1.0) Defaults.spec_ratio=t.spec_ratio;
  if (t.spec_exp >=0.0 && t.spec_exp<=10.0) Defaults.spec_exp=t.spec_exp;
  if (t.spec_div >=1.0 && t.spec_exp<=10.0) Defaults.spec_div=t.spec_div;
  if (t.diff_exp >=0.0 && t.diff_exp<=2.0) Defaults.diff_exp=t.diff_exp;
  if (t.diff_div >=1.0 && t.diff_exp<=10.0) Defaults.diff_div=t.diff_div;
//  if (t.amb_r <=0xFFFF && t.amb_g <=0xFFFF && t.amb_b <=0xFFFF) {
    Defaults.amb_r=t.amb_r; Defaults.amb_g=t.amb_g; Defaults.amb_b=t.amb_b;
//  }
//  if (t.bg_r <=0xFFFF && t.bg_g <=0xFFFF && t.bg_b <=0xFFFF) {
    Defaults.bg_r=t.bg_r; Defaults.bg_g=t.bg_g; Defaults.bg_b=t.bg_b;
//  }
//  if (t.box_r <=0xFFFF && t.box_g <=0xFFFF && t.box_b <=0xFFFF ) {
    Defaults.box_r=t.box_r; Defaults.box_g=t.box_g; Defaults.box_b=t.box_b;
//  }

  return(0);
}

int write_defaults()
{

  FILE *fp;

  if ((fp=fopen("SURFACE.DEF","wb"))==NULL) {
    return(1);
  }
  
  fprintf(fp,"%u %u %u\n",Defaults.r,Defaults.g,Defaults.b);
  fprintf(fp,"%f %f %f %f %f %f\n",Defaults.opacity,Defaults.spec_ratio,
	     Defaults.spec_exp,Defaults.spec_div,
	     Defaults.diff_exp,Defaults.diff_div);
  fprintf(fp,"%u %u %u\n",Defaults.amb_r,Defaults.amb_g,Defaults.amb_b);
  fprintf(fp,"%u %u %u\n",Defaults.bg_r,Defaults.bg_g,Defaults.bg_b);
  fprintf(fp,"%u %u %u\n",Defaults.box_r,Defaults.box_g,Defaults.box_b);

  return(0);

}

