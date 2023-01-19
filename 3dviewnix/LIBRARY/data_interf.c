/*
  Copyright 1993-2013, 2015, 2017 Medical Image Processing Group
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

 
 
 
/************************************************************************
 *                              SAMPLE                                  *
 *                                                                      *
 *      Function        : name_of_function                              *
 *      Description     : A brief statement of purpose of this function.*
 *      Return Value    : The meaning of the return value of the        *
 *                        function, if any.                             *
 *      Parameters      :                                               *
 *        first_param   : the meaning of the first argument,            *
 *                        and any constraints on it.                    *
 *        second_param  : ....                                          *
 *      Side effects    : Any global or static variables(*) changed by  *
 *                        a call to this function, and any output       *
 *                        performed.                                    *
 *      Entry condition : Any global or static variables required by    *
 *                        this function to be initialized, and any      *
 *                        files, server connections, or windws reqd.    *
 *                        for input; or an initialization function to   *
 *                        be called first which ensures the former.     *
 *      Related funcs   : Any related functions.                        *
 *      History         : Creation date and name of programmer;         *
 *                        description of each change, date of change,   *
 *                        and name of programmer.                       *
 *                                                                      *
 ************************************************************************/

/************************************************************************
 *                                                                      *
 *      Filename  : data_interf.c                                       *
 *      Ext Funcs : VGetHeaderLength, VReadHeader, VReadData,           *
 *                  VWriteHeader, VWriteData, VCloseData,               *
 *                  VInitializeTape, VSeekData.                         *
 *      Int Funcs : v_read_recognition_code, v_initialize_validity_flag,*
 *                  v_ReadHeader_10, v_read_data_set_type,v_read_items, *
 *                  v_read_general_10,v_read_scene_10,                  *
 *                  v_read_structure_10,v_read_display_10,v_check_item, *
 *                  v_file_to_string,v_assign_data_error_code,          *
 *                  v_file_to_strptr,v_decode_strptr,v_file_to_short,   *
 *                  v_file_to_int,v_file_to_ints,v_file_to_shorts,      *
 *                  v_file_to_shorts1,v_file_to_float,v_file_to_floats, *
 *                  v_file_to_floats1,v_WriteHeader_10,                 *
 *                  v_write_general_10,v_write_scene_10,                *
 *                  v_write_structure_10,v_count_samples_memory_space,  *
 *                  v_write_display_10,v_cvt_delimiter,v_string_to_file,*
 *                  v_short_to_file,v_int_to_file,v_float_to_file,      *
 *                  v_ints_to_file,v_floats_to_file,v_shorts_to_file,   *
 *                  v_write_len,v_close_header,v_ReadData,v_WriteData.  *
 *                                                                      *
 ************************************************************************/

#define MAX_GRP_LEN 0xffffffff

#include "Vlibrary.h"
#include <assert.h>     /* gjg */
#include <stdlib.h>
#include <sys/types.h>
#if ! defined (WIN32) && ! defined (_WIN32)
    #include <unistd.h>
#endif
#include "3dv.h"
#ifdef FTP_EXPIRATION
    #include <time.h>
#endif
 

ItemInfo acr_items[100] ;
char lib_group_error[5], lib_element_error[5] ;

static int v_assign_data_error_code ( int* error1, ItemInfo* item );
static int v_close_header ( FILE* fp, long offset, int grplen, int type );
static int v_count_samples_memory_space ( int* total, short* samples, short dim );
static int v_cvt_delimiter ( char* string1, char* string2, char del );
static int v_decode_strptr ( char* label, Char30* axis_label, int items );
static int v_file_to_float ( FILE* fp, float* num, ItemInfo* item, int* read_next,
    unsigned short* group, unsigned int* valid, int* error1 );
static int v_file_to_floats ( FILE* fp, float** nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 );
static int v_file_to_floats1 ( FILE* fp, float* nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 );
static int v_file_to_ints ( FILE* fp, int** nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 );
static int v_file_to_short ( FILE* fp, short* num, ItemInfo* item, int* read_next,
    unsigned short* group, unsigned int* valid, int* error1 );
static int v_file_to_shorts ( FILE* fp, short** nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group,unsigned int* valid, int* error1 );
static int v_file_to_shorts1 ( FILE* fp, short* nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 );
static int v_file_to_string ( FILE* fp, char* string, int max, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 );
static int v_file_to_strptr ( FILE* fp, char** strptr, ItemInfo* item, int* read_next,
    unsigned short* group, unsigned int* valid, int* error1 );
static int v_float_to_file ( FILE* fp, float num, unsigned int valid, int* len,
    ItemInfo* item, int* error1 );
static int v_floats_to_file ( FILE* fp, float* num, unsigned int valid, int item,
    int* len, ItemInfo* spc_item, int* error1 );
static int v_initialize_validity_flag ( ViewnixHeader* vh );
static int v_ints_to_file ( FILE* fp, int* num, unsigned int valid, int item,
    int* len, ItemInfo* spc_item, int* error1 );
static int v_read_data_set_type ( FILE* fp, int* type );
static int v_read_display_10 ( FILE* fp, DisplayInfo* dsp, int* read_next,
    unsigned short* group, int* items, int* error1 );
static int v_read_general_10 ( FILE* fp, GeneralInfo* gen, int* read_next,
    unsigned short* group, int* items, int* error1 );
static int v_read_items ( char* version, int type );
static int v_read_recognition_code ( FILE* fp, char* recognition_code );
static int v_read_scene_10 ( FILE* fp, SceneInfo* scn, int* read_next,
    unsigned short* group, int* items, int* error1 );
static int v_read_structure_10 ( FILE* fp, StructureInfo* str, int* read_next,
    unsigned short* group, int* items, int* error1 );
static int v_ReadHeader_10 ( FILE* fp, ViewnixHeader* vh );
static int v_short_to_file ( FILE* fp, short num, unsigned int valid, int* len,
    ItemInfo* item, int* error1 );
static int v_shorts_to_file ( FILE* fp, short* num, unsigned int valid, int item,
    int* len, ItemInfo* spc_item, int* error1 );
static int v_string_to_file ( FILE* fp, char* string, unsigned int valid, int* len,
    ItemInfo* item, int* error1 );
static int v_write_display_10 ( FILE* fp, DisplayInfo* dsp, long* offset, int* len,
    int* items, int* error1 );
static int v_write_general_10 ( FILE* fp, GeneralInfo* gen, long* offset, int* len,
    int* items, int* error1 );
static int v_write_len ( FILE* fp, long* skip, int* grplen, short* grp );
static int v_write_scene_10 ( FILE* fp, SceneInfo* scn, long* offset, int* len,
    int* items, int* error1 );
static int v_write_structure_10 ( FILE* fp, StructureInfo* str, long* offset,
    int* len, int* items, int* error1 );
static int v_WriteHeader_10 ( FILE* fp, ViewnixHeader* vh );

/************************************************************************
 *                                                                      *
 *      Function        : VGetHeaderLength                              *
 *      Description     : This function computes the header length      *
 *                        for the file specified. If the file header    *
 *                        format is not correct, this function will     *
 *                        return 100, else will return the length of    *
 *                        header to the hdrlen and return 0 to this     *
 *                        function.                                     *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         hdrlen - length of the header.               *
 *      Side effects    : None.                                         *
 *      Entry condition : If fp or hdrlen is NULL, this function will   *
 *                        print a proper message to the standard error  *
 *                        stream, produce a core dump file, and exit    *
 *                        from the current process.                     *
 *      Related funcs   : VReadHeader, VWriteHeader, VWriteData,        *
 *                        VReadData.                                    *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
int VGetHeaderLength ( FILE* fp, int* hdrlen )
{
        int len, grplen, error ;
        unsigned short grp, ele ;
        long skip ;
        /*char msg[80] ; */

        if (fp == NULL) 
            v_print_fatal_error("VGetHeaderLength",
                "The specified file pointer should not be NULL.",0) ;
        if (hdrlen == NULL) 
            v_print_fatal_error("VGetHeaderLength",
                "The pointer of hdrlen should not be NULL.",0) ;
        grplen=0 ;
        skip=0 ;
        grp=0;
        ele=0 ;
        *hdrlen=0 ;
        while (TRUE) { 
            error=fseek(fp,skip,0) ;
            if (error == -1) return(5) ;
            error= v_ReadData((char *)&grp,sizeof(unsigned short),1,fp) ;
            if (error == 0) return(2) ;
            error=v_ReadData((char *)&ele,sizeof(unsigned short),1,fp) ;
            if (error == 0) return(2) ;
            error=v_ReadData((char *)&len,sizeof(int),1,fp) ;
            if (error == 0) return(2) ;
            error=v_ReadData((char *)&grplen,sizeof(int),1,fp) ;
            if (error == 0) return(2) ;
            if (ele != 0 || len != 4 )
                return(100) ;
            if ((grp != 0x7FE0) && (grp != 0x8001) && (grp != 0x8021)) {
                *hdrlen= *hdrlen+grplen+12 ;
                skip= *hdrlen ;
            } 
            else {
                *hdrlen += 20 ;
                return(0) ;
            }
        }
}

/************************************************************************
 *                                                                      *
 *      Function        : VReadHeader                                   *
 *      Description     : This function will read the file header       *
 *                        information from disk file user specifies to  *
 *                        the struct vh according to the data set type  *
 *                        stored in the file header.                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *                         7 - 3DVIEWNIX directory pathname not given   *
 *                             in environment variable.                 *
 *                         100 - incorrect 3DVIEWNIX file format.       *
 *                         102 - invalid data set type.                 *
 *                         103 - can not open specification file.       *
 *                         104 - invalid value in the file header for   *
 *                               Type 1.                                *
 *                         106 - invalid value in the file header for   *
 *                               Type 1D.                               *
 *                         107 - invalid value in the file header for   *
 *                               2, 2D, or 3.                           *
 *                         105 - invalid recognition code.              *
 *      Parameters      :  vh - pointer to the struct where the function*
 *                              puts the information.                   *
 *                         group - the group number of the error item.  *
 *                         element - the element number of the error    *
 *                               item.                                  *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader, VGetHeaderLength.               *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
int VReadHeader( FILE* fp, ViewnixHeader* vh, char group[5], char element[5] )
{
        char recognition_code[80] ;
        int error ;


#ifdef FTP_EXPIRATION
        {
          time_t tim;
          char CUR_TIME[100];
 
          time(&tim);
          strftime(CUR_TIME,100,"%Y%m",localtime(&tim));
          if (atoi(CUR_TIME)>FTP_EXPIRATION)
            v_print_fatal_error("VSetup",
                                "The FTP VERSION has expired",FTP_EXPIRATION);
        }
#endif

        if (fp == NULL) 
            v_print_fatal_error("VReadHeader",
                "The specified file pointer should not be NULL.",0) ;
        if (vh == NULL) 
            v_print_fatal_error("VReadHeader",
                "The pointer of vh should not be NULL.",0) ;
        if (group == NULL) 
            v_print_fatal_error("VReadHeader",
                "The pointer of group should not be NULL.",0) ;
        if (element == NULL) 
            v_print_fatal_error("VReadHeader",
                "The pointer of element should not be NULL.",0) ;
        error=v_read_recognition_code(fp,recognition_code) ;
        if (error != 0) return(error) ;
        strcpy(lib_group_error,"") ;
        strcpy(lib_element_error,"") ;
        v_initialize_validity_flag(vh) ;
        if (strcmp(recognition_code,"VIEWNIX1.0") == 0) {
            error=v_ReadHeader_10(fp,vh) ;
            strcpy(group,lib_group_error) ;
            strcpy(element,lib_element_error) ;
            return(error) ;
        }
        return(105) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_read_recognition_code                       *
 *      Description     : This function read the input file header to   *
 *                        get the value of the recognition code.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *                         100 - incorrect 3DVIEWNIX file header format.*
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         recognition_code - Returns the recognition   *
 *                         code stored in the input file header.        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_read_recognition_code ( FILE* fp, char* recognition_code )
{
        unsigned short item[2] ; /* item[0] - group #, item[1] - elemt # */
        int len[2], error ;

        strcpy(recognition_code,"") ;
        while (TRUE) {
            error=v_ReadData((char*)item,sizeof(short),2,fp) ;
            if (error == 0) return(2) ;
            while (item[0] < 0x0008) {
                error=v_ReadData((char*)len,sizeof(int),2,fp) ;
                if (error == 0) return(2) ;
                error=fseek(fp,len[1],1) ;
                if (error == -1) return(5) ;
                error=v_ReadData((char*)item,sizeof(short),2,fp) ;
                if (error == 0) return(2) ;
            }
            if (item[0] > 0x0008) return(100) ;
            if (item[0] == 0x0008) {
                while (item[1] < 0x0010) {
                    error=v_ReadData((char*)&len[0],sizeof(int),1,fp) ;
                    if (error == 0) return(2) ;
                    error=fseek(fp,len[0],1) ;
                    if(error == -1) return(5) ;
                    error=v_ReadData((char*)item,sizeof(short),2,fp) ;
                    if (error == 0) return(2) ;
                    if (item[0] > 0x0008) return(100) ;
                }
                if (item[1] > 0x0010) return(100) ;
                if (item[1] == 0x0010) {
                    error=v_ReadData((char*)&len[0],sizeof(int),1,fp) ;
                    if (error == 0) return(2) ;
                    strcpy(recognition_code,"") ;
                    error=v_ReadData(recognition_code,sizeof(char),len[0],fp) ;
                    if (error == 0) return(2) ;
                    else {
                        recognition_code[len[0]]='\0' ;
                        return(0) ;
                    }
                }
            }
        }
}

/************************************************************************
 *                                                                      *
 *      Function        : v_initialize_validity_flag                    *
 *      Description     : This function initializes all the valid       *
 *                        flags in the 3dviewnix header.                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  vh - viewnix header.                         *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on July 27, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_initialize_validity_flag ( ViewnixHeader* vh )
{
        memset(vh,0,sizeof(ViewnixHeader));
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_ReadHeader_10                               *
 *      Description     : This function will read the file header       *
 *                        information to the vh according to the data   *
 *                        type stored in file header and the            *
 *                        specification file which contains the group   *
 *                        number, element number, and VR marked by star *
 *                        in the document "DATA FORMAT SPECIFICATION: A *
 *                        MULTIDIMENSIONAL EXTENSION TO THE ACR-NEMA    *
 *                        STANDARDS". This function can read the file   *
 *                        header information which has the previous     *
 *                        3DVIEWNIX file header format. If a certain    *
 *                        header info. can not get from the input file  *
 *                        which has the previous file format, then this *
 *                        function will give the corresponding valid bit*
 *                        field to 0, and the value of the corresponding*
 *                        item to be no meaning at all. Otherwise this  *
 *                        function will set the corresponding valid bit *
 *                        field to 1, and assign the right value to the *
 *                        corresponding item. If the fp or vh is NULL,  *
 *                        this function will print the proper message to*
 *                        the standard error stream, produce the core   *
 *                        dump file, and exit from the current process. *
 *                        If read error occurs, this function will      *
 *                        return 2, if the improper seeks happens, this *
 *                        function will return 5, if the data set type  *
 *                        is invalid, this function will return 102, or *
 *                        if the specification file can not open, this  *
 *                        function will return 103. If there is no error*
 *                        this function will return 0.                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *                         7 - environment variable does not exist.     *
 *                         102 - invalid data set type.                 *
 *                         103 - can not open specification file.       *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         vh - pointer to the struct where the         *
 *                              function puts the information.          *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VGetHeaderLength, VWriteHeader.               *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_ReadHeader_10 ( FILE* fp, ViewnixHeader* vh )
{
        /*char msg[80] ; */
        int type, error, read_next ;
        unsigned short group[2] ;
        int items, error1 ;

        error1=0 ;
        error=v_read_data_set_type(fp,&type) ;
        if (error != 0) return(error) ;

        error=fseek(fp,0,0) ;
        if (error == -1) return(5) ;
        read_next=TRUE ;
        items=0 ;
        switch (type) {
            case IMAGE0 : /* scene of type IMAGE0 */
            case IMAGE1 : /* scene of type IMAGE1 */
                error=v_read_items("1.0",0);
                if (error != 0) return(error) ;
                error=v_read_general_10(fp,&(vh->gen),&read_next,group,&items,
                                      &error1) ;
                if (error != 0) return(error) ;
                error=v_read_scene_10(fp,&(vh->scn),&read_next,group,&items,
                                    &error1) ;   
                if (error != 0) return(error) ;
                break;
            case CURVE0 : /* structure date of type CURVE0 */
            case SURFACE0: /* structure data of type SURFACE0 */
            case SURFACE1: /* structure data of type SURFACE1 */
            case SHELL1: /* structure data of type SHELL1 */
            case SHELL0 : /* structure data of type SHELL */
            case SHELL2: /* structure data of type SHELL2 */
                error=v_read_items("1.0",1);
                if (error != 0) return(error) ;
                error=v_read_general_10(fp,&(vh->gen),&read_next,group,&items,
                                      &error1) ;
                if (error != 0) return(error) ;
                error=v_read_structure_10(fp,&(vh->str),&read_next,group,
                                        &items,&error1) ;
                if (error != 0) return(error) ;
                break;
            case MOVIE0 :  /* display data of type MOVIE0 */
                error=v_read_items("1.0",2) ;
                if (error != 0) return(error) ;
                error=v_read_general_10(fp,&(vh->gen),&read_next,group,&items,
                                      &error1) ;
                if (error != 0) return(error) ;
                error=v_read_display_10(fp,&(vh->dsp),&read_next,group,&items,
                                      &error1) ;
                if (error != 0) return(error) ;
                break;
            default :
                return(102);
                break;
        }
        return(error1);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_read_data_set_type                          *
 *      Description     : This function read the input file header to   *
 *                        get the value of the data set type.           *
 *                        If the read error occurs, this function will  *
 *                        return 2, if the improper seek happens, this  *
 *                        function will return 5, or if the input file  *
 *                        has the incorrect file header format, this    *
 *                        function will return 100.                     *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *                         100 - incorrect 3DVIEWNIX file header format.*
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         type - Returns the data set type stored in   *
 *                                the input file header.                *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                                                                      *
 ************************************************************************/
static int v_read_data_set_type ( FILE* fp, int* type )
{
        unsigned short item[2] ; /* item[0] - group #, item[1] - elemt # */
        int len[2], error ;
        short type1 ;

        while (TRUE) {
            error=v_ReadData((char*)item,sizeof(short),2,fp) ;
            if (error == 0) return(2) ;
            while (item[0] < 0x0008) {
                error=v_ReadData((char*)len,sizeof(int),2,fp) ;
                if (error == 0) return(2) ;
                error=fseek(fp,len[1],1) ;
                if (error == -1) return(5) ;
                error=v_ReadData((char*)item,sizeof(short),2,fp) ;
                if (error == 0) return(2) ;
            }
            if (item[0] > 0x0008) return(100) ;
            if (item[0] == 0x0008) {
                while (item[1] < 0x0040) {
                    error=v_ReadData((char*)&len[0],sizeof(int),1,fp) ;
                    if (error == 0) return(2) ;
                    error=fseek(fp,len[0],1) ;
                    if(error == -1) return(5) ;
                    error=v_ReadData((char*)item,sizeof(short),2,fp) ;
                    if (error == 0) return(2) ;
                    if (item[0] > 0x0008) return(100) ;
                }
                if (item[1] > 0x0040) return(100) ;
                if (item[1] == 0x0040) {
                    error=v_ReadData((char*)&len[0],sizeof(int),1,fp) ;
                    if (error == 0) return(2) ;
                    error=v_ReadData((char*)&type1,sizeof(short),1,fp) ;
                    if (error == 0) return(2) ;
                    *type=type1 ;
                    return(0) ;
                }
            }
        }
}

/************************************************************************
 *                                                                      *
 *      Function        : v_read_items                                  *
 *      Description     : This function will open one of specification  *
 *                        files according to the type value and read the*
 *                        group numbers and element numbers to the      *
 *                        acr_items. If the specification file can not  *
 *                        open, this function will return 103, else     *
 *                        return 0.                                     *
 *                        function.                                     *
 *      Return Value    :  0 - work successfully.                       *
 *                         103 - can not open specification file.       *
 *      Parameters      :  version - version of 3dvewiewnix.            *
 *                         type - the specification file type. It can be*
 *                                      0 - SCENE.SPC,                  *
 *                                      1 - STRUCTURE.SPC,              *
 *                                      2 - DISPLAY.SPC.                *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_read_items ( char* version, int type )
{
        FILE *fp ;
        static char *spc_files[MAX_DATA_TYPES]={
                                "SCENE_V","STRUCTURE_V","DISPLAY_V"} ;
        int   i, group, elem;
        char  filename[256],  type1[80], *env, data_type[80] ;

        env=getenv("VIEWNIX_ENV");
        if (env == NULL) return(7);
//        sprintf(filename,"%s/%s/%s%s.SPC",env,FILES_PATH,spc_files[type],
//                version) ;

#ifdef WIN32x
        sprintf( filename, "%s%s%s.SPC", env, spc_files[type], version );
#else
        sprintf( filename, "%s/%s/%s%s.SPC", env, FILES_PATH, spc_files[type], version );
#endif
        fp=fopen(filename,"rb") ;
        if (fp == NULL) return(103) ;
        i=0 ;
        while ((fscanf(fp,"%x %x %s %s",&group,&elem,data_type,
                       type1)) != EOF) {
            acr_items[i].group=group ;
            acr_items[i].elem=elem ;
            strcpy(acr_items[i].type,data_type) ;
            i++ ;
        }
        fclose(fp) ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_read_general_10                             *
 *      Description     : This function reads the general information   *
 *                        from input file header.                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         gen - pointer to general information data in *
 *                               the file header.                       *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         items - the number of items in the group.    *
 *                         error1 - the error code that is returned.    * 
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_read_general_10 ( FILE* fp, GeneralInfo* gen, int* read_next,
    unsigned short* group, int* items, int* error1 )
{
        int error, i ;
        unsigned valid ;

        i= *items ;
        error=v_file_to_string(fp,gen->recognition_code,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->recognition_code_valid=valid ;
        error=v_file_to_string(fp,gen->study_date,15,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->study_date_valid=valid ;
        error=v_file_to_string(fp,gen->study_time,15,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->study_time_valid=valid ;
        error=v_file_to_short(fp,&(gen->data_type),&acr_items[i++],
                            read_next,group,&valid,error1);
        if (error != 0) return(error) ;
        gen->data_type_valid=valid ;
        error=v_file_to_string(fp,gen->modality,10,&acr_items[i++],
                             read_next,group,&valid,error1);
        if (error != 0) return(error) ;
        gen->modality_valid=valid ;
        error=v_file_to_string(fp,gen->institution,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->institution_valid=valid ;
        error=v_file_to_string(fp,gen->physician,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->physician_valid=valid ;
        error=v_file_to_string(fp,gen->department,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->department_valid=valid ;
        error=v_file_to_string(fp,gen->radiologist,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->radiologist_valid=valid ;
        error=v_file_to_string(fp,gen->model,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->model_valid=valid ;
        error=v_file_to_string(fp,gen->filename,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->filename_valid=valid ;
        error=v_file_to_string(fp,gen->filename1,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->filename1_valid=valid ;
        error=v_file_to_strptr(fp,&(gen->description),&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->description_valid=valid ;
        error=v_file_to_strptr(fp,&(gen->comment),&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->comment_valid=valid ;
        error=v_file_to_string(fp,gen->patient_name,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->patient_name_valid=valid ;
        error=v_file_to_string(fp,gen->patient_id,80,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->patient_id_valid=valid ;
        error=v_file_to_float(fp,&(gen->slice_thickness),&acr_items[i++],
                        read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->slice_thickness_valid=valid ;
        error=v_file_to_floats1(fp,gen->kvp,2,&acr_items[i++],read_next,group,
                             &valid,error1) ;
        if (error != 0) return(error) ;
        gen->kvp_valid=valid ;
        error=v_file_to_float(fp,&(gen->repetition_time),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->repetition_time_valid=valid ;
        error=v_file_to_float(fp,&(gen->echo_time),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->echo_time_valid=valid ;
        error=v_file_to_strptr(fp,&(gen->imaged_nucleus),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->imaged_nucleus_valid=valid ;
        error=v_file_to_float(fp,&(gen->gantry_tilt),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->gantry_tilt_valid=valid ;
        error=v_file_to_string(fp,gen->study,10,&acr_items[i++],read_next,
                             group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->study_valid=valid ;
        error=v_file_to_string(fp,gen->series,10,&acr_items[i++],read_next,
                             group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->series_valid=valid ;
        error=v_file_to_shorts1(fp,gen->gray_descriptor,3,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->gray_descriptor_valid=valid ;
        error=v_file_to_shorts1(fp,gen->red_descriptor,3,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->red_descriptor_valid=valid ;
        error=v_file_to_shorts1(fp,gen->green_descriptor,3,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->green_descriptor_valid=valid ;
        error=v_file_to_shorts1(fp,gen->blue_descriptor,3,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->blue_descriptor_valid=valid ;
        error=v_file_to_shorts(fp, (short**)&(gen->gray_lookup_data),
                        gen->gray_descriptor[0],&acr_items[i++],read_next,
                        group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->gray_lookup_data_valid=valid ;
        error=v_file_to_shorts(fp, (short**)&(gen->red_lookup_data),
                        gen->red_descriptor[0],&acr_items[i++],read_next,
                        group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->red_lookup_data_valid=valid ;
        error=v_file_to_shorts(fp, (short**)&(gen->green_lookup_data),
                        gen->green_descriptor[0],&acr_items[i++],read_next,
                        group,&valid,error1) ;
        if (error != 0) return(error) ;
        gen->green_lookup_data_valid=valid ;
        error=v_file_to_shorts(fp, (short**)&(gen->blue_lookup_data),
                        gen->blue_descriptor[0],&acr_items[i++],read_next,
                        group,&valid,error1);
        if (error != 0) return(error);
        gen->blue_lookup_data_valid=valid;
        *items=i;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_read_scene_10                               *
 *      Description     : This function reads the scene information     *
 *                        from input file header.                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         scn - a pointer to the scene information     *
 *                               in the viewnix header.                 *
 *                         read_next - the next item to be read.        *
 *                         group - the group number of the item.        *
 *                         items - the number of items in this group.   *
 *                         error1 - the error code returned by the      *
 *                                  function.                           *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                        Modified: 12/6/00 check for correct count of
 *                           loc_of_subscenes by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_read_scene_10 ( FILE* fp, SceneInfo* scn, int* read_next,
    unsigned short* group, int* items, int* error1 )
{
        int error, i, j, k;
        char *label ;
        unsigned valid ;

        i= (*items) ;
        error=v_file_to_short(fp,&(scn->dimension),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->dimension_valid=valid ;
        error=v_file_to_floats(fp,&(scn->domain),
                        (scn->dimension+1)*(scn->dimension),&acr_items[i++],
                        read_next,group,&valid,error1);
        if (error != 0) return(error) ;
        scn->domain_valid=valid ;
        if (scn->dimension_valid == 1 && scn->domain_valid == 0) {
            if (scn->domain != NULL) free(scn->domain) ;
            scn->domain=(float *)malloc(scn->dimension*(scn->dimension+1)*
                                        sizeof(float)) ;
            if (scn->domain == NULL) return(1) ;
            for (j=0; j<(scn->dimension+1)*(scn->dimension); j++)
                scn->domain[j]=0 ;
            for (j=scn->dimension; j<scn->dimension*(scn->dimension+1); 
                 j=j+scn->dimension+1) scn->domain[j]=1 ;
        }
        if (scn->dimension_valid == 0 && scn->domain_valid == 1) {
            if (scn->domain != NULL) free(scn->domain) ;
            scn->domain=NULL ;
            scn->domain_valid=0 ;
        }
        error=v_file_to_strptr(fp,&label,&acr_items[i++],read_next,group,
                             &valid,error1);
        if (error != 0) return(error) ;
        scn->axis_label_valid=valid ;
        if (label == NULL) scn->axis_label=NULL ;
        else {
            scn->axis_label=(Char30 *)malloc(scn->dimension*sizeof(Char30)) ;
            if (scn->axis_label == NULL) return(1) ;
            v_decode_strptr(label,scn->axis_label,scn->dimension);
            free(label) ;
        }
        error=v_file_to_shorts(fp,&(scn->measurement_unit),scn->dimension,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->measurement_unit_valid=valid ;
        if (scn->dimension_valid == 1 && scn->measurement_unit_valid == 0) {
            if (scn->measurement_unit != NULL) free(scn->measurement_unit) ;
            scn->measurement_unit=(short *)malloc(scn->dimension *      
                                                  sizeof(short));
            if (scn->measurement_unit == NULL) return(1) ;
            for (j=0; j<scn->dimension; j++) scn->measurement_unit[j]=3 ;
        }
        if (scn->dimension_valid == 0 && scn->measurement_unit_valid == 1) {
            if (scn->measurement_unit != NULL) free(scn->measurement_unit) ;
            scn->measurement_unit=NULL ;
            scn->measurement_unit_valid=0 ;
        }
        error=v_file_to_short(fp,&(scn->num_of_density_values),
                &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->num_of_density_values_valid=valid ;

        error=v_file_to_shorts(fp,&(scn->density_measurement_unit),
                scn->num_of_density_values,&acr_items[i++],read_next,
                group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->density_measurement_unit_valid=valid ;

        error=v_file_to_floats(fp,&(scn->smallest_density_value),
                        scn->num_of_density_values,&acr_items[i++],read_next,
                        group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->smallest_density_value_valid=valid ;
        if (scn->num_of_density_values_valid == 1 &&
            scn->smallest_density_value_valid == 0) {
            if (scn->smallest_density_value != NULL) 
                free(scn->smallest_density_value) ;
            scn->smallest_density_value=(float *)malloc(sizeof(float)*
                                                scn->num_of_density_values) ;
            if (scn->smallest_density_value == NULL) return(1) ;
            for (j=0; j<scn->num_of_density_values; j++)
                    scn->smallest_density_value[j]=0 ;
        }
        if (scn->num_of_density_values_valid == 0 &&
            scn->smallest_density_value_valid == 1) {
            if (scn->smallest_density_value != NULL) 
                free(scn->smallest_density_value) ;
            scn->smallest_density_value=NULL ;
            scn->smallest_density_value_valid=0 ;
        }
        error=v_file_to_floats(fp,&(scn->largest_density_value),
                        scn->num_of_density_values,&acr_items[i++],read_next,
                        group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->largest_density_value_valid=valid ;
        if (scn->num_of_density_values_valid == 1 &&
            scn->largest_density_value_valid == 0) {
            if (scn->largest_density_value != NULL) 
                free(scn->largest_density_value) ;
            scn->largest_density_value=(float *)malloc(sizeof(float)*
                                                scn->num_of_density_values) ;
            if (scn->largest_density_value == NULL) return(1) ;
            for (j=0; j<scn->num_of_density_values; j++)
                    scn->largest_density_value[j]=0x7fff ;
        }
        if (scn->num_of_density_values_valid == 0 &&
            scn->largest_density_value_valid == 1) {
            if (scn->largest_density_value != NULL) 
                free(scn->largest_density_value) ;
            scn->largest_density_value=NULL ;
            scn->largest_density_value_valid=0 ;
        }
        error=v_file_to_short(fp,&(scn->num_of_integers),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->num_of_integers_valid=valid ;
        error=v_file_to_shorts(fp,&(scn->signed_bits),scn->num_of_integers,
                        &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->signed_bits_valid=valid ;
        if (scn->num_of_integers_valid == 1 && scn->signed_bits_valid == 0) {
            if (scn->signed_bits != NULL) free(scn->signed_bits) ;
            scn->signed_bits=(short *)malloc(sizeof(short)*
                                             scn->num_of_integers) ;
            if (scn->signed_bits == NULL) return(1) ;
            for (j=0; j<scn->num_of_integers; j++) scn->signed_bits[j]=0 ;
        }
        if (scn->num_of_integers_valid == 0 && scn->signed_bits_valid == 1) {
            if (scn->signed_bits != NULL) free(scn->signed_bits) ;
            scn->signed_bits=NULL ;
            scn->signed_bits_valid=0 ;
        }
        error=v_file_to_short(fp,&(scn->num_of_bits),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->num_of_bits_valid=valid ;
        error=v_file_to_shorts(fp,&(scn->bit_fields),
                           2*scn->num_of_density_values,&acr_items[i++],
                           read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->bit_fields_valid=valid ;
        if (scn->num_of_density_values_valid == 0 &&
            scn->bit_fields_valid == 1) {
            if (scn->bit_fields != NULL) free(scn->bit_fields) ;
            scn->bit_fields=NULL ;
            scn->bit_fields_valid=0 ;
        }
        error=v_file_to_short(fp,&(scn->dimension_in_alignment),
                &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->dimension_in_alignment_valid=valid ;
        if (valid == 0) scn->dimension_in_alignment=2 ;
        error=v_file_to_short(fp,&(scn->bytes_in_alignment),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->bytes_in_alignment_valid=valid ;
        if (valid == 0) scn->bytes_in_alignment=1 ;
        error=v_file_to_shorts1(fp,scn->xysize,2,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->xysize_valid=valid ;
        j = scn->dimension==4? -9999: 1;
		error=v_file_to_shorts(fp,&(scn->num_of_subscenes), j,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->num_of_subscenes_valid=valid ;
        error=v_file_to_floats1(fp,scn->xypixsz,2,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->xypixsz_valid=valid ;
        j = scn->num_of_subscenes[0];
		if (scn->dimension == 4)
			for (k=1; k<=scn->num_of_subscenes[0]; k++)
				j += scn->num_of_subscenes[k];
		error=v_file_to_floats(fp,&(scn->loc_of_subscenes), j,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->loc_of_subscenes_valid=valid ;
        error=v_file_to_strptr(fp,&(scn->description),&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        scn->description_valid=valid ;
        *items=i ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_read_structure_10                           *
 *      Description     : This function reads the structure information *
 *                        from the input file header.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         str - a pointer to the structure information *
 *                               in the viewnix header.                 *
 *                         read_next - the next item to be read.        *
 *                         group - the group number the item belongs to.*
 *                         items - the number of items in the group.    *
 *                         error1 - the error code returned.            *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                        Modified: 12/6/00 check for correct count of
 *                           loc_of_samples by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_read_structure_10 ( FILE* fp, StructureInfo* str, int* read_next,
    unsigned short* group, int* items, int* error1 )
{
        int error, i, j , k ;
        char *label,*scene_file ;
        unsigned valid ;
        int val = 1;

        i= *items ;


        /**002B 8000****/
        error=v_file_to_short(fp,&(str->dimension),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->dimension_valid=valid ;

        /**002B 8005****/
        error=v_file_to_short(fp,&(str->num_of_structures),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_structures_valid=valid ;
        if (str->num_of_structures_valid == 0) str->num_of_structures=1;


        /**002B 8010****/
        error=v_file_to_floats(fp,&(str->domain),
                        str->num_of_structures*(str->dimension+1)*(str->dimension),
                        &acr_items[i++],read_next,group,&valid,error1);
        if (error != 0) return(error) ;
        str->domain_valid=valid ;
        if (str->dimension_valid == 1 && str->domain_valid == 0) {
          if (str->domain != NULL) free(str->domain) ;
          str->domain=(float *)malloc(str->num_of_structures*str->dimension*(str->dimension+1)*
                                      sizeof(float)) ;
          if (str->domain == NULL) return(1) ;
          for(k=0;k<str->num_of_structures;k++) {
            for (j=0; j<(str->dimension+1)*(str->dimension); j++)
              str->domain[k*str->dimension*(str->dimension+1)+j]=0 ;
            for (j=str->dimension; j<str->dimension*(str->dimension+1); 
                 j=j+str->dimension+1) 
              str->domain[k*str->dimension*(str->dimension+1)+j]=1 ;
          }
        }
        if (str->dimension_valid == 0 && str->domain_valid == 1) {
            if (str->domain != NULL) free(str->domain) ;
            str->domain=NULL ;
            str->domain_valid=0 ;
        }

        /**002B 8015****/
        error=v_file_to_strptr(fp,&label,&acr_items[i++],read_next,group,
                             &valid,error1);
        if (error != 0) return(error) ;
        str->axis_label_valid=valid ;
        if (label == NULL) str->axis_label=NULL ;
        else {
            str->axis_label=(Char30 *)malloc(str->dimension*sizeof(Char30)) ;
            if (str->axis_label == NULL) return(1) ;
            v_decode_strptr(label,str->axis_label,str->dimension) ;
            free(label) ;
        }

        /**002B 8020****/
        error=v_file_to_shorts(fp,&(str->measurement_unit),str->dimension,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->measurement_unit_valid=valid ;
        if (str->dimension_valid == 1 && str->measurement_unit_valid == 0) {
            if (str->measurement_unit != NULL) free(str->measurement_unit) ;
            str->measurement_unit=(short *)malloc(str->dimension*
                                                  sizeof(short));
            if (str->measurement_unit == NULL) return(1) ;
            for (j=0; j<str->dimension; j++) str->measurement_unit[j]=3 ;
        }
        if (str->dimension_valid == 0 && str->measurement_unit_valid == 1) {
            if (str->measurement_unit != NULL) free(str->measurement_unit) ;
            str->measurement_unit=NULL ;
            str->measurement_unit_valid=0 ;
        }

        /**002B 8025****/
        error=v_file_to_strptr(fp,&scene_file,&acr_items[i++],read_next,group,
                             &valid,error1);
        if (error != 0) return(error) ;
        str->scene_file_valid=valid ;
        if (scene_file == NULL) str->scene_file=NULL ;
        else {
          str->scene_file=(Char30 *)malloc(str->num_of_structures*sizeof(Char30)) ;
          if (str->scene_file == NULL) return(1) ;
          v_decode_strptr(scene_file,str->scene_file,str->num_of_structures) ;
          free(scene_file) ;
        }

        

        /**002B 8030****/
        error=v_file_to_ints(fp, (int**)&(str->num_of_TSE),
                             str->num_of_structures,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_TSE_valid=valid ;
        if (str->num_of_structures_valid &&
            !str->num_of_TSE_valid) {
            if (str->num_of_TSE != NULL)
                free(str->num_of_TSE) ;
            str->num_of_TSE=(unsigned int *)malloc(str->num_of_structures*
                                            sizeof(unsigned int)) ;
            if (str->num_of_TSE == NULL) return(1) ;
            for (j=0; j<str->num_of_structures; j++)
                str->num_of_TSE[j]=1 ;
        }
        if (str->num_of_structures_valid == 0 &&
            str->num_of_TSE_valid == 1) {
            if (str->num_of_TSE != NULL)
                free(str->num_of_TSE) ;
            str->num_of_TSE=NULL ;
            str->num_of_TSE_valid=0 ;
        }

        /**002B 8040****/
        error=v_file_to_ints(fp, (int**)&(str->num_of_NTSE),
                             str->num_of_structures,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_NTSE_valid=valid ;
        if (str->num_of_structures_valid &&
            !str->num_of_NTSE_valid) {
            if (str->num_of_NTSE != NULL)
                free(str->num_of_NTSE) ;
            str->num_of_NTSE=(unsigned int *)malloc(str->num_of_structures*
                                            sizeof(unsigned int)) ;
            if (str->num_of_NTSE == NULL) return(1) ;
            for (j=0; j<str->num_of_structures; j++)
                str->num_of_NTSE[j]=1 ;
        }
        if (str->num_of_structures_valid == 0 &&
            str->num_of_NTSE_valid == 1) {
            if (str->num_of_NTSE != NULL)
                free(str->num_of_NTSE) ;
            str->num_of_NTSE=NULL ;
            str->num_of_NTSE_valid=0 ;
        }

        /**002B 8050****/
        error=v_file_to_short(fp,&(str->num_of_components_in_TSE),
                            &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_components_in_TSE_valid=valid ;

        /**002B 8060****/
        error=v_file_to_short(fp,&(str->num_of_components_in_NTSE),
                            &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_components_in_NTSE_valid=valid ;



        /**002B 8065****/
        error=v_file_to_shorts(fp,&(str->TSE_measurement_unit),str->num_of_components_in_TSE,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->TSE_measurement_unit_valid=valid ;

        /**002B 8065****/
        error=v_file_to_shorts(fp,&(str->NTSE_measurement_unit),str->num_of_components_in_NTSE,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->NTSE_measurement_unit_valid=valid ;



        /**002B 8070****/
        error=v_file_to_floats(fp,&(str->smallest_value),
                             str->num_of_components_in_TSE*
                             str->num_of_structures,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->smallest_value_valid=valid ;
        if ((str->num_of_components_in_TSE_valid == 1 ||
                str->num_of_structures_valid == 1) &&
            str->smallest_value_valid == 0) {
            if (str->smallest_value != NULL) free(str->smallest_value) ;
            str->smallest_value=(float *)malloc(sizeof(float)*
                                        str->num_of_components_in_TSE) ;
            if (str->smallest_value == NULL) return(1) ;
            for (j=0; j<str->num_of_components_in_TSE; j++)
                str->smallest_value[j]=0 ;
        }
        if ((str->num_of_components_in_TSE_valid == 0 ||
                str->num_of_structures_valid == 0) &&
            str->smallest_value_valid == 1) {
            if (str->smallest_value != NULL) free(str->smallest_value) ;
            str->smallest_value=NULL ;
            str->smallest_value_valid=0 ;
        }

        /**002B 8080****/
        error=v_file_to_floats(fp,&(str->largest_value),
                             str->num_of_components_in_TSE*
                             str->num_of_structures,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->largest_value_valid=valid ;
        if ((str->num_of_components_in_TSE_valid == 1 ||
                str->num_of_structures_valid == 1) &&
            str->largest_value_valid == 0) {
            if (str->largest_value != NULL) free(str->largest_value) ;
            str->largest_value=(float *)malloc(sizeof(float)*
                                        str->num_of_components_in_TSE) ;
            if (str->largest_value == NULL) return(1) ;
            for (j=0; j<str->num_of_components_in_TSE; j++)
                str->largest_value[j]=0x7fff ;
        }
        if ((str->num_of_components_in_TSE_valid == 0 ||
                str->num_of_structures_valid == 0) &&
            str->largest_value_valid == 1) {
            if (str->largest_value != NULL) free(str->largest_value) ;
            str->largest_value=NULL ;
            str->largest_value_valid=0 ;
        }

        /**002B 8090****/
        error=v_file_to_short(fp,&(str->num_of_integers_in_TSE),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_integers_in_TSE_valid=valid ;

        /**002B 80A0****/
        error=v_file_to_shorts(fp,&(str->signed_bits_in_TSE),
                             str->num_of_integers_in_TSE,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->signed_bits_in_TSE_valid=valid ;
        if (str->num_of_integers_in_TSE_valid == 1 && 
            str->signed_bits_in_TSE_valid == 0) {
            if (str->signed_bits_in_TSE != NULL) 
                free(str->signed_bits_in_TSE) ;
            str->signed_bits_in_TSE=(short *)malloc(sizeof(short)*
                                             str->num_of_integers_in_TSE) ;
            if (str->signed_bits_in_TSE == NULL) return(1) ;
            for (j=0; j<str->num_of_integers_in_TSE; j++) 
                str->signed_bits_in_TSE[j]=0 ;
        }
        if (str->num_of_integers_in_TSE_valid == 0 && 
            str->signed_bits_in_TSE_valid == 1) {
            if (str->signed_bits_in_TSE != NULL) 
                free(str->signed_bits_in_TSE) ;
            str->signed_bits_in_TSE=NULL ;
            str->signed_bits_in_TSE_valid=0 ;
        }

        /**002B 80B0****/
        error=v_file_to_short(fp,&(str->num_of_bits_in_TSE),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_bits_in_TSE_valid=valid ;

        /**002B 80C0****/
        error=v_file_to_shorts(fp,&(str->bit_fields_in_TSE),
                             2*str->num_of_components_in_TSE,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->bit_fields_in_TSE_valid=valid ;
        if (str->num_of_components_in_TSE_valid == 0 &&
            str->bit_fields_in_TSE_valid == 1) {
            if (str->bit_fields_in_TSE != NULL) free(str->bit_fields_in_TSE) ;
            str->bit_fields_in_TSE=NULL ;
            str->bit_fields_in_TSE_valid=0 ;
        }

        /**002B 80D0****/
        error=v_file_to_short(fp,&(str->num_of_integers_in_NTSE),
                            &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_integers_in_NTSE_valid=valid ;

        /**002B 80E0****/
        error=v_file_to_shorts(fp,&(str->signed_bits_in_NTSE),
                             str->num_of_integers_in_NTSE,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->signed_bits_in_NTSE_valid=valid ;
        if (str->num_of_integers_in_NTSE_valid == 1 && 
            str->signed_bits_in_NTSE_valid == 0) {
            if (str->signed_bits_in_NTSE != NULL) 
                free(str->signed_bits_in_NTSE) ;
            str->signed_bits_in_NTSE=(short *)malloc(sizeof(short)*
                                             str->num_of_integers_in_NTSE) ;
            if (str->signed_bits_in_NTSE == NULL) return(1) ;
            for (j=0; j<str->num_of_integers_in_NTSE; j++) 
                str->signed_bits_in_NTSE[j]=0 ;
        }
        if (str->num_of_integers_in_NTSE_valid == 0 && 
            str->signed_bits_in_NTSE_valid == 1) {
            if (str->signed_bits_in_NTSE != NULL) 
                free(str->signed_bits_in_NTSE) ;
            str->signed_bits_in_NTSE=NULL ;
            str->signed_bits_in_NTSE_valid=0 ;
        }

        /**002B 80F0****/
        error=v_file_to_short(fp,&(str->num_of_bits_in_NTSE),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_bits_in_NTSE_valid=valid ;

        /**002B 8100****/
        error=v_file_to_shorts(fp,&(str->bit_fields_in_NTSE),
                           2*str->num_of_components_in_NTSE,&acr_items[i++],
                           read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->bit_fields_in_NTSE_valid=valid ;
        if (str->num_of_components_in_NTSE_valid == 0 &&
            str->bit_fields_in_NTSE_valid == 1) {
            if (str->bit_fields_in_NTSE != NULL) 
                free(str->bit_fields_in_NTSE) ;
            str->bit_fields_in_NTSE=NULL ;
            str->bit_fields_in_NTSE_valid=0 ;
        }

        /**002B 8110****/
		j = str->dimension==4? -9999: 1;
        error=v_file_to_shorts(fp,&(str->num_of_samples), j, &acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_samples_valid=valid ;

        /**002B 8120****/
        error=v_file_to_floats1(fp,str->xysize,2,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->xysize_valid=valid ;

        /**002B 8130****/
        j = str->num_of_samples[0];
		if (str->dimension == 4)
			for (k=1; k<=str->num_of_samples[0]; k++)
				j += str->num_of_samples[k];
        error=v_file_to_floats(fp,&(str->loc_of_samples), j, &acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->loc_of_samples_valid=valid ;

        /**002B 8131****/
        error=v_file_to_short(fp,&(str->num_of_elements),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->num_of_elements_valid=valid ;
        if (str->num_of_elements_valid == 0) str->num_of_elements=1 ;

        /**002B 8132****/
        error=v_file_to_shorts(fp,&(str->description_of_element),
                             str->num_of_elements,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->description_of_element_valid=valid ;
        if ( !str->description_of_element_valid) {
            if (str->description_of_element != NULL) 
                free(str->description_of_element) ;
            str->description_of_element=(short *)malloc(str->num_of_elements*
                                                        sizeof(short)) ;
            if (str->description_of_element == NULL) return(1) ;
            for (j=0; j<str->num_of_elements; j++) 
                str->description_of_element[j]=1 ;
        }
/*
        if (str->num_of_elements_valid == 0 &&
            str->description_of_element_valid == 1) {
            if (str->description_of_element != NULL) 
                free(str->description_of_element) ;
            for (j=0; j<str->num_of_elements; j++)
                str->description_of_element[j]=1 ;
            str->description_of_element_valid=0 ;
        }
*/
        /**002B 8134****/
        error=v_file_to_floats(fp,&(str->parameter_vectors),
                             str->num_of_elements*str->num_of_structures,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->parameter_vectors_valid=valid ;
        if (str->num_of_structures_valid && 
            !str->parameter_vectors_valid) {
            if (str->parameter_vectors != NULL) 
                free(str->parameter_vectors) ;
            str->parameter_vectors=(float *)malloc(str->num_of_elements*
                                str->num_of_structures*sizeof(float)) ;
            if (str->parameter_vectors == NULL) return(1) ;
            for(j=0;j<str->num_of_structures*str->num_of_elements;j++) {
                   if(j==str->num_of_elements) val++;
                   str->parameter_vectors[j] = (float)val;
            }
        }
        if ( str->num_of_structures_valid == 0 &&
            str->parameter_vectors_valid == 1) {
            if (str->parameter_vectors != NULL) 
                free(str->parameter_vectors) ;
            str->parameter_vectors=(float *)malloc(str->num_of_elements*
                                str->num_of_structures*sizeof(float)) ;
            if (str->parameter_vectors == NULL) return(1) ;
            for(j=0;j<str->num_of_structures*str->num_of_elements;j++) {
                   if(j==str->num_of_elements) val++;
                   str->parameter_vectors[j] = (float)val;
            }
            str->parameter_vectors_valid=0 ;
        }

        /**002B 8140****/
        error=v_file_to_floats(fp,&(str->min_max_coordinates),
                             2*str->dimension*str->num_of_structures,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->min_max_coordinates_valid=valid ;
        if ((str->dimension_valid == 0 ||
            str->num_of_structures_valid == 0) && 
            str->min_max_coordinates_valid == 1) {
            if (str->min_max_coordinates != NULL)
                 free(str->min_max_coordinates) ;
            str->min_max_coordinates=NULL ;
            str->min_max_coordinates_valid=0 ;
        }

        /**002B 8150****/
        error=v_file_to_floats(fp,&(str->volume),
                             str->num_of_structures,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->volume_valid=valid ;
        if (str->num_of_structures_valid &&
            !str->volume_valid) {
            if (str->volume != NULL)
                free(str->volume) ;
            str->volume=(float *)malloc(str->num_of_structures*
                                                        sizeof(float));
            if (str->volume == NULL) return(1) ;
            for (j=0; j<str->num_of_structures; j++)
                str->volume[j]=0 ;
        }
        if (str->num_of_structures_valid == 0 &&
            str->volume_valid == 1) {
            if (str->volume != NULL)
                free(str->volume) ;
            str->volume=NULL ;
            str->volume_valid=0 ;
        }

        /**002B 8160****/
        error=v_file_to_floats(fp,&(str->surface_area),
                             str->num_of_structures,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->surface_area_valid=valid ;
        if (str->num_of_structures_valid &&
            !str->surface_area_valid) {
            if (str->surface_area != NULL)
                free(str->surface_area) ;
            str->surface_area=(float *)malloc(str->num_of_structures*
                                                        sizeof(float)) ;
            if (str->surface_area == NULL) return(1) ;
            for (j=0; j<str->num_of_structures; j++)
                str->surface_area[j]=0;
        }
        if (str->num_of_structures_valid == 0 &&
            str->surface_area_valid == 1) {
            if (str->surface_area != NULL)
                free(str->surface_area) ;
            str->surface_area=NULL ;
            str->surface_area_valid=0 ;
        }

        /**002B 8170****/
        error=v_file_to_floats(fp,&(str->rate_of_change_volume),
                             str->num_of_structures,&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        str->rate_of_change_volume_valid=valid ;
        if (str->num_of_structures_valid &&
            !str->rate_of_change_volume_valid) {
            if (str->rate_of_change_volume != NULL)
                free(str->rate_of_change_volume) ;
            str->rate_of_change_volume=(float *)malloc(str->num_of_structures*
                                                        sizeof(float)) ;
            if (str->rate_of_change_volume == NULL) return(1) ;
            for (j=0; j<str->num_of_structures; j++)
                str->rate_of_change_volume[j]=0;
        }
        if (str->num_of_structures_valid == 0 &&
            str->rate_of_change_volume_valid == 1) {
            if (str->rate_of_change_volume != NULL)
                free(str->rate_of_change_volume) ;
            str->rate_of_change_volume=NULL ;
            str->rate_of_change_volume_valid=0 ;
        }

        /**002B 8180****/
        error=v_file_to_strptr(fp,&(str->description),&acr_items[i++],
                             read_next,group,&valid,error1);
        if (error != 0) return(error) ;
        str->description_valid=valid ;

        /**end structure data****/

        *items=i ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_read_display_10                             *
 *      Description     : This function reads the display information   *
 *                        from the input file header.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         dsp - pointer to display information data in * 
 *                               the file header.                       *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         items - the number of items in the group.    *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_read_display_10 ( FILE* fp, DisplayInfo* dsp, int* read_next,
    unsigned short* group, int* items, int* error1 )
{
        int error, i, j ;
        unsigned valid ;

        i= *items ;
        error=v_file_to_short(fp,&(dsp->dimension),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->dimension_valid=valid ;
        error=v_file_to_shorts1(fp,dsp->measurement_unit,2,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->measurement_unit_valid=valid ;

/***********New changes as requested by Dewey on 10/7/92*********/
/*Changed because meausement_unit is now a fixed array instead*/
/*of a pointer. Also dependencies on dimension is eliminated now.*/
/*These changes were made by Krishna Iyer on 10/7/92********/
        if (!valid)
            for (j=0; j<2; j++) dsp->measurement_unit[j]=3 ;

/***************end changes*******************/

        error=v_file_to_short(fp,&(dsp->num_of_elems),&acr_items[i++],
                            read_next,group,&valid,error1);
        if (error != 0) return(error) ;
        dsp->num_of_elems_valid=valid ;
        error=v_file_to_floats(fp,&(dsp->smallest_value),
                        dsp->num_of_elems,&acr_items[i++],read_next,
                        group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->smallest_value_valid=valid ;
        if (dsp->num_of_elems_valid == 1 && dsp->smallest_value_valid == 0) {
            if (dsp->smallest_value != NULL) free(dsp->smallest_value) ;
            dsp->smallest_value=(float *)malloc(sizeof(float)*
                                                dsp->num_of_elems) ;
            if (dsp->smallest_value == NULL) return(1) ;
            for (j=0; j<dsp->num_of_elems; j++) dsp->smallest_value[j]=0 ;
        }
        if (dsp->num_of_elems_valid == 0 && dsp->smallest_value_valid == 1) {
            if (dsp->smallest_value != NULL) free(dsp->smallest_value) ;
            dsp->smallest_value=NULL ;
            dsp->smallest_value_valid=0 ;
        }
        error=v_file_to_floats(fp,&(dsp->largest_value),dsp->num_of_elems,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->largest_value_valid=valid ;
        if (dsp->num_of_elems_valid == 1 && dsp->largest_value_valid == 0) {
            if (dsp->largest_value != NULL) free(dsp->largest_value) ;
            dsp->largest_value=(float *)malloc(sizeof(float)*
                                               dsp->num_of_elems) ;
            if (dsp->largest_value == NULL) return(1) ;
            for (j=0; j<dsp->num_of_elems; j++) dsp->largest_value[j]=0x7fff ;
        }
        error=v_file_to_short(fp,&(dsp->num_of_integers),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->num_of_integers_valid=valid ;
        error=v_file_to_shorts(fp,&(dsp->signed_bits),dsp->num_of_integers,
                        &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->signed_bits_valid=valid ;
        if (dsp->num_of_integers_valid == 1 && dsp->signed_bits_valid == 0) {
            if (dsp->signed_bits != NULL) free(dsp->signed_bits) ;
            dsp->signed_bits=(short *)malloc(sizeof(short)*
                                             dsp->num_of_integers) ;
            if (dsp->signed_bits == NULL) return(1) ;
            for (j=0; j<dsp->num_of_integers; j++) dsp->signed_bits[j]=0 ;
        }
        if (dsp->num_of_integers_valid == 0 && dsp->signed_bits_valid == 1) {
            if (dsp->signed_bits != NULL) free(dsp->signed_bits) ;
            dsp->signed_bits=NULL ;
            dsp->signed_bits_valid=0 ;
        }
        error=v_file_to_short(fp,&(dsp->num_of_bits),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->num_of_bits_valid=valid ;
        error=v_file_to_shorts(fp,&(dsp->bit_fields),2*dsp->num_of_elems,
                             &acr_items[i++],read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->bit_fields_valid=valid ;
        if (dsp->num_of_elems_valid == 0 && dsp->bit_fields_valid == 1) {
            if (dsp->bit_fields != NULL) free(dsp->bit_fields) ;
            dsp->bit_fields=NULL ;
            dsp->bit_fields_valid=0 ;
        }
        error=v_file_to_short(fp,&(dsp->dimension_in_alignment),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->dimension_in_alignment_valid=valid ;
        if (valid == 0) dsp->dimension_in_alignment=2 ;
        error=v_file_to_short(fp,&(dsp->bytes_in_alignment),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->bytes_in_alignment_valid=valid ;
        if (valid == 0) dsp->bytes_in_alignment=1 ;
        error=v_file_to_short(fp,&(dsp->num_of_images),&acr_items[i++],
                            read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->num_of_images_valid=valid ;
        error=v_file_to_shorts1(fp,dsp->xysize,2,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->xysize_valid=valid ;
        error=v_file_to_floats1(fp,dsp->xypixsz,2,&acr_items[i++],
                              read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->xypixsz_valid=valid ;
        if (dsp->xypixsz_valid == 0) dsp->xypixsz[0]=dsp->xypixsz[1]=1.0 ;
        error=v_file_to_strptr(fp,&(dsp->specification_pv),&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->specification_pv_valid=valid ;
        if (dsp->dimension_valid == 0) {
            if (dsp->specification_pv != NULL) {
                free(dsp->specification_pv) ;
                dsp->specification_pv=NULL ;
            }
            dsp->specification_pv_valid=0 ;
        }
        error=v_file_to_shorts(fp,&(dsp->pv),dsp->num_of_images,
                             &acr_items[i++],read_next,group,&valid,error1);
        if (error != 0) return(error) ;
        dsp->pv_valid=valid ;
        if (dsp->num_of_images_valid == 0 && dsp->pv_valid == 1) {
            if (dsp->pv != NULL) free(dsp->pv) ;
            dsp->pv=NULL ;
            dsp->pv_valid=0 ;
        }
        error=v_file_to_strptr(fp,&(dsp->description),&acr_items[i++],
                             read_next,group,&valid,error1) ;
        if (error != 0) return(error) ;
        dsp->description_valid=valid ;
        *items=i ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_check_item                                  *
 *      Description     : This function will check whether the current  *
 *                        item in the specification file is the same as *
 *                        the current item read from the input file. If *
 *                        not number in the input file is greater than  *
 *                        or equal to the item in the specification     *
 *                        file. If the items are the same, then this    *
 *                        function will return 0, otherwise will return *
 *                        104. If the read error occurs, this function  *
 *                        will return 2, or if the improper seek        *
 *                        happens, this function will return 5.         *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *                         104 - invalid value in the  file header.     *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         read_next - Specifies if read the next group *
 *                                 number and element number from the   *
 *                                 input file.                          *
 *                         group - Specifies the current group number   *
 *                                 and element number in the input file.*
 *                         item - Specifies the current group number    *
 *                                 and element number in the            *
 *                                 specification file.                  *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_check_item ( FILE* fp, int* read_next, unsigned short* group,
    ItemInfo* item, int* error1 )
{
        unsigned short grp[2] ;
        int i, error, len[2] ;

        if (*read_next) {
            error=v_ReadData((char*)grp,sizeof(short),2,fp) ;
            if (error == 0) return(2) ;
        }
        else for (i=0; i<2; i++) grp[i]=group[i] ;

        while (grp[0] < item->group) {
            error=v_ReadData((char*)len,sizeof(int),2,fp) ;
            if (error == 0) return(2) ;
            error=fseek(fp,len[1],1) ;
            if (error == -1) return(5) ;
            error=v_ReadData((char*)grp,sizeof(short),2,fp) ;
            if (error == 0) return(2) ;
        }
        if (grp[0] > item->group) {
            *read_next=FALSE ;
            for (i=0; i<2; i++) group[i]=grp[i] ;
            v_assign_data_error_code(error1,item) ;
            return(-1) ;
        }
        if (grp[0] == item->group) {
            while (grp[1] < item->elem) {
                error=v_ReadData((char*)&len[0],sizeof(int),1,fp) ;
                if (error == 0) return(2) ;
                error=fseek(fp,len[0],1) ;
                if (error == -1) return(5) ;
                error=v_ReadData((char*)grp,sizeof(short),2,fp) ;
                if (error == 0) return(2) ;
                if (grp[0] > item->group) {
                    *read_next=FALSE ;
                    for (i=0; i<2; i++) group[i]=grp[i] ;
                    v_assign_data_error_code(error1,item) ;
                    return(-1) ;
                }
            }
            if (grp[1] > item->elem) {
                *read_next=FALSE ;
                for (i=0; i<2; i++) group[i]=grp[i] ;
                v_assign_data_error_code(error1,item) ;
                return(-1) ;
            }
            if (grp[1] == item->elem) {
                *read_next=TRUE ;
                return(0) ;
            }
        }
        return 0;  //gjg?
}

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_string                              *
 *      Description     : If the length of this item is zero, then      *
 *                        return empty string to string, else read      *
 *                        string from input file.                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         string - return the desired string in the    *
 *                                  file.                               *
 *                         max - Specifies the number of characters in  *
 *                               the string.                            *
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                list of items.                        *
 *                         group - the group the item belongs to.       *
 *                         valid - the vlaid flag for the item.         *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                        Modified 4/3/97 len==max case corrected
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_file_to_string ( FILE* fp, char* string, int max, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 )
//unsigned *valid ;  //might be short or int
{
        char *data ;
        int len, error ;

        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (len == 0) {
            v_assign_data_error_code(error1,item) ;
            strcpy(string,"") ;
            return(0) ;
        }
        data=(char *)malloc((len+1)*sizeof(char)) ;
        if (data == NULL) return(1) ;
        memset(data,0,len) ;
        error=v_ReadData(data,sizeof(char),len,fp) ;
        if (error == 0) return(2) ;
        *valid=1 ;
        data[len]='\0' ;
        if (len >= max) strncpy(string,data,max-1), string[max-1]=0;
        else strcpy(string,data) ;
        free(data) ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_assign_data_error_code                      *
 *      Description     : This function reads the error code for the    *
 *                        different data types.                         *
 *      Return Value    :  0 - work successfully.                       *
        Parameters      :  error1 - the error code that is returned.    *
 *                         items - the number of items in the group.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on May 22, 1992 by Hsiu-Mei Hung.     *
 *                        Modified: 12/6/00 inconsistent return fixed
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_assign_data_error_code ( int* error1, ItemInfo* item )
{
        int error ;

        error= *error1 ;
        switch (error) {
            case 0 :
                if (strcmp(item->type,"1") == 0) *error1=104 ;
                if (strcmp(item->type,"1D") == 0) *error1=106 ;
                if (strcmp(item->type,"2") == 0) *error1=107 ;
                if (strcmp(item->type,"2D") == 0) *error1=107 ;
                if (strcmp(item->type,"3") == 0) *error1=107 ;
                break ;
            case 104 : 
                return (error);
                break ;
            case 106 :
                if (strcmp(item->type,"1") == 0) *error1=104 ;
                else return (error);
                break ;
            case 107 :
                if (strcmp(item->type,"1") == 0) *error1=104 ;
                else {
                    if (strcmp(item->type,"1D") == 0) *error1=106 ;
                    else return (error);
                }
                break ;
        }
        sprintf(lib_group_error,"%-.4x",item->group) ;
        sprintf(lib_element_error,"%-.4x",item->elem) ;
        return(0);
}       

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_strptr                              *
 *      Description     : This function reads string from input file and*
 *                        return a pointer to the string. If the length *
 *                        of string is zero, this function will return  *
 *                        NULL pointer to the *strptr. Otherwise this   *
 *                        function will allocate memory space, copy the *
 *                        string, and return a pointer to *strptr.      *
 *                        The space can be freed by using the C function*
 *                        free().                                       *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         strptr - a pointer to the string to be       *
 *                                  returned.                           *
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         items - the number of items in the group.    *
 *                         valid - the valid flag of the item.          *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_file_to_strptr ( FILE* fp, char** strptr, ItemInfo* item, int* read_next,
    unsigned short* group, unsigned int* valid, int* error1 )
//unsigned *valid ;
{
        char *data ;
        int len, error ;

        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (len == 0) {
            v_assign_data_error_code(error1,item) ;
            *strptr=NULL ;
            return(0) ;
        }
        data=(char *)malloc((len+1)*sizeof(char)) ;
        if (data == NULL) return(1) ;
        memset(data,0,len) ;
        error=v_ReadData(data,sizeof(char),len,fp) ;
        if (error == 0) return(2) ;
        data[len]='\0' ;
        *valid=1 ;
        *strptr=data ;
        return(0) ;
} 
      
/************************************************************************
 *                                                                      *
 *      Function        : v_decode_strptr                               *
 *      Description     : This function decodes the label string from   *
 *                        label for each axis, and put each of them into*
 *                        array axis_label. The parator between two axis*
 *                        is '\'.                                       *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  label - the label string to be decoded.      *
 *                         axis_label - label of the axis.              *
 *                         items - the number of items in the group.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified: 12/6/00 inconsistent return fixed
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_decode_strptr ( char* label, Char30* axis_label, int items )
{
        int i, j, k ;

        for (i=0; i<items; i++) strcpy(axis_label[i],"") ;
        for (i=0, j=0, k=0; label[i] != '\0'; i++) {
            if (label[i] != '\\') {
                if (k < sizeof(Char30)) axis_label[j][k++]=label[i] ;
            }
            else {
                axis_label[j][k]='\0' ;
                k=0 ;
                j++ ; 
                if (j >= items) return (0);
            }
        }
        axis_label[j][k]='\0' ;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_short                               *
 *      Description     : This function reads a short integer number    *
 *                        from input file. If the length is zero, then  *
 *                        return the INVALID to *num.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         num - the number of items.                   *
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         items - the number of items in the group.    *
 *                         valid - the valid flag of th item.           *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_file_to_short ( FILE* fp, short* num, ItemInfo* item, int* read_next,
    unsigned short* group, unsigned int* valid, int* error1 )
//unsigned *valid ;
{       
        int len, error ;

        *num=0 ;
        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (len == 0) v_assign_data_error_code(error1,item) ;
        else {
            error=v_ReadData((char*)num,sizeof(short),1,fp) ;
            if (error == 0) return(2) ;
            *valid=1 ;
        }
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_ints                                *
 *      Description     : This function reads an array of  integers from*
 *                        input file, and return a pointer to this      *
 *                        array. If the length is zero, then return the *
 *                        NULL pointer to *num. Otherwise this function *
 *                        will allocate space for this array, store the *
 *                        values, and return a pointer to *num. You can *
 *                        use C function free to free the space.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         nums - Returns a pointer to an array of the  *
 *                                short integers.                       *
 *                         count - the number of items.                 *
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         valid - the valid flag of the item.          *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on January 12, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_file_to_ints ( FILE* fp, int** nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 )
//unsigned *valid ;
{
        int len, item1, error ;
        int *nums1, *nums2 ;
        int i ;
 
        *nums=NULL ;
        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (len == 0) {
            v_assign_data_error_code(error1,item) ;
            return(0) ;
        }
        item1=len/sizeof(int);
        nums1=(int *)malloc(sizeof(int)*item1) ;
        if (nums1 == NULL) return(1) ;
        error=v_ReadData((char*)nums1,sizeof(int),item1,fp) ;
        if (error == 0) {
            free(nums1) ;
            return(2) ;
        }
        else {
            if (count == -9999) {
                *nums=nums1 ;
                *valid=1 ;
                return(0) ;
            }
            nums2=(int *)malloc(sizeof(int)*count) ;
            if (nums2 == NULL) {
                free(nums1) ;
                return(1) ;
            }
            if (item1 < count) {
                for (i=0; i<item1; i++) nums2[i]=nums1[i] ;
                for (i=item1; i<count; i++) nums2[i]= -1 ;
            }
            else for (i=0; i<count; i++) nums2[i]=nums1[i] ;
            free(nums1); 
            *nums=nums2;
            if (item1 != count) v_assign_data_error_code(error1,item) ;
            else *valid=1 ;
            return(0) ;
        }
}

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_shorts                              *
 *      Description     : This function reads an array of short integers*
 *                        from input file, and return a pointer to this *
 *                        array. If the length is zero, then return the *
 *                        NULL pointer to *num. Otherwise this function *
 *                        will allocate space for this array, store the *
 *                        values, and return a pointer to *num. You can *
 *                        use C function free to free the space.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         nums - Returns a pointer to an array of the  *
 *                                short integers.                       *
 *                         count - the number of items.  If -9999, first
 *                            item gives number of following items.
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         valid - the valid flag of the item.          *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                        Modified: 12/6/00 case of unpassed count
 *                           changed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_file_to_shorts ( FILE* fp, short** nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group,unsigned int* valid, int* error1 )
//unsigned *valid ;
{
        int len, item1, error ;
        short *nums1, *nums2 ;
        int i ;

        *nums=NULL ;
        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (len == 0) {
            v_assign_data_error_code(error1,item) ;
            return(0) ;
        }
        item1=len/2 ;
        nums1=(short *)malloc(sizeof(short)*item1) ;
        if (nums1 == NULL) return(1) ;
        error=v_ReadData((char*)nums1,sizeof(short),item1,fp) ;
        if (error == 0) {
            free(nums1) ;
            return(2) ;
        }
        else {
            if (count == -9999)
				count = nums1[0]+1;
            nums2=(short *)malloc(sizeof(short)*count) ;
            if (nums2 == NULL) {
                free(nums1) ;
                return(1) ;
            }
            if (item1 < count) {
                for (i=0; i<item1; i++) nums2[i]=nums1[i] ;
                for (i=item1; i<count; i++) nums2[i]= -1 ;
            }
            else for (i=0; i<count; i++) nums2[i]=nums1[i] ;
            free(nums1) ;
            *nums=nums2 ;
            if (item1 != count) v_assign_data_error_code(error1,item) ;
            else *valid=1 ;
            return(0) ;
        }
}

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_shorts1                             *
 *      Description     : This function reads an array of short integers*
 *                        from input file, and return a pointer to this *
 *                        array. If the length is zero, then return the *
 *                        NULL pointer to *num. Otherwise this function *
 *                        will allocate space for this array, store the *
 *                        values, and return a pointer to *num. You can *
 *                        use C function free to free the space.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         nums - Returns a pointer to an array of the  *
 *                                short integers.                       *
 *                         count - the number of items.                 *
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         valid - the valid flag of the item.          *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_file_to_shorts1 ( FILE* fp, short* nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 )
//unsigned *valid ;
{
        int len, item1, error, i ;
        short *nums1 ;

        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (len == 0) {
            for (i=0; i<count; i++) nums[i]= -1 ;
            v_assign_data_error_code(error1,item) ;
            return(0) ;
        }
        item1=len/2 ;
        nums1=(short *)malloc(sizeof(short)*item1) ;
        if (nums1 == NULL) return(1) ;
        error=v_ReadData((char*)nums1,sizeof(short),item1,fp) ;
        if (error == 0) {
            free(nums1) ;
            return(2) ;
        }
        else {
            if (item1 < count) {
                for (i=0; i<item1; i++) nums[i]=nums1[i] ;
                for (i=item1; i<count; i++) nums[i]= -1 ;
            }
            else for (i=0; i<count; i++) nums[i]=nums1[i] ;
            free(nums1) ;
            if (item1 != count) v_assign_data_error_code(error1,item) ;
            else *valid=1 ;
            return(0) ;
        }
}

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_float                               *
 *      Description     : This function reads a real number from input  *
 *                        file. If the length is zero, then return the  *
 *                        INVALID to *num.                              *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         num - the number of items.                   *
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         valid - the valid flag of the item.          *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                        Modified 4/14/97 string null-terminated
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_file_to_float ( FILE* fp, float* num, ItemInfo* item, int* read_next,
    unsigned short* group, unsigned int* valid, int* error1 )
//unsigned *valid ;
{
        int len, error ;
        char *data ;

        *num=0 ;
        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (len == 0) v_assign_data_error_code(error1,item) ; 
        else {   
            data=(char *)malloc(len+1) ;
            if (data == NULL) return(1) ;
            memset(data,0,len+1) ;
            error=v_ReadData(data,sizeof(char),len,fp) ;
            if (error == 0) return(2) ;
            sscanf(data,"%f",num) ;
            *valid=1 ;
			free(data);
        }
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_floats                              *
 *      Description     : This function reads an array of real numbers  *
 *                        from input file, and return a pointer to this *
 *                        array. if the length is zero, then return the *
 *                        pointer to *num. Otherwise this function will *
 *                        allocate space for this array, store the      *
 *                        values, and return a pointer to *num. you can *
 *                        use C function free to free the space.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         nums - Returns a pointer to an array of the  *
 *                                floats.                               *
 *                         count - the number of items.                 *
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         valid - the valid flag of the item.          *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_file_to_floats ( FILE* fp, float** nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 )
//unsigned *valid ;
{
        int len, i, j, item1, error ;
        char *data, temp[100] ;
        float *nums1, *nums2 ;

        *nums=NULL ;
        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (len == 0)  {
            v_assign_data_error_code(error1,item) ;
            return(0) ;
        }
        data=(char *)malloc(sizeof(char)*len) ;
        if (data == NULL) return(1) ;
        memset(data,0,len) ;
        error=v_ReadData(data,sizeof(char),len,fp) ;
        if (error == 0) return(2) ;
        for (i=0, item1=1; i<len; i++) 
            if (data[i] == '/' || data[i] == '\\') item1++;
        nums1=(float *)malloc(sizeof(float)*item1) ;
        if (nums1 == NULL) {
            free(data) ;
            return(1) ;
        }
        for (i=0, j=0, item1=0; i<len; i++) {
            if(data[i] != '/' && data[i] != '\\') temp[j++]=data[i];
            else {
                temp[j]='\0' ;
                sscanf(temp,"%f",&nums1[item1]) ;
                item1++ ;
                j=0 ;
            }
        }
        temp[j]='\0' ;
        sscanf(temp,"%f",&nums1[item1]) ;
        if (count == -9999) {
            *nums=nums1 ;
            free(data) ;
            *valid=1 ;
            return(0) ;
        }
        nums2=(float *)malloc(sizeof(float)*count) ;
        if (nums2 == NULL) {
            free(data) ;
            free(nums1) ;
            return(1) ;
        }
        if ((item1+1) < count) {
            for (i=0; i<(item1+1); i++) nums2[i]=nums1[i] ;
            for (i=(item1+1); i<count; i++) nums2[i]= -1. ;
        }
        else for (i=0; i<count; i++) nums2[i]=nums1[i] ;
        free(nums1) ;
        free(data) ;
        *nums=nums2 ;
        if ((item1+1) != count) v_assign_data_error_code(error1,item) ;
        else *valid=1 ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_file_to_floats1                             *
 *      Description     : This function reads an array of real numbers  *
 *                        from input file, and return a pointer to this *
 *                        array. if the length is zero, then return the *
 *                        pointer to *num. Otherwise this function will *
 *                        allocate space for this array, store the      *
 *                        values, and return a pointer to *num. you can *
 *                        use C function free to free the space.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         nums - Returns a pointer to an array of the  *
 *                                floats.                               *
 *                         count - the number of items.                 *
 *                         item - the item in consideration.            *
 *                         read_next - the next item to be read in the  *
 *                                     list of items.                   *
 *                         group - the group the item belongs to.       *
 *                         valid - the valid flag of the item.          *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_file_to_floats1 ( FILE* fp, float* nums, int count, ItemInfo* item,
    int* read_next, unsigned short* group, unsigned int* valid, int* error1 )
//unsigned *valid ;
{
        int len, i, j, item1, error ;
        char *data, temp[100] ;
        float *nums1 ;

        *valid=0 ;
        error=v_check_item(fp,read_next,group,item,error1) ;
        if (error == -1) return(0) ;
        if (error != 0) return(error) ;
        error=v_ReadData((char*)&len,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (len == 0)  {
            for (i=0; i<count; i++) nums[i]= -1. ;
            v_assign_data_error_code(error1,item) ;
            return(0) ;
        }
        data=(char *)malloc(sizeof(char)*len) ;
        if (data == NULL) return(1) ;
        memset(data,0,len) ;
        error=v_ReadData(data,sizeof(char),len,fp) ;
        if (error == 0) return(2) ;
        for (i=0, item1=1; i<len; i++) 
            if (data[i] == '/' || data[i] == '\\') item1++ ;
        nums1=(float *)malloc(sizeof(float)*item1) ;
        if (nums1 == NULL) {
            free(data) ;
            return(1) ;
        }
        for (i=0, j=0, item1=0; i<len; i++) {
            if(data[i] != '/' && data[i] != '\\') temp[j++]=data[i] ;
            else {
                temp[j]='\0' ;
                sscanf(temp,"%f",&(nums1[item1])) ;
                item1++ ;
                j=0 ;
            }
        }
        temp[j]='\0' ;
        sscanf(temp,"%f",&(nums1[item1])) ;
        if ((item1+1) < count) {
            for (i=0; i<(item1+1); i++) nums[i]=nums1[i] ;
            for (i=(item1+1); i<count; i++) nums[i]= -1. ;
        }
        else for (i=0; i<count; i++) nums[i]=nums1[i] ;
        free(nums1) ;
        free(data) ;
        if ((item1+1) != count) v_assign_data_error_code(error1,item) ;
        else *valid=1 ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VReadData                                     *
 *      Description     : This function will read a string of bytes into*
 *                        an array from the specified file pointer. This*
 *                        function assumes that fp points to the        *
 *                        beginning of the data part, and that the data *
 *                        in the file follows the big endian format for *
 *                        short (size 2), and integer (size 4) values.  *
 *                        This function would read in the data, covert  *
 *                        it to the format the machine supports and     *
 *                        return the number read in the item_read field.*
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *                         100 - incorrect 3dviewnix file header.       *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         size - size of each data item. Valid items   *
 *                                are 1, 2 & 4.                         *
 *                         items- the number of items to be read.       *
 *                         data - the array where the function puts the *
 *                                string of bytes.                      *
 *                         items_read- Returns the number of items this *
 *                                actually read.                        *
 *      Side effects    : This function can read only 1,2 or 4 byte     *
 *                        integer data. This should be expanded to      * 
 *                        handle floating point data.                   *
 *      Entry condition : If the fp, data, or items_read is NULL, or    *
 *                        items is zero, or size is not 1, 2 or 4 this  *
 *                        function will print the proper message to the *
 *                        standard error stream, producce the core dump,*
 *                        and exit from the current process.            *
 *      Related funcs   : VGetHeaderLength, VCloseData, VWriteData,     *
 *                        VSeekData.                                    *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
int VReadData ( char* data, int size, int items,FILE* fp, int* items_read )
{


  if (fp == NULL) 
    v_print_fatal_error("VReadData",
                       "The specified file pointer should not be NULL.",0) ;
  if (data == NULL) 
    v_print_fatal_error("VReadData",
                       "The data pointer should not be NULL.",0) ;
  if (items_read == NULL) 
    v_print_fatal_error("VReadData",
                       "The items_read pointer should not be NULL.",0) ;
  if ( items <= 0) 
    v_print_fatal_error("VReadData",
                       "The value of items is greater than 0.",0) ; 
  if ( size !=1 && size !=2 && size !=4) 
    v_print_fatal_error("VReadData",
                       "The value of size should be 1, 2 or 4.",0) ; 
  
  *items_read = v_ReadData(data,size,items,fp);
  if (*items_read!=items) return(2);
  
  return(0);

}

/************************************************************************
 *                                                                      *
 *      Function        : VWriteHeader                                  *
 *      Description     : This function will write the file header      *
 *                        information to a disk file specifiesd by the  *
 *                        user, from the struct vh according to the     *
 *                        data set type stored in the file header.      *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *                         7 - 3DVIEWNIX directory pathname not given   *
 *                             in environment variable.                 *
 *                         100 - incorrect 3DVIEWNIX file format.       *
 *                         102 - invalid data set type.                 *
 *                         103 - can not open specification file.       *
 *                         104 - invalid value in the file header for   *
 *                               Type 1.                                *
 *                         106 - invalid value in the file header for   *
 *                               Type 1D.                               *
 *                         107 - invalid value in the file header for   *
 *                               2, 2D, or 3.                           *
 *                         105 - invalid recognition code.              *
 *      Parameters      :  fp - pointer to the output file.             *
 *                         vh - pointer to the struct where the function*
 *                              puts the information.                   *
 *                         group - the group number of the error item.  *
 *                         element - the element number of the error    *
 *                               item.                                  *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VReadHeader.                                  *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
int VWriteHeader ( FILE* fp, ViewnixHeader* vh, char group[5], char element[5] ) 
{
        int error ;

        if (fp == NULL) 
            v_print_fatal_error("VWriteHeader",
                "The specified file pointer should not be NULL.",0) ;
        if (group == NULL) 
            v_print_fatal_error("VWriteHeader",
                "The group pointer should not be NULL.",0) ;
        if (element == NULL) 
            v_print_fatal_error("VWriteHeader",
                "The element pointer should not be NULL.",0) ;
        if (vh->gen.recognition_code_valid == 0) 
            v_print_fatal_error("VWriteHeader",
                "The recognition code flag is not 1.",
                vh->gen.recognition_code_valid) ;
        if (strcmp(vh->gen.recognition_code,"VIEWNIX1.0") == 0) {
            error=v_WriteHeader_10(fp,vh) ;
            strcpy(group,lib_group_error) ;
            strcpy(element,lib_element_error) ;
            return(error) ;
        }
        printf("The error occurred in the function VWriteHeader.\n") ;
        printf("The recognition code stored in vh is not defined properly.\n") ;
        printf("The current recognition code is %s.\n",
                vh->gen.recognition_code);
        exit(-1);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_WriteHeader_10                              *
 *      Description     : This function will write header information   *
 *                        stored in the ViewnixHeader structure vh to   *
 *                        the disk file specified by fp and check the   *
 *                        validity flag defined in the struct           *
 *                        ViewnixHeader. If flag is 0 (i.e., this item  *
 *                        is invalid), this function will write the     *
 *                        corresponding group number and element number *
 *                        (store in the specification files), and a     *
 *                        value length of 0 to the file, and ignore the *
 *                        value specified in vh.                        *
 *                        If the data item is of char type and flag is  *
 *                        1, the function will write the corresponding  *
 *                        group number, element number, value length,   *
 *                        and value to the file irrespective of what the*
 *                        value length is. If the data item is a pointer*
 *                        of type short, int, or float with a value that*
 *                        is flag is 1, then the function will write the*
 *                        corresponding group number, element number,   *
 *                        the value length (i.e., the number of bytes   *
 *                        corresponding to the value in vh) and the     *
 *                        value(s) in ViewnixHeader to the file. If the *
 *                        data item is of type 1D and flag is 0, the    *
 *                        function will assign a default value(s) to the*
 *                        item and write the group number, element      *
 *                        number, length of the value(s) and the        *
 *                        value(s) to the file (no error returned).     *
 *                        If the value of the recognition code stored   *
 *                        in vh does not have any matching set of       *
 *                        specification file and ViewnixHeader struct,  *
 *                        the function will give fatal error. Otherwise,*
 *                        the matching files and struct will be used for*
 *                        writing the data.                             *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         2 - read error.                              *
 *                         5 - improper seeks.                          *
 *                         7 - 3DVIEWNIX directory pathname not given   *
 *                             in environment variable.                 *
 *                         102 - invalid data set type.                 *
 *                         103 - cannot open specification file.        *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         vh - pointer to the struct where the function*
 *                              the information.                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader, VGetHeaderLength.               *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_WriteHeader_10 ( FILE* fp, ViewnixHeader* vh )
{
        /*char msg[80] ; */
        int error, grplen ;
		long offset;
        static short scene_grp[6]={0x0029,0,0,4,0,0} ;
        static short structure_grp[6]={0x002B,0,0,4,0,0} ;
        static short display_grp[6]={0x002D,0,0,4,0,0} ;
        int items, error1 ;

        if (vh->gen.data_type_valid == 0) return(102) ;
        items=error1=0 ;
        switch (vh->gen.data_type) {
            case IMAGE0 : /* scene of type IMAGE0 */
            case IMAGE1 : /* scene of type IMAGE1 */
                error=v_read_items("1.0",0) ;
                if (error != 0) return(error) ;
                error=v_write_general_10(fp,&(vh->gen), &offset,&grplen,&items,
                                       &error1) ;
                if (error != 0) return(error) ;
                error=v_write_scene_10(fp,&(vh->scn), &offset,&grplen,&items,
                                     &error1);
                if (error != 0) return(error) ;
                error=v_write_len(fp, &offset,&grplen,structure_grp) ;
                if (error != 0) return(error) ;
                error=v_write_len(fp, &offset,&grplen,display_grp) ;
                if (error != 0) return(error) ;
                break ;
            case CURVE0 : /* structure date of type CURVE0 */
            case SURFACE0: /* structure data of type SURFACE0 */
            case SURFACE1: /* structure data of type SURFACE1 */
            case SHELL1: /* structure data of type SHELL1 */
            case SHELL0 : /* structure data of type SHELL */
            case SHELL2: /* structure data of type SHELL2 */
                error=v_read_items("1.0",1) ;
                if (error != 0) return(error) ;
                error=v_write_general_10(fp,&(vh->gen), &offset,&grplen,&items,
                                       &error1) ;
                if (error != 0) return(error) ;
                error=v_write_len(fp, &offset,&grplen,scene_grp) ;
                if (error != 0) return(error) ;
                error=v_write_structure_10(fp,&(vh->str), &offset,
                        &grplen,&items,&error1);
                if (error != 0) return(error) ;
                error=v_write_len(fp, &offset,&grplen,display_grp) ;
                if (error != 0) return(error) ;
                break ;
            case MOVIE0 :  /* display data of type MOVIE0 */
                error=v_read_items("1.0",2) ;
                if (error != 0) return(error) ;
                error=v_write_general_10(fp,&(vh->gen), &offset,&grplen,&items,
                                       &error1) ;
                if (error != 0) return(error) ;
                error=v_write_len(fp, &offset,&grplen,scene_grp) ;
                if (error != 0) return(error) ;
                error=v_write_len(fp, &offset,&grplen,structure_grp) ;
                if (error != 0) return(error) ;
                error=v_write_display_10(fp,&(vh->dsp), &offset,&grplen,&items,
                                       &error1) ;
                if (error != 0) return(error) ;
                break ;
            default :
                return(102) ;
                break ;
        }
        error=v_close_header(fp,offset,grplen,vh->gen.data_type);
        if (error == 0) return(error1) ;
        else return(error) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_write_general_10                            *
 *      Description     : This function writes the general information  *
 *                        contained by the GeneralInfo structure to     *
 *                        header of the file pointed by fp.             *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         gen - pointer to general information data in *
 *                               the file header.                       *
 *                         offset - Returns the bytes to be skipped     *
 *                               from the beginning of file for the last*
 *                               group in this function.                *
 *                         len - Returns the length of the last group   *
 *                               in this function.                      *
 *                         items - the number of items in the group.    *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_write_general_10 ( FILE* fp, GeneralInfo* gen, long* offset, int* len,
    int* items, int* error1 )
{
        static short cmd_grp[12]={0,0,0,4,0,12,0,1,0,4,0,0} ;
        static short ident_grp[12]={0x0008,0,0,4,0,0,8,1,0,4,0,0} ;
        static short gen_grp[6]={0x0009,0,0,4,0,0} ;
        static short pat_grp[6]={0x0010,0,0,4,0,0} ;
        static short acq_grp[6]={0x0018,0,0,4,0,0} ;
        static short rel_grp[6]={0x0020,0,0,4,0,0} ;
        static short img_grp[6]={0x0028,0,0,4,0,0} ;
        char temp[20] ;
        long skip ;
        int grplen, error, i ;

        error=fseek(fp,0,0) ;
        if (error == -1) return(5) ;
        error=v_WriteData((unsigned char*)cmd_grp,sizeof(short),12,fp) ; 
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)ident_grp,sizeof(short),12,fp) ;
        if (error == 0) return(3) ;
        grplen=12 ;
        skip=32 ;
        i= *items ;
        error=v_string_to_file(fp,gen->recognition_code,
                             gen->recognition_code_valid,
                             &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        v_cvt_delimiter(temp,gen->study_date,'.') ;
        error=v_string_to_file(fp,temp,gen->study_date_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        v_cvt_delimiter(temp,gen->study_time,':') ;
        error=v_string_to_file(fp,temp,gen->study_time_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_short_to_file(fp,gen->data_type,gen->data_type_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->modality,gen->modality_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->institution,gen->institution_valid,
                             &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->physician,gen->physician_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->department,gen->department_valid,
                                &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->radiologist,gen->radiologist_valid,
                             &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ; 
        error=v_string_to_file(fp,gen->model,gen->model_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_write_len(fp,&skip,&grplen,gen_grp) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->filename,gen->filename_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->filename1,gen->filename1_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->description,gen->description_valid,
                             &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->comment,gen->comment_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_write_len(fp,&skip,&grplen,pat_grp) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->patient_name,gen->patient_name_valid,
                             &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->patient_id,gen->patient_id_valid,
                                &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_write_len(fp,&skip,&grplen,acq_grp) ;
        if (error != 0) return(error) ;
        error=v_float_to_file(fp,gen->slice_thickness,
                gen->slice_thickness_valid,&grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_floats_to_file(fp,gen->kvp,gen->kvp_valid,2,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_float_to_file(fp,gen->repetition_time,
                gen->repetition_time_valid,&grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_float_to_file(fp,gen->echo_time,gen->echo_time_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->imaged_nucleus,
                gen->imaged_nucleus_valid,&grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_float_to_file(fp,gen->gantry_tilt,gen->gantry_tilt_valid,
                        &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_write_len(fp,&skip,&grplen,rel_grp) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->study,gen->study_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,gen->series,gen->series_valid,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_write_len(fp,&skip,&grplen,img_grp) ;
        if (error != 0) return(error) ;
        error=v_shorts_to_file(fp,gen->gray_descriptor,
                             gen->gray_descriptor_valid,
                             3,&grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_shorts_to_file(fp,gen->red_descriptor,
                gen->red_descriptor_valid,3,&grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_shorts_to_file(fp,gen->green_descriptor,
                             gen->green_descriptor_valid,3,&grplen,
                             &acr_items[i++],error1);
        if (error != 0) return(error) ;
        error=v_shorts_to_file(fp,gen->blue_descriptor,
                             gen->blue_descriptor_valid,
                             3,&grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        if (gen->gray_descriptor_valid == 1) {
            if (gen->gray_lookup_data_valid == 1 && 
                gen->gray_descriptor[0] != 0 && gen->gray_lookup_data == NULL)
                v_print_fatal_error("VWriteHeader",
                    "gen->gray_lookup_data should not be NULL.",
                    0);
            error=v_shorts_to_file(fp, (short*)gen->gray_lookup_data,
                                 gen->gray_lookup_data_valid,
                                 gen->gray_descriptor[0],
                                 &grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        else {
            if (gen->gray_lookup_data_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "gen->gray_descriptor_valid should be 1.",
                    gen->gray_descriptor_valid) ;
            error=v_shorts_to_file(fp, (short*)gen->gray_lookup_data,
                                 gen->gray_lookup_data_valid,0,
                                 &grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        if (gen->red_descriptor_valid == 1) {
            if (gen->red_lookup_data_valid == 1 && 
                gen->red_descriptor[0] != 0 && gen->red_lookup_data == NULL)
                v_print_fatal_error("VWriteHeader",
                    "gen->red_lookup_data should not be NULL.",
                    0);
            error=v_shorts_to_file(fp, (short*)gen->red_lookup_data,
                                 gen->red_lookup_data_valid,
                                 gen->red_descriptor[0],
                                 &grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        else {
            if (gen->red_lookup_data_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "gen->red_descriptor_valid should be 1.",
                    gen->red_descriptor_valid) ;
            error=v_shorts_to_file(fp, (short*)gen->red_lookup_data,
                                 gen->red_lookup_data_valid,0,
                                 &grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        if (gen->green_descriptor_valid == 1) {
            if (gen->green_lookup_data_valid == 1 && 
                gen->green_descriptor[0] != 0 && gen->green_lookup_data == NULL)
                v_print_fatal_error("VWriteHeader",
                    "gen->green_lookup_data should not be NULL.",
                    0);
            error=v_shorts_to_file(fp, (short*)gen->green_lookup_data,
                                 gen->green_lookup_data_valid,
                                 gen->green_descriptor[0],
                                 &grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        else {
            if (gen->green_lookup_data_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "gen->green_descriptor_valid should be 1.",
                    gen->green_descriptor_valid) ;
            error=v_shorts_to_file(fp, (short*)gen->green_lookup_data,
                                 gen->green_lookup_data_valid,0,
                                 &grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        if (gen->blue_descriptor_valid == 1) {
            if (gen->blue_lookup_data_valid == 1 && 
                gen->blue_descriptor[0] != 0 && gen->blue_lookup_data == NULL)
                v_print_fatal_error("VWriteHeader",
                    "gen->blue_lookup_data should not be NULL.",
                    0);
            error=v_shorts_to_file(fp, (short*)gen->blue_lookup_data,
                                 gen->blue_lookup_data_valid,
                                 gen->blue_descriptor[0],
                                 &grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        else {
            if (gen->blue_lookup_data_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "gen->blue_descriptor_valid should be 1.",
                    gen->blue_descriptor_valid) ;
            error=v_shorts_to_file(fp, (short*)gen->blue_lookup_data,
                                 gen->blue_lookup_data_valid,0,
                                 &grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        *offset=skip ;
        *len=grplen ;
        *items=i ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_write_scene_10                              *
 *      Description     : This function writes the scene information    *
 *                        contained by the SceneInfo structure to       *
 *                        header of the file pointed by fp.             *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         scn - pointer to scene information data in   *
 *                               the file header.                       *
 *                         offset - Returns the bytes to be skipped     *
 *                               from the beginning of file for the last*
 *                               group in this function.                *
 *                         len - Returns the length of the last group   *
 *                               in this function.                      *
 *                         items - the number of items in the group.    *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                        Modified 3/5/97 label freed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_write_scene_10 ( FILE* fp, SceneInfo* scn, long* offset, int* len,
    int* items, int* error1 )
{ 
        int grplen, error, i, j, total, total1 ;
        long skip ;
        char *label=NULL;
        static short scn_grp[6]={0x0029,0,0,4,0,0} ;

        skip=(*offset) ;
        grplen=(*len) ;
        i= *items ;
        error=v_write_len(fp,&skip,&grplen,scn_grp) ;
        if (error != 0) return(error) ;
        error=v_short_to_file(fp,scn->dimension,scn->dimension_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        if (scn->dimension_valid == 1) {
            if (scn->domain == NULL && scn->domain_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->domain should not be NULL.",0) ;
            error=v_floats_to_file(fp,scn->domain,scn->domain_valid,
                                 (scn->dimension+1)*scn->dimension,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (scn->domain_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->dimension_valid should be 1.",scn->dimension_valid);
            else {
                error=v_floats_to_file(fp,scn->domain,scn->domain_valid,
                                     (scn->dimension+1)*scn->dimension,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
        if (scn->dimension_valid == 1) {
            if (scn->axis_label_valid == 1) {
                label=(char *)malloc(sizeof(Char30)*scn->dimension) ;
                if (label == NULL) return(1) ;
                for (j=0; j<scn->dimension; j++) {
                    if (j == 0) sprintf(label,"%s",scn->axis_label[j]) ;
                    else sprintf(label,"%s\\%s",label,scn->axis_label[j]) ;
                }
            }
            else label=NULL ;
            error=v_string_to_file(fp,label,scn->axis_label_valid,&grplen,
                                 &acr_items[i++],error1) ;
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->axis_label_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->dimension_valid should be 1.",scn->dimension_valid) ;
            else {
                error=v_string_to_file(fp,NULL,scn->axis_label_valid,&grplen,
                                     &acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
        if (scn->dimension_valid == 1) {        
            if (scn->measurement_unit == NULL && 
                scn->measurement_unit_valid == 1)    
                v_print_fatal_error("VWriteHeader",
                    "scn->measurement_unit should not be NULL.",
                    0) ;
            error=v_shorts_to_file(fp,scn->measurement_unit,
                                 scn->measurement_unit_valid,
                                 scn->dimension,&grplen,&acr_items[i++],
                                 error1) ;
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->measurement_unit_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->dimension_valid should be 1.",scn->dimension_valid);
            else {
                error=v_shorts_to_file(fp,scn->measurement_unit,
                                     scn->measurement_unit_valid,
                                     scn->dimension,&grplen,&acr_items[i++],
                                     error1) ;
                if (error != 0) {if(label)free(label); return(error);}
            }
        }

        error=v_short_to_file(fp,scn->num_of_density_values,
                            scn->num_of_density_values_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) {if(label)free(label); return(error);}


        if (scn->num_of_density_values_valid == 1) {    
            if (scn->density_measurement_unit == NULL && 
                scn->density_measurement_unit_valid == 1)    
                v_print_fatal_error("VWriteHeader",
                    "scn->density_measurement_unit should not be NULL.",
                    0) ;
            error=v_shorts_to_file(fp,scn->density_measurement_unit,
                                 scn->density_measurement_unit_valid,
                                 scn->num_of_density_values,
                                 &grplen,&acr_items[i++],
                                 error1) ;
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->density_measurement_unit_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->num_of_density_values_valid should be 1.",
                                   scn->num_of_density_values_valid) ;
            else {
                error=v_shorts_to_file(fp,scn->density_measurement_unit,
                                     scn->density_measurement_unit_valid,
                                     scn->num_of_density_values,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) {if(label)free(label); return(error);}
              }
        }
        

        if (scn->num_of_density_values_valid == 1) {
            if (scn->smallest_density_value == NULL &&
                scn->smallest_density_value_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "scn->smallest_density_value should not be NULL.",
                    0) ;
            error=v_floats_to_file(fp,scn->smallest_density_value,
                                 scn->smallest_density_value_valid,
                                 scn->num_of_density_values,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->smallest_density_value_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->num_of_density_values_valid should be 1.",
                    scn->num_of_density_values_valid) ;
            else {
                error=v_floats_to_file(fp,scn->smallest_density_value,
                                     scn->smallest_density_value_valid,
                                     scn->num_of_density_values,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) {if(label)free(label); return(error);}
            }
        }
        if (scn->num_of_density_values_valid == 1) {
            if (scn->largest_density_value == NULL &&
                scn->largest_density_value_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "scn->largest_density_value should not be NULL.",
                    0) ;
            error=v_floats_to_file(fp,scn->largest_density_value,
                                 scn->largest_density_value_valid,
                                 scn->num_of_density_values,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->largest_density_value_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->num_of_density_values_valid should be 1.",
                    scn->num_of_density_values_valid) ;
            else {
                error=v_floats_to_file(fp,scn->largest_density_value,
                                     scn->largest_density_value_valid,
                                     scn->num_of_density_values,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) {if(label)free(label); return(error);}
            }
        }
        error=v_short_to_file(fp,scn->num_of_integers,
                scn->num_of_integers_valid,&grplen,&acr_items[i++],error1) ;
        if (error != 0) {if(label)free(label); return(error);}
        if (scn->num_of_integers_valid == 1) {
            if (scn->signed_bits == NULL && scn->signed_bits_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "scn->signed_bits should not be NULL.",0) ;
            error=v_shorts_to_file(fp,scn->signed_bits,scn->signed_bits_valid,
                                 scn->num_of_integers,&grplen,&acr_items[i++],
                                 error1) ;
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->signed_bits_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->num_of_integers_valid should be 1.",
                    scn->num_of_integers_valid) ;
            else {
                error=v_shorts_to_file(fp,scn->signed_bits,
                                  scn->signed_bits_valid,
                                  scn->num_of_integers,&grplen,
                                  &acr_items[i++],error1) ;
                if (error != 0) {if(label)free(label); return(error);}
            }
        }
        error=v_short_to_file(fp,scn->num_of_bits,scn->num_of_bits_valid,
                            &grplen,&acr_items[i++],error1) ;
        if (error != 0) {if(label)free(label); return(error);}
        if (scn->num_of_density_values_valid == 1) {
            if (scn->bit_fields_valid == 1 && scn->bit_fields == NULL) 
                v_print_fatal_error("VWriteHeader",
                    "scn->bit_fileds should not be NULL.",0) ;
            error=v_shorts_to_file(fp,scn->bit_fields,scn->bit_fields_valid,
                                 scn->num_of_density_values*2,&grplen,
                                 &acr_items[i++],error1) ;
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->bit_fields_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->num_of_density_values_valid should be 1.",
                    scn->num_of_density_values_valid) ;
            else {
                error=v_shorts_to_file(fp,scn->bit_fields,
                scn->bit_fields_valid,0,&grplen,&acr_items[i++],error1) ;
                if (error != 0) {if(label)free(label); return(error);}
            }
        }
        error=v_short_to_file(fp,scn->dimension_in_alignment,
                            scn->dimension_in_alignment_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) {if(label)free(label); return(error);}
        error=v_short_to_file(fp,scn->bytes_in_alignment,
                            scn->bytes_in_alignment_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) {if(label)free(label); return(error);}
        error=v_shorts_to_file(fp,scn->xysize,scn->xysize_valid,2,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) {if(label)free(label); return(error);}
        if (scn->dimension_valid == 1) {
            if (scn->num_of_subscenes_valid == 1 && scn->num_of_subscenes == NULL)
                 v_print_fatal_error("VWriteHeader",
                    "scn->num_of_subscenes should not be NULL.",
                    0) ;
            if (scn->num_of_subscenes_valid == 1)
                v_count_samples_memory_space(&total,scn->num_of_subscenes,
                                           scn->dimension) ;
            else total=0 ;
            error=v_shorts_to_file(fp,scn->num_of_subscenes,
                                 scn->num_of_subscenes_valid,
                                 total,&grplen,&acr_items[i++],error1);
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->num_of_subscenes_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->dimension_valid should be 1.",scn->dimension_valid);
            else {
                error=v_shorts_to_file(fp,scn->num_of_subscenes,
                                     scn->num_of_subscenes_valid,0,&grplen,
                                     &acr_items[i++],error1) ;
                if (error != 0) {if(label)free(label); return(error);}
            }
        }
        error=v_floats_to_file(fp,scn->xypixsz,scn->xypixsz_valid,2,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) {if(label)free(label); return(error);}
        if (scn->num_of_subscenes_valid == 1) {
            if (scn->loc_of_subscenes == NULL && scn->loc_of_subscenes_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "scn->loc_of_subscenes should not be NULL.",
                    0);
            if (scn->num_of_subscenes_valid == 1)
                for (j=0, total1=0; j<total; j++)
                    total1 += scn->num_of_subscenes[j] ;
            else total1=0 ;
            error=v_floats_to_file(fp,scn->loc_of_subscenes,
                             scn->loc_of_subscenes_valid,total1,&grplen,
                             &acr_items[i++],error1);
            if (error != 0) {if(label)free(label); return(error);}
        }
        else {
            if (scn->loc_of_subscenes_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "scn->num_of_subscenes_valid should be 1.",
                    scn->num_of_subscenes_valid) ;
            else {
                error=v_floats_to_file(fp,scn->loc_of_subscenes,
                                     scn->loc_of_subscenes_valid,0,&grplen,
                                     &acr_items[i++],error1) ;
                if (error != 0) {if(label)free(label); return(error);}
            }
        }
        error=v_string_to_file(fp,scn->description,scn->description_valid,
                             &grplen,&acr_items[i++],error1) ;
        if (error != 0) {if(label)free(label); return(error);}
        *offset=skip ;
        *len=grplen ;
        *items=i ;
        if (label)
			free(label);
		return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_write_structure_10                          *
 *      Description     : This function writes the structure information*
 *                        contained by the StructureInfo structure to   *
 *                        header of the file pointed by fp.             *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         str - pointer to structure information data  *
 *                               in the file header.                    *
 *                         offset - Returns the bytes to be skipped     *
 *                               from the beginning of file for the last*
 *                               group in this function.                *
 *                         len - Returns the length of the last group   *
 *                               in this function.                      *
 *                         items - the number of items in the group.    *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_write_structure_10 ( FILE* fp, StructureInfo* str, long* offset,
    int* len, int* items, int* error1 )
{ 
        int grplen, error, i, j, total, total1 ;
        long skip ;
        static short structure_grp[6]={0x002B,0,0,4,0,0} ;
        char *label ;
        short num_of_structures,num_of_elements;

        skip=(*offset) ;
        grplen=(*len) ;
        i= *items ;
        error=v_write_len(fp,&skip,&grplen,structure_grp) ;
        if (error != 0) return(error) ;

                        /**002B 8000****/
        error=v_short_to_file(fp,str->dimension,str->dimension_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;


                        /**002B 8005****/
        error=v_short_to_file(fp,str->num_of_structures,
                            str->num_of_structures_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        if (str->num_of_structures_valid)
          num_of_structures=str->num_of_structures;
        else 
          num_of_structures=1;


                        /**002B 8010****/
        if (str->dimension_valid == 1) {
          if (str->domain == NULL && str->domain_valid == 1) 
            v_print_fatal_error("VWriteHeader",
                               "str->domain should not be NULL.",0) ;
          error=v_floats_to_file(fp,str->domain,str->domain_valid,
                        num_of_structures*(str->dimension+1)*str->dimension,
                        &grplen,&acr_items[i++],error1) ;
          if (error != 0) return(error) ;
        }
        else {
          if (str->domain_valid == 1) 
            v_print_fatal_error("VWriteHeader",
                               "str->dimension_valid should be 1.",str->dimension_valid) ;
          else {
            error=v_floats_to_file(fp,str->domain,str->domain_valid,
                 num_of_structures*(str->dimension+1)*str->dimension,
                                  &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
          }
        }

                        /**002B 8015****/
        if (str->dimension_valid == 1) {
            if (str->axis_label_valid == 1) {
                label=(char *)malloc(sizeof(Char30)*str->dimension) ;
                if (label == NULL) return(1) ;
                for (j=0; j<str->dimension; j++) {
                    if (j == 0) sprintf(label,"%s",str->axis_label[j]) ;
                    else sprintf(label,"%s\\%s",label,str->axis_label[j]) ;
                }
            }
            else label=NULL ;   
            error=v_string_to_file(fp,label,str->axis_label_valid,&grplen,
                                 &acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (str->axis_label_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->dimension_valid should be 1.",str->dimension_valid);
            else {
                error=v_string_to_file(fp,NULL,str->axis_label_valid,&grplen,
                                     &acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }

                        /**002B 8020****/
        if (str->dimension_valid == 1) {        
            if (str->measurement_unit==NULL && str->measurement_unit_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "str->measurement_unit should not be NULL.",
                    0) ;
            error=v_shorts_to_file(fp,str->measurement_unit,
                                 str->measurement_unit_valid,
                                 str->dimension,&grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (str->measurement_unit_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->dimension_valid should be 1.",str->dimension_valid);
            else {
                error=v_shorts_to_file(fp,str->measurement_unit,
                                     str->measurement_unit_valid,
                                     str->dimension,&grplen,&acr_items[i++],
                                     error1) ;
                if (error != 0) return(error) ;
            }
        }

        

                        /**002B 8025****/
        if (str->scene_file_valid == 1) {
          label=(char *)malloc(sizeof(Char30)*num_of_structures) ;
          if (label == NULL) return(1) ;
          for (j=0; j<num_of_structures; j++) {
            if (j == 0) sprintf(label,"%s",str->scene_file[j]) ;
            else sprintf(label,"%s\\%s",label,str->scene_file[j]) ;
          }
        }
        else label=NULL ;       
        error=v_string_to_file(fp,label,str->scene_file_valid,&grplen,
                              &acr_items[i++],error1) ;
        if (error != 0) return(error) ;


                        /**002B 8030****/
        if (str->num_of_TSE==NULL && str->num_of_TSE_valid == 1)
          v_print_fatal_error("VWriteHeader",
                             "str->num_of_TSE should not be NULL.",
                             0) ;
        error=v_ints_to_file(fp, (int*)str->num_of_TSE,
                            str->num_of_TSE_valid,num_of_structures,
                            &grplen,&acr_items[i++],error1);
        if (error != 0) return(error) ;


                        /**002B 8040****/
        if (str->num_of_NTSE==NULL && str->num_of_NTSE_valid == 1)
          v_print_fatal_error("VWriteHeader",
                             "str->num_of_NTSE should not be NULL.",
                             0) ;
        error=v_ints_to_file(fp, (int*)str->num_of_NTSE,
                            str->num_of_NTSE_valid,num_of_structures,
                            &grplen,&acr_items[i++],error1);
        if (error != 0) return(error) ;
        

                        /**002B 8050****/
        error=v_short_to_file(fp,str->num_of_components_in_TSE,
                            str->num_of_components_in_TSE_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;

                        /**002B 8060****/
        error=v_short_to_file(fp,str->num_of_components_in_NTSE,
                            str->num_of_components_in_NTSE_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;



                        /**002B 8065****/
        if (str->num_of_components_in_TSE_valid == 1) { 
          if (str->TSE_measurement_unit==NULL && str->TSE_measurement_unit_valid == 1)
            v_print_fatal_error("VWriteHeader",
                          "str->TSE_measurement_unit should not be NULL.",0);
          error=v_shorts_to_file(fp,str->TSE_measurement_unit,
                                 str->TSE_measurement_unit_valid,
                                 str->num_of_components_in_TSE,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
          if (str->TSE_measurement_unit_valid == 1) 
            v_print_fatal_error("VWriteHeader",
                               "str->num_of_components_in_TSE_valid should be 1.",
                               str->num_of_components_in_TSE_valid) ;
          else {
            error=v_shorts_to_file(fp,str->TSE_measurement_unit,
                                  str->TSE_measurement_unit_valid,
                                  str->num_of_components_in_TSE,&grplen,&acr_items[i++],
                                  error1) ;
            if (error != 0) return(error) ;
          }
        }



                        /**002B 8066****/
        if (str->num_of_components_in_NTSE_valid == 1) {        
          if (str->NTSE_measurement_unit==NULL && str->NTSE_measurement_unit_valid == 1)
            v_print_fatal_error("VWriteHeader",
                               "str->NTSE_measurement_unit should not be NULL.",
                               0) ;
          error=v_shorts_to_file(fp,str->NTSE_measurement_unit,
                                 str->NTSE_measurement_unit_valid,
                                 str->num_of_components_in_NTSE,&grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
          if (str->NTSE_measurement_unit_valid == 1) 
            v_print_fatal_error("VWriteHeader",
                               "str->num_of_components_in_NTSE_valid should be 1.",
                               str->num_of_components_in_NTSE_valid) ;
          else {
            error=v_shorts_to_file(fp,str->NTSE_measurement_unit,
                                  str->NTSE_measurement_unit_valid,
                                  str->num_of_components_in_NTSE,&grplen,&acr_items[i++],
                                  error1) ;
            if (error != 0) return(error) ;
          }
        }
        




                        /**002B 8070****/
        if (str->num_of_components_in_TSE_valid == 1 &&
                str->num_of_structures_valid == 1) {
            if (str->smallest_value == NULL && str->smallest_value_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "str->smallest_value should not be NULL.",
                    0) ;
            error=v_floats_to_file(fp,str->smallest_value,
                                 str->smallest_value_valid,
                                 str->num_of_components_in_TSE*
                                 str->num_of_structures,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (str->smallest_value_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->num_of_components_in_TSE_valid should be 1.",
                    str->num_of_components_in_TSE_valid) ;
            else {
                error=v_floats_to_file(fp,str->smallest_value,
                                     str->smallest_value_valid,0,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
                        /**002B 8080****/
        if (str->num_of_components_in_TSE_valid == 1 &&
                str->num_of_structures_valid ==1) {
            if (str->largest_value == NULL && str->largest_value_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "str->largest_value should not be NULL.",
                    0) ;
            error=v_floats_to_file(fp,str->largest_value,
                                 str->largest_value_valid,
                                 str->num_of_components_in_TSE*
                                 str->num_of_structures,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (str->largest_value_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->num_of_components_in_TSE_valid should be 1.",
                    str->num_of_components_in_TSE_valid) ;
            else {
                error=v_floats_to_file(fp,str->largest_value,
                                     str->largest_value_valid,0,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }

                        /**002B 8090****/
        error=v_short_to_file(fp,str->num_of_integers_in_TSE,
                            str->num_of_integers_in_TSE_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;

                        /**002B 80A0****/
        if (str->num_of_integers_in_TSE_valid == 1) {
            if (str->signed_bits_in_TSE == NULL && 
                str->signed_bits_in_TSE_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "str->signed_bits_in_TSE should not be NULL.",
                    0) ;
            error=v_shorts_to_file(fp,str->signed_bits_in_TSE,
                                 str->signed_bits_in_TSE_valid,
                                 str->num_of_integers_in_TSE,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (str->signed_bits_in_TSE_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->num_of_integers_in_TSE_valid should be 1.",
                    str->num_of_integers_in_TSE_valid) ;
            else {
                error=v_shorts_to_file(fp,str->signed_bits_in_TSE,
                                     str->signed_bits_in_TSE_valid,0,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }

                        /**002B 80B0****/
        error=v_short_to_file(fp,str->num_of_bits_in_TSE,
                            str->num_of_bits_in_TSE_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;

                        /**002B 80C0****/
        if (str->num_of_components_in_TSE_valid == 1) {
            if (str->bit_fields_in_TSE == NULL && 
                str->bit_fields_in_TSE_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "str->bit_fields_in_TSE should not be NULL.",
                    0) ;
            error=v_shorts_to_file(fp,str->bit_fields_in_TSE,
                                 str->bit_fields_in_TSE_valid,
                                 2*str->num_of_components_in_TSE,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (str->bit_fields_in_TSE_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->num_of_components_in_TSE_valid should be 1.",
                    str->num_of_components_in_TSE_valid) ;
            else {
                error=v_shorts_to_file(fp,str->bit_fields_in_TSE,
                                     str->bit_fields_in_TSE_valid,0,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }

                        /**002B 80D0****/
        error=v_short_to_file(fp,str->num_of_integers_in_NTSE,
                            str->num_of_integers_in_NTSE_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;

                        /**002B 80E0****/
        if (str->num_of_integers_in_NTSE_valid == 1) {
            if (str->signed_bits_in_NTSE == NULL && 
                str->signed_bits_in_NTSE_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "str->signed_bits_in_NTSE should not be NULL.",
                    0) ;
            error=v_shorts_to_file(fp,str->signed_bits_in_NTSE,
                                 str->signed_bits_in_NTSE_valid,
                                 str->num_of_integers_in_NTSE,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (str->signed_bits_in_NTSE_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->num_of_integers_in_NTSE_valid should be 1.",
                    str->num_of_integers_in_NTSE_valid) ;
            else {
                error=v_shorts_to_file(fp,str->signed_bits_in_NTSE,
                                     str->signed_bits_in_NTSE_valid,0,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }

                        /**002B 80F0****/
        error=v_short_to_file(fp,str->num_of_bits_in_NTSE,
                            str->num_of_bits_in_NTSE_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;

                        /**002B 8100****/
        if (str->num_of_components_in_NTSE_valid == 1) {
            if (str->bit_fields_in_NTSE == NULL && 
                str->bit_fields_in_NTSE_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "str->bit_fields_in_NTSE should not be NULL.",
                    0) ;
            error=v_shorts_to_file(fp,str->bit_fields_in_NTSE,
                                 str->bit_fields_in_NTSE_valid,
                                 2*str->num_of_components_in_NTSE,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (str->bit_fields_in_NTSE_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->num_of_components_in_NTSE_valid should be 1.",
                    str->num_of_components_in_NTSE_valid) ;
            else {
                error=v_shorts_to_file(fp,str->bit_fields_in_NTSE,
                                     str->bit_fields_in_NTSE_valid,0,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }

                        /**002B 8110****/
        if (str->dimension_valid == 1) {
            if (str->num_of_samples_valid == 1 && str->num_of_samples == NULL)
                 v_print_fatal_error("VWriteHeader",
                    "str->num_of_samples should not be NULL.",
                    0) ;
            if (str->num_of_samples_valid == 1)
                v_count_samples_memory_space(&total,str->num_of_samples,
                                           str->dimension) ;
            else total=0 ;
            error=v_shorts_to_file(fp,str->num_of_samples,
                                 str->num_of_samples_valid,
                                 total,&grplen,&acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        else {
            if (str->num_of_samples_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->dimension_valid should be 1.",str->dimension_valid);
            else {
                error=v_shorts_to_file(fp,str->num_of_samples,
                                     str->num_of_samples_valid,0,&grplen,
                                     &acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }

                        /**002B 8120****/
        error=v_floats_to_file(fp,str->xysize,str->xysize_valid,2,&grplen,
                             &acr_items[i++],error1);
        if (error != 0) return(error) ;

                        /**002B 8130****/
        if (str->num_of_samples_valid == 1) {
            if (str->loc_of_samples == NULL && str->loc_of_samples_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "str->loc_of_samples should not be NULL.",
                    0);
            if (str->num_of_samples_valid == 1)
                for (j=0, total1=0; j<total; j++)
                    total1 += str->num_of_samples[j] ;
            else total1=0 ;
            error=v_floats_to_file(fp,str->loc_of_samples,
                                 str->loc_of_samples_valid,total1,&grplen,
                                 &acr_items[i++],error1);
            if (error != 0) return(error) ;
        }
        else {
            if (str->loc_of_samples_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "str->num_of_samples_valid should be 1.",
                    str->num_of_samples_valid) ;
            else {
                error=v_floats_to_file(fp,str->loc_of_samples,
                                     str->loc_of_samples_valid,0,&grplen,
                                     &acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
                        /**002B 8131****/
        error=v_short_to_file(fp,str->num_of_elements,
                            str->num_of_elements_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        if (str->num_of_elements_valid)
          num_of_elements=str->num_of_elements;
        else
          num_of_elements=1;

                        /**002B 8132****/
        if (str->description_of_element == NULL && 
            str->description_of_element_valid ==1) 
          v_print_fatal_error("VWriteHeader",
                             "str->description_of_element should not be NULL.",
                             0);
        error=v_shorts_to_file(fp,str->description_of_element,
                              str->description_of_element_valid,
                              num_of_elements,&grplen,&acr_items[i++],
                              error1) ;
        if (error != 0) return(error) ;
        
                        /**002B 8134****/
        if (str->parameter_vectors == NULL &&
            str->parameter_vectors_valid ==1) 
          v_print_fatal_error("VWriteHeader",
                             "str->parameter_vectors should not be NULL.",
                             0);
        error=v_floats_to_file(fp,str->parameter_vectors,
                              str->parameter_vectors_valid,
                              num_of_elements*num_of_structures,
                              &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        

                        /**002B 8140****/
        if (str->dimension_valid == 1) {
          if (str->min_max_coordinates == NULL && 
              str->min_max_coordinates_valid == 1)
            v_print_fatal_error("VWriteHeader",
                               "str->min_max_coordinates should not be NULL.",
                               0);
          error=v_floats_to_file(fp,str->min_max_coordinates,
                                str->min_max_coordinates_valid,
                                str->dimension*num_of_structures*2,
                                &grplen,&acr_items[i++],error1);
          if (error != 0) return(error) ;
        }
        else {
          if (str->min_max_coordinates_valid == 1) 
            v_print_fatal_error("VWriteHeader",
                               "str->dimension_valid should be 1.",
                               str->dimension_valid) ;
          else {
            error=v_floats_to_file(fp,str->min_max_coordinates,
                                  str->min_max_coordinates_valid,0,&grplen,
                                  &acr_items[i++],error1) ;
            if (error != 0) return(error) ;
          }
        }

                        /**002B 8150****/
        if (str->volume == NULL &&
            str->volume_valid ==1)
          v_print_fatal_error("VWriteHeader",
                             "str->volume should not be NULL.",
                             0);
        error=v_floats_to_file(fp,str->volume,
                              str->volume_valid,
                              num_of_structures,&grplen,&acr_items[i++],
                              error1) ;
        if (error != 0) return(error) ;


                        /**002B 8160****/
        if (str->surface_area == NULL &&
            str->surface_area_valid ==1)
          v_print_fatal_error("VWriteHeader",
                             "str->surface_area should not be NULL.",
                             0);
        error=v_floats_to_file(fp,str->surface_area,
                              str->surface_area_valid,
                              num_of_structures,&grplen,&acr_items[i++],
                              error1) ;
        if (error != 0) return(error) ;


                        /**002B 8170****/
        if (str->rate_of_change_volume == NULL &&
            str->rate_of_change_volume_valid ==1)
          v_print_fatal_error("VWriteHeader",
                             "str->rate_of_change_volume should not be NULL.",
                             0);
        error=v_floats_to_file(fp,str->rate_of_change_volume,
                              str->rate_of_change_volume_valid,
                              num_of_structures,&grplen,&acr_items[i++],
                              error1) ;
        if (error != 0) return(error) ;

                        /**002B 8180****/
        error=v_string_to_file(fp,str->description,str->description_valid,
                             &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;

                        /*end structure data*/

        *offset=skip ;
        *len=grplen ;
        *items=i ;
        return(0) ;
}

/***********************************************************************/
static int v_count_samples_memory_space ( int* total, short* samples, short dim )
{
    int grand_total, level_total, last_level_total,
        i, n, x;

    if (total == NULL || samples == NULL || dim < 3)    return 0;

    grand_total = last_level_total = samples[0];
    x = 1;
    for (n = 0; n < dim-3; n++)  {
        level_total = 0;
        for (i = 0; i < last_level_total; i++)  level_total += samples[x++];
        last_level_total = level_total;
        grand_total     += level_total;
    }
    *total = x;
    return x;
#if     0       /* old code that doesn't work   */
                /* changed by gjg               */
        int i, j, k ;
        int total1, total2, total3 ;

        if (samples == NULL || dim < 3) {
            *total=0 ;
            return ;
        }
        for (i=j=0, total1=1 ; i<dim-3; i++) {
            if (j == 0) {
                total2=total1 ;
                total1 += samples[j++] ;
            }
            else {
                total3=total1 ;
                for (k=0; k<total1-total2; k++) total1 += samples[j++] ;
                total2=total3 ;
            }
        }
        *total=total1 ;
#endif
}
/************************************************************************
 *                                                                      *
 *      Function        : v_write_display_10                            *
 *      Description     : This function writes the display information  *
 *                        contained by the DisplayInfo structure to     *
 *                        header of the file pointed by fp.             *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         dsp - pointer to display information data in *
 *                               the file header.                       *
 *                         offset - Returns the bytes to be skipped     *
 *                               from the beginning of file for the last*
 *                               group in this function.                *
 *                         len - Returns the length of the last group   *
 *                               in this function.                      *
 *                         items - the number of items in the group.    *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_write_display_10 ( FILE* fp, DisplayInfo* dsp, long* offset, int* len,
    int* items, int* error1 )
{ 
        int grplen, error, i ;
        long skip ;
        static short dsp_grp[6]={0x002D,0,0,4,0,0} ;

        skip=(*offset) ;
        grplen=(*len) ;
        i= *items ;
        error=v_write_len(fp,&skip,&grplen,dsp_grp) ;
        if (error != 0) return(error) ;
        error=v_short_to_file(fp,dsp->dimension,dsp->dimension_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;

/***********New changes as requested by Dewey on 10/7/92*********/
/*Changed because meausement_unit is now a fixed array instead*/
/*of a pointer. Also dependencies on dimension is eliminated now.*/
/*These changes were made by Krishna Iyer on 10/7/92********/
        error=v_shorts_to_file(fp,dsp->measurement_unit,
                                 dsp->measurement_unit_valid,
                                 2,&grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
/**************end changes***************************/

        error=v_short_to_file(fp,dsp->num_of_elems,dsp->num_of_elems_valid,
                            &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        if (dsp->num_of_elems_valid == 1) {
            if (dsp->smallest_value == NULL && dsp->smallest_value_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "dsp->smallest_value should not be NULL.",
                    0) ;
            error=v_floats_to_file(fp,dsp->smallest_value,
                                 dsp->smallest_value_valid,dsp->num_of_elems,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (dsp->smallest_value_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "dsp->num_of_elems_valid should be 1.",
                    dsp->num_of_elems_valid) ;
            else {
                error=v_floats_to_file(fp,
                                     dsp->smallest_value,
                                     dsp->smallest_value_valid,
                                     dsp->num_of_elems,
                                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
        if (dsp->num_of_elems_valid == 1) {
            if (dsp->largest_value == NULL && dsp->largest_value_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "dsp->largest_value should not be NULL.",
                    0) ;
            error=v_floats_to_file(fp,dsp->largest_value,
                                 dsp->largest_value_valid,dsp->num_of_elems,
                                 &grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (dsp->largest_value_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "dsp->num_of_elems_valid should be 1.",
                    dsp->num_of_elems_valid) ;
            else {
                error=v_floats_to_file(fp,dsp->largest_value,
                     dsp->largest_value_valid,dsp->num_of_elems,
                     &grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
        error=v_short_to_file(fp,dsp->num_of_integers,dsp->num_of_integers_valid,
                            &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        if (dsp->num_of_integers_valid == 1) {
            if (dsp->signed_bits == NULL && dsp->signed_bits_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "dsp->signed_bits should not be NULL.",0) ;
            error=v_shorts_to_file(fp,dsp->signed_bits,dsp->signed_bits_valid,
                                 dsp->num_of_integers,&grplen,&acr_items[i++],
                                 error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (dsp->signed_bits_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "dsp->num_of_integers_valid should be 1.",
                    dsp->num_of_integers_valid) ;
            else {
                error=v_shorts_to_file(fp,dsp->signed_bits,
                                     dsp->signed_bits_valid,
                                     0,&grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
        error=v_short_to_file(fp,dsp->num_of_bits,dsp->num_of_bits_valid,
                            &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        if (dsp->num_of_elems_valid == 1) {
            if (dsp->bit_fields == NULL && dsp->bit_fields_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "dsp->bit_fields should not be NULL.",0) ;
            error=v_shorts_to_file(fp,dsp->bit_fields,dsp->bit_fields_valid,
                                 2*dsp->num_of_elems,&grplen,&acr_items[i++],
                                 error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (dsp->bit_fields_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "dsp->num_of_elems_valid should be 1.",
                    dsp->num_of_elems_valid) ;
            else {
                error=v_shorts_to_file(fp,dsp->bit_fields,dsp->bit_fields_valid,
                                     0,&grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
        error=v_short_to_file(fp,dsp->dimension_in_alignment,
                            dsp->dimension_in_alignment_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_short_to_file(fp,dsp->bytes_in_alignment,
                            dsp->bytes_in_alignment_valid,&grplen,
                            &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_short_to_file(fp,dsp->num_of_images,dsp->num_of_images_valid,
                            &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_shorts_to_file(fp,dsp->xysize,dsp->xysize_valid,2,&grplen,
                             &acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        error=v_floats_to_file(fp,dsp->xypixsz,dsp->xypixsz_valid,2,&grplen,
                             &acr_items[i++],error1);
        if (error != 0) return(error) ;
        error=v_string_to_file(fp,dsp->specification_pv,
                             dsp->specification_pv_valid,&grplen,
                             &acr_items[i++],error1);
        if (error != 0) return(error) ;
        if (dsp->num_of_images_valid == 1) {
            if (dsp->pv == NULL && dsp->pv_valid == 1)
                v_print_fatal_error("VWriteHeader",
                    "dsp->pv should not be NULL.",0) ;
            error=v_shorts_to_file(fp,dsp->pv,dsp->pv_valid,
                        dsp->num_of_images,&grplen,&acr_items[i++],error1) ;
            if (error != 0) return(error) ;
        }
        else {
            if (dsp->pv_valid == 1) 
                v_print_fatal_error("VWriteHeader",
                    "dsp->num_of_images_valid should be 1.",
                    dsp->num_of_images_valid) ;
            else {
                error=v_shorts_to_file(fp,dsp->pv,dsp->pv_valid,
                                     0,&grplen,&acr_items[i++],error1) ;
                if (error != 0) return(error) ;
            }
        }
        error=v_string_to_file(fp,dsp->description,dsp->description_valid,
                             &grplen,&acr_items[i++],error1) ;
        if (error != 0) return(error) ;
        *offset=skip ;
        *len=grplen ;
        *items=i ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_cvt_delimiter                               *
 *      Description     : This function converts string2 to string1 with*
 *                        specific delimiter.                           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  string1 - the converted string.              *
 *                         string2 - the string to be converted.        *
 *                         del - the delimiter to be used.              *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                                                                      *
 ************************************************************************/
static int v_cvt_delimiter ( char* string1, char* string2, char del )
{
        int i ;

        memset(string1,0,20) ;
        for (i=0; string2[i] != '\0'; i++) {
            if (string2[i] >= '0' && string2[i] <= '9') string1[i]=string2[i] ;
            else {
                if (string2[i] == ' ') string1[i]='0' ;
                else string1[i]=del ;
            }
        }
        string1[i]='\0' ;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_string_to_file                              *
 *      Description     : If the string is NULL, then the length of this*
 *                        string is zero, else count the string in      *
 *                        bytes. If the number is odd, then pad blank to*
 *                        be even bytes. Output group number and element*
 *                        number from specification file, string length,*
 *                        and string value if the string is not null.   *
 *                        Add this item length to the group length,     *
 *                        and return the the current group length to    *
 *                        *len.                                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         string - string to be converted to file.     *
 *                         valid - valid falg of the item.              *
 *                         len - the length of the item.                *
 *                         item - the item under considerations.        *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_string_to_file ( FILE* fp, char* string, unsigned int valid, int* len,
    ItemInfo* item, int* error1 )
//unsigned valid ;
{
        char *string1 ;
        int bytes, error ;

        if (valid == 0) v_assign_data_error_code(error1,item) ;
        error=v_WriteData((unsigned char*)&(item->group),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)&(item->elem),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        if (valid == 0 || string == NULL) {
            bytes=0;
            error=v_WriteData((unsigned char*)&bytes,sizeof(int),1,fp) ;
            if (error == 0) return(3) ;
            *len += 8 ;
            return(0) ;
        }
        if (strcmp(string,"") == 0) {
            bytes=0;
            error=v_WriteData((unsigned char*)&bytes,sizeof(int),1,fp) ;
            if (error == 0) return(3) ;
            *len += 8 ;
            return(0) ;
        }
        bytes=(int)strlen(string) ;
        string1=(char *)calloc(bytes+2,1);
        if (string1==NULL) return(1);
        strcpy(string1,string) ;
        if ((bytes%2) != 0) {
            string1[bytes]=' ' ;
            string1[bytes+1]='\0' ;
            bytes++;
        }
        
        error=v_WriteData((unsigned char*)&bytes,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)string1,1,bytes,fp) ;
        free(string1);
        if (error == 0) return(3) ;
        *len = *len + bytes + 8 ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_short_to_file                               *
 *      Description     : If the num is invalid number, then the        *
 *                        length of this item value is zero, else the   *
 *                        length of num is 2 bytes. Output group and    *
 *                        element numbers from specification file, the  *
 *                        length of num, and the num value if the num is*
 *                        not INVALID. Add this item length to the group*
 *                        length, and return the the current group      *
 *                        length to *len.                               *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         num - the number of items.                   *
 *                         string - string to be converted to file.     *
 *                         valid - valid falg of the item.              *
 *                         len - the length of the item.                *
 *                         item - the item under considerations.        *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_short_to_file ( FILE* fp, short num, unsigned int valid, int* len,
    ItemInfo* item, int* error1 )
//unsigned valid ;
{
        int i, error ;

        if (valid == 0) v_assign_data_error_code(error1,item) ;
        error=v_WriteData((unsigned char*)&(item->group),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)&(item->elem),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        if (valid == 0) {
            i=0 ;
            error=v_WriteData((unsigned char*)&i,sizeof(int),1,fp) ;
            if (error == 0) return(3) ;
            *len += 8 ;
        }
        else {
            i=2 ;
            error=v_WriteData((unsigned char*)&i,sizeof(int),1,fp) ;
            if (error == 0) return(3) ;
            error=v_WriteData((unsigned char*)&num,sizeof(short),1,fp) ;
            if (error == 0) return(3) ;
            *len += 10 ;
        }
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_float_to_file                               *
 *      Description     : If the num is invalid number, then the        *
 *                        length of this item value is zero, else       *
 *                        convert the real number to ASCII character    *
 *                        string and count the string length in bytes.  *
 *                        If the length is odd, then pad blank to be    *
 *                        even bytes. Output group and element numbers  *
 *                        from specification file, the length of string,*
 *                        and the string value if the num is not        *
 *                        INVALID. Add this item length to the group    *
 *                        length, and return the the current group      *
 *                        length to *len.                               *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         num - Specifies an integer to be output.     *
 *                         valid - valid falg of the item.              *
 *                         len - the length of the group before this    *
 *                               item.                                  *
 *                         item - the item under consideration.         *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_float_to_file ( FILE* fp, float num, unsigned int valid, int* len,
    ItemInfo* item, int* error1 )
//unsigned valid ;
{
        int bytes, error ;
        char string[100], string1[100] ;

        if (valid == 0) v_assign_data_error_code(error1,item) ;
        error=v_WriteData((unsigned char*)&(item->group),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)&(item->elem),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        if (valid == 0) {
            bytes=0 ;
            error=v_WriteData((unsigned char*)&bytes,sizeof(int),1,fp) ;
            if (error == 0) return(3) ;
            *len += 8 ;
            return(0) ;
        }
        memset(string,0,100) ;
        sprintf(string,"%e", (double)num) ;
        bytes=(int)strlen(string) ;
        memset(string1,0,100) ;
        strcpy(string1,string) ;
        if ((bytes%2) != 0) {
            string1[bytes]=' ' ;
            string1[bytes+1]='\0' ;
        }
        bytes=(int)strlen(string1) ;
        error=v_WriteData((unsigned char*)&bytes,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)string1,1,bytes,fp) ;
        if (error == 0) return(3) ;
        *len = (*len)+8+bytes ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_ints_to_file                                *
 *      Description     : If the array num contains nothing, or the     *
 *                        number item is zero, then the length of this  *
 *                        item value is zero, else the length of array  *
 *                        num is item*2 bytes. Output group and element *
 *                        numbers from specification file, the length of*
 *                        num, and the num value if the pointer of array*
 *                        num is not NULL, or the item is not zero.     *
 *                        Add this item length to the group length, and *
 *                        return the the current group length to *len.  *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         num - Specifies an integer to be output.     *
 *                         valid - valid falg of the item.              *
 *                         len - the length of the group before this    *
 *                               item.                                  *
 *                         spc_item - the item under consideration.     *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on January 12, 1993, by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
static int v_ints_to_file ( FILE* fp, int* num, unsigned int valid, int item,
    int* len, ItemInfo* spc_item, int* error1 )
//unsigned valid ;
{
        int i, error ;
 
        if (valid == 0) v_assign_data_error_code(error1,spc_item) ;
        error=v_WriteData((unsigned char*)&(spc_item->group),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)&(spc_item->elem),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        if (num == NULL || item < 0 || valid == 0) i=0 ;
        else i=item*sizeof(int);
        /*printf("item %d len %d\n",item,i);*/
        error=v_WriteData((unsigned char*)&i,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        *len += 8 ;
        if (num != NULL && item > 0 && valid > 0) {
            error=v_WriteData((unsigned char*)num,sizeof(int),item,fp) ;
            if (error == 0) return(3) ;
            *len = (*len) + item*sizeof(int);
        }
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_floats_to_file                              *
 *      Description     : If nothing is in the array num or the number  *
 *                        of items in the array num is zero, then the   *
 *                        length of this item value is zero, else       *
 *                        convert the real numbers to ASCII character   *
 *                        string with delimiter "/" and count the string*
 *                        length in bytes. If the length is odd, then   *
 *                        pad blank to be even bytes. Output group and  *
 *                        element numbers from specification file, the  *
 *                        length of string, and the string value if the *
 *                        array num is not NULL or items is not zero.   *
 *                        Add this item length to the group length, and *
 *                        return the the current group length to *len.  *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         num - Specifies an integer to be output.     *
 *                         valid - valid falg of the item.              *
 *                         len - the length of the group before this    *
 *                               item.                                  *
 *                         spc_item - the item under consideration.     *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                        Modified 3/20/95 float passed to sprintf
 *                        cast to double by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_floats_to_file ( FILE* fp, float* num, unsigned int valid, int item,
    int* len, ItemInfo* spc_item, int* error1 )
//unsigned valid ;
{
        int bytes, i, total, total1, error ;
        long skip ;
        char string[100], string1[100] ;

        if (valid == 0) v_assign_data_error_code(error1,spc_item) ;
        error=v_WriteData((unsigned char*)&(spc_item->group),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)&(spc_item->elem),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        total=0 ;
        error=v_WriteData((unsigned char*)&total,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        *len += 8 ;
        if (num==NULL || item <= 0 || valid == 0) return(0) ;
        for (i=0; i<item-1; i++) {
            memset(string,0,100) ;
            sprintf(string,"%e\\", (double)num[i]) ;
            bytes=(int)strlen(string) ;
            error=v_WriteData((unsigned char*)string,1,bytes,fp) ;
            if (error == 0) return(3) ;
            total += bytes ;
        }
        memset(string,0,100) ;
        sprintf(string,"%e", (double)num[i]) ;
        bytes=(int)strlen(string) ;
        total1 = total + bytes ;
        memset(string1,0,100) ;
        strcpy(string1,string) ;
        if ((total1%2) != 0) {
            string1[bytes]=' ' ;
            string1[bytes+1]='\0' ;
        }
        bytes=(int)strlen(string1) ;
        total += bytes ;
        error=v_WriteData((unsigned char*)string1,1,bytes,fp) ;
        if (error == 0) return(3) ;
        skip=(-total)-4 ;
        error=fseek(fp,skip,1) ;
        if (error == -1) return(5) ;
        error=v_WriteData((unsigned char*)&total,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=fseek(fp,total,1) ;
        if (error == -1) return(5) ;
        *len += total ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_shorts_to_file                              *
 *      Description     : If the array num contains nothing, or the     *
 *                        number item is zero, then the length of this  *
 *                        item value is zero, else the length of array  *
 *                        num is item*2 bytes. Output group and element *
 *                        numbers from specification file, the length   *
 *                        of num, and the num value if the pointer of   *
 *                        array num is not NULL, or the item is not     *
 *                        zero. Add this item length to the group       *
 *                        length, the the current group length to *len. *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         num - Specifies an integer to be output.     *
 *                         valid - valid falg of the item.              *
 *                         len - the length of the group before this    *
 *                               item.                                  *
 *                         spc_item - the item under consideration.     *
 *                         error1 - the error code that is returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_shorts_to_file ( FILE* fp, short* num, unsigned int valid, int item,
    int* len, ItemInfo* spc_item, int* error1 )
//unsigned valid ;
{
        int i, error ;

        if (valid == 0) v_assign_data_error_code(error1,spc_item) ;
        error=v_WriteData((unsigned char*)&(spc_item->group),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData((unsigned char*)&(spc_item->elem),sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        if (num == NULL || item < 0 || valid == 0) i=0 ;
        else i=item*2 ;
        error=v_WriteData((unsigned char*)&i,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        *len += 8 ;
        if (num != NULL && item > 0 && valid > 0) {
            error=v_WriteData((unsigned char*)num,sizeof(short),item,fp) ;
            if (error == 0) return(3) ;
            *len = (*len) + item*2 ;
        }
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_write_len                                   *
 *      Description     : This function writes the current group length *
 *                        of value field, and the next group, element,  *
 *                        length, and value fields to the output file.  *
 *                        Return the next value field position for      *
 *                        writing group length and initialize group     *
 *                        length for the next group.                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *                         5 - improper seeks.                          *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         skip - Specifies the bytes skipped from      *
 *                              the beginning of the file for writing   *
 *                              the current group length to the output  *
 *                              file.                                   *
 *                         grplen - Specifies the current group length  *
 *                              to be written to the output file.       *
 *                         grp - the next group numbers and element     *
 *                              numbers to be written to the output     *
 *                              file.                                   *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_write_len ( FILE* fp, long* skip, int* grplen, short* grp )
{
        int error ;

        error=fseek(fp,*skip,0) ;
        if (error == -1) return(5) ;
        error=v_WriteData((unsigned char*)grplen,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        *skip = (*skip)+4+(*grplen) ;
        error=fseek(fp,*skip,0) ;
        if (error == -1) return(5) ;
        error=v_WriteData((unsigned char*)grp,sizeof(short),6,fp) ;
        if (error == 0) return(3) ;
        *skip += 8 ; /* skipping the group, elemet, and length fields for 
                        writing the next group length */
        *grplen=0 ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_close_header                                *
 *      Description     : Output the current group length and the group *
 *                        element, length, and value of the image data  *
 *                        group or the non-image data group according   *
 *                        to the data type. This function will complete *
 *                        header part and return the header length.     *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *                         5 - improper seeks.                          *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         offset - Specifies the bytes skipped from the*
 *                              beginning of the file for writing the   *
 *                              current group length to the output file.*
 *                         grplen - Specifies the current group length  *
 *                              to be written to the output file.       *
 *                         type - Specifies the data type of output     *
 *                              file.                                   *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                                                                      *
 ************************************************************************/
static int v_close_header ( FILE* fp, long offset, int grplen, int type )
{
        static short image[10]={0x7FE0,0,0,4,0,0,0x7FE0,0x0010,0,0} ;
        static short structure[10]={0x8001,0,0,4,0,0,0x8001,0x8000,0,0} ;
        static short display[10]={0x8021,0,0,4,0,0,0x8021,0x8000,0,0} ;
        long skip ;
        int error ;

        error=fseek(fp,offset,0) ;
        if (error == -1) return(5) ;
        error=v_WriteData((unsigned char*)&grplen,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        skip=offset+4+grplen ;
        error=fseek(fp,skip,0) ;
        if (error == -1) return(5) ;
        switch (type) {
            case IMAGE0 :
            case IMAGE1 :
                error=v_WriteData((unsigned char*)image,sizeof(short),10,fp) ;
                if (error == 0) return(3) ;
                break ;
            case CURVE0 :
            case SURFACE0 :
            case SURFACE1 :
            case SHELL1: /* structure data of type SHELL1 */
            case SHELL0 :
            case SHELL2: /* structure data of type SHELL2 */
                error=v_WriteData((unsigned char*)structure,sizeof(short),10,fp) ;
                if (error == 0) return(3) ;
                break ;
           case MOVIE0 :
                error=v_WriteData((unsigned char*)display,sizeof(short),10,fp) ;
                if (error == 0) return(3) ;
                break ;
            default :
                return(102) ;   
                break ;
        }
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VWriteData                                    *
 *      Description     : This function will write a string of bytes    *
 *                        into an array from the specified file pointer.*
 *                        This function assumes that fp points to the   *
 *                        beginning of the data part, and that the data *
 *                        in the file follows the big endian format     *
 *                        for short (size 2), and integer (size 4)      *
 *                        values. This function would covert from the   *
 *                        format the machine supports to the big endian *
 *                        format and write out the data. It will return *
 *                        the items written in the item_written field.  *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  fp - a pointer to the input data file.       *
 *                         size - size of each data item. Valid items   *
 *                              are 1, 2 & 4                            *
 *                         items- the number of items to be written.    *
 *                         data - the array where the function puts the *
 *                              string.                                 *
 *                         items_written-Returns the number of items    *
 *                              this function actually read.            *
 *      Side effects    : This function can write only 1,2 or 4 byte    *
 *                        integer data. This should be expanded to      *
 *                        handle floating point data.                   *
 *      Entry condition : If the fp, data, or items_written is NULL,    *
 *                        or items is zero, or size is not 1, 2 or 4    *
 *                        this function will print the proper message to*
 *                        the standard error stream, producce the core  *
 *                        dump, and exit from the current process.      *
 *      Related funcs   : VGetHeaderLength, VCloseData, VReadData,      *
 *                        VSeekData.                                    *
 *      History         : Written on March 22, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on April 12, 1993 by S. Samarasekera.*
 *                        Modified 6/12/95 3 returned on write error
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VWriteData ( char* data, int size, int items, FILE* fp, int* items_written )
{


  if (fp == NULL) 
    v_print_fatal_error("VWriteData",
                       "The specified file pointer should not be NULL.",0) ;
  if (data == NULL) 
    v_print_fatal_error("VWriteData",
                       "The data pointer should not be NULL.",0) ;
  if (items_written == NULL) 
    v_print_fatal_error("VWriteData",
                       "The items_written pointer should not be NULL.",0) ;
  if ( items <= 0) 
    v_print_fatal_error("VWriteData",
                       "The value of items is greater than 0.",0) ; 
  if ( size !=1 && size !=2 && size !=4) 
    v_print_fatal_error("VWriteData",
                       "The value of size should be 1, 2 or 4.",0) ; 
  
  *items_written = v_WriteData((unsigned char*)data,size,items,fp);
  if (*items_written!=items) return(3);

  return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VSeekData                                     *
 *      Description     : This function would skip the file header      *
 *                        and offset bytes from the data segment and    *
 *                        make the file pointer point to the correct    *
 *                        location in the data.                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error .                             *
 *                         5 - improper seeks.                          *
 *                         100 - incorrect 3DVIEWNIX file header format.*
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         offset - number of data bytes to skip.       *
 *      Side effects    : Cannot seek nore than the size of a long.     *
 *                        This is a unix limitation.                    *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteHeader.                                 *
 *      History         : Written on April 12, 1993 by S. Samarasekera. *
 *                                                                      *
 ************************************************************************/
int VSeekData ( FILE* fp, off_t offset )
{

  int hdrlen,error;

  if (fp == NULL) 
    v_print_fatal_error("VReadData",
                       "The specified file pointer should not be NULL.",0) ;

  error=VGetHeaderLength(fp,&hdrlen) ;
  if (error != 0) return(error) ;
  error=fseek(fp,hdrlen,0);
  if (error==-1) return(5);
  error=fseek(fp,offset,1);
  if (error==-1) return(5);
  return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VLSeek                                        *
 *      Description     : This function would                           *
 *                        make the file pointer point to the offset     *
 *                        location in the file.                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         5 - improper seeks.                          *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         offset - number of data bytes to skip.       *
 *      Side effects    : 
 *      Entry condition : None
 *      Related funcs   : VLSeekData
 *      History         : Written 6/30/15 by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VLSeek ( FILE* fp, double offset )
{

  int error;

  if (fp == NULL) 
    v_print_fatal_error("VLSeek",
                       "The specified file pointer should not be NULL.",0) ;

  error = fseek(fp, 0, 0);
  if (error == -1)
    return (5);
  while (offset > 0x40000000)
  {
	error = fseek(fp, 0x40000000, 1);
	if (error == -1)
    return (5);
    offset -= 0x40000000;
  }
  error = fseek(fp, (long)(offset+.5), 1);
  if (error == -1)
	return (5);
  return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VLSeekData                                     *
 *      Description     : This function would skip the file header      *
 *                        and offset bytes from the data segment and    *
 *                        make the file pointer point to the correct    *
 *                        location in the data.                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error .                             *
 *                         5 - improper seeks.                          *
 *                         100 - incorrect 3DVIEWNIX file header format.*
 *      Parameters      :  fp - a pointer to the output data file.      *
 *                         offset - number of data bytes to skip.       *
 *      Side effects    : 
 *      Entry condition : The environment variable VIEWNIX_ENV must be
 *                          properly set.
 *      Related funcs   : VSeekData
 *      History         : Written 10/12/10 by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VLSeekData ( FILE* fp, double offset )
{

  int error;

  if (fp == NULL) 
    v_print_fatal_error("VLSeekData",
                       "The specified file pointer should not be NULL.",0) ;

  error = VSeekData(fp, 0);
  if (error != 0) return(error) ;
  while (offset > 0x40000000)
  {
	error = fseek(fp, 0x40000000, 1);
	if (error == -1)
    return (5);
    offset -= 0x40000000;
  }
  error = fseek(fp, (long)(offset+.5), 1);
  if (error == -1)
	return (5);
  return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VCloseData                                    *
 *      Description     : This function will write the group length     *
 *                        for the data group (SCENE, STRUCTURE, or      *
 *                        DISPLAY), message length for the COMMAND      *
 *                        group and IDENTIFICATION groups, and close    *
 *                        this data file. When you finish writing data  *
 *                        to the disk file according to the 3DVIEWNIX   *
 *                        protocol and want to close the data file, you *
 *                        should call this function.                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         5 - improper seeks.                          *
 *                         100 - incorrect  3dviewnix file header       *
 *                               format.                                *
 *      Parameters      :  fp - a pointer to the output data file.      *
 *      Side effects    : None.                                         *
 *      Entry condition : If the fp is NULL, this function will print   *
 *                        the proper message to the standard error      *
 *                        stream, produce the core dump, and exit from  *
 *                        the current process.                          *
 *      Related funcs   : VWriteHeader, VWriteData.                     * 
 *      History         : Written on April 12, 1993 by S. Samarasekera. *
 *                                                                      *
 ************************************************************************/
int VCloseData ( FILE* fp )
{
  long length, grplen;
  long skip,start,end;
  int error;
  

  if (fp==NULL) 
    v_print_fatal_error("VCloseData",
                       "The specified file pointer should not be NULL",0);

  error=fseek(fp,0L,2L);
  if (error==-1) return(5);
  end=ftell(fp);

  error=VSeekData(fp,0);
  if (error) return(error);
  start=ftell(fp);

  length=end-start;

  error=fseek(fp,-12,1);
  if (error==-1) return(-1);

  grplen=length+8;
  error=v_WriteData((unsigned char*)&grplen,sizeof(int),1,fp);
  if (error!=1) return(3);

  /* Write the message length of the command group */
  length=end-24;
  error=fseek(fp,20,0);
  if (error==-1) return(5);
  error=v_WriteData((unsigned char*)&length,sizeof(int),1,fp);
  if (error!=1) return(3);
  
  /* Write message length for the identification group */
  error=fseek(fp,8,0);
  if (error==-1) return(5);
  error=v_ReadData((char*)&grplen,sizeof(int),1,fp);
  if (error!=1) return(2);
  grplen +=12;
  length=end-24-grplen;
  skip = grplen+20;
  error=fseek(fp,skip,0);
  if (error==-1) return(5);
  error=v_WriteData((unsigned char*)&length,sizeof(int),1,fp);
  if (error!=1) return(3);
  
  fclose(fp) ;
  return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_ReadData                                    *
 *      Description     : This functions is similar to fread. It        *
 *                        assumes the input file follows the big endian *
 *                        format and converts the data from this format *
 *                        to the convention the machine follows. This   *
 *                        program assumes the data to be either char,   *
 *                        short or int. It returns the number of items  *
 *                        that were read successfully. The file pointer *
 *                        should be pointing to the correct location in *
 *                        the input file that the data should be        *
 *                        read from.                                    *
 *      Return Value    : number of items read.                         *
 *      Parameters      :  data - a pointer to the data segment.        *
 *                         size - size of an item being read.           *
 *                         items - number of data items to read.        *
 *                         fp - pointer to the input file               *
 *      Side effects    : None.                                         *
 *      Related funcs   : None                                          *
 *      History         : Written on April 12, 1993 by S. Samarasekera. *
 *                                                                      *
 ************************************************************************/
int v_ReadData ( char* data, int size, int items, FILE* fp )
{

  static unsigned short *short_test=(unsigned short *)lib_short_test.c; 
  static unsigned int *int_test=(unsigned int *)lib_int_test.c; 
  int i,items_read;
  unsigned char tmp_char;

  items_read=(int)fread(data,size,items,fp);
  if (items_read!=items) return(items_read);

  switch (size) {
  case 1:  /* char - no conversion to be made */
    break;
  case 2:  /* short - two possible coversions */
    if ( *short_test == 0x0100 ) /* bytes_switched */
      for(i=0;i<items;i++)  {
        tmp_char=data[0]; data[0]=data[1]; data[1]=tmp_char; 
        data +=2;
      }
    break;
  case 4: /* int - 4 possible conversions */
    switch (*int_test) {
    case 0x00010203:
      break;
    case 0x03020100:
      for(i=0;i<items;i++)  {
        tmp_char=data[0]; data[0]=data[3]; data[3]=tmp_char; 
        tmp_char=data[1]; data[1]=data[2]; data[2]=tmp_char; 
        data += 4;
      }      
      break;
    case 0x02030001:
      for(i=0;i<items;i++)  {
        tmp_char=data[0]; data[0]=data[2]; data[2]=tmp_char; 
        tmp_char=data[1]; data[1]=data[3]; data[3]=tmp_char; 
        data += 4;
      }      
      break;
    case 0x01000302:
      for(i=0;i<2*items;i++)  {
        tmp_char=data[0]; data[0]=data[1]; data[1]=tmp_char; 
        data += 2;
      }      
      break;
    }
    break;
  }
   
  return(items_read);

}

/************************************************************************
 *                                                                      *
 *      Function        : v_WriteData                                   *
 *      Description     : This functions is similar to fwrite. It       *
 *                        assumes the output file follows the big endian*
 *                        format and converts the data to this format   *
 *                        from the convention the machine follow. This  *
 *                        program assumes the data to be either char,   *
 *                        short or int. It returns the number of items  *
 *                        that were read successfully. The file pointer *
 *                        should be pointing to the correct location in *
 *                        the output file that the data should be       *
 *                        written to.                                   *
 *      Return Value    : number of items written.                      * 
 *      Parameters      :  data - a pointer to the data segment.        *
 *                         size - size of an item being written.        *
 *                         items - number of data items to write.       *
 *                         fp - pointer to the output file              *
 *      Side effects    : None.                                         *
 *      Related funcs   : None                                          *
 *      History         : Written on April 12, 1993 by S. Samarasekera. *
 *                                                                      *
 ************************************************************************/
int v_WriteData ( unsigned char* data, int size, int items, FILE* fp )
{
#define BLOCK_SIZE 1000
#define INT_BLOCK BLOCK_SIZE/4
#define SHORT_BLOCK BLOCK_SIZE/2
  static unsigned short *short_test=(unsigned short *)lib_short_test.c; 
  static unsigned int *int_test=(unsigned int *)lib_int_test.c; 
  int i,items_written,cur_written,left_over;
  unsigned char block[BLOCK_SIZE],*ptr;

  switch (size) {
  case 1:  /* char - no conversion to be made */
    return((int)fwrite(data,size,items,fp));
  case 2:  /* short - two possible coversions */
    if ( *short_test == 0x0100 ) { /* bytes_switched */
      items_written=0;
      while (items_written+SHORT_BLOCK<=items) {
        for(ptr=block,i=0;i<SHORT_BLOCK;i++)  {
          ptr[1]=data[0]; ptr[0]=data[1];
          data +=size;
          ptr  +=size;
        }
        cur_written=(int)fwrite(block,size,SHORT_BLOCK,fp);
        items_written+=cur_written;
        if (cur_written!=SHORT_BLOCK) return(items_written);
      }
      left_over=items-items_written;
      for(ptr=block,i=0;i<left_over;i++)  {
        ptr[1]=data[0]; ptr[0]=data[1];
        data +=size;
        ptr  +=size;
      }
      return(items_written + (int)fwrite(block,size,left_over,fp));
    }
    else 
      return((int)fwrite(data,size,items,fp));
    break;
  case 4: /* int - 4 possible convertons */
    switch (*int_test) {
    case 0x00010203:
      return((int)fwrite(data,size,items,fp));
      break;
    case 0x03020100:
      items_written=0;
      while (items_written+INT_BLOCK<=items) {
        for(ptr=block,i=0;i<INT_BLOCK;i++)  {
          ptr[0]=data[3]; ptr[1]=data[2]; 
          ptr[2]=data[1]; ptr[3]=data[0];
          data +=size;
          ptr  +=size;
        }
        cur_written=(int)fwrite(block,size,INT_BLOCK,fp);
        items_written+=cur_written;
        if (cur_written!=INT_BLOCK) return(items_written);
      }
      left_over=items-items_written;
      for(ptr=block,i=0;i<left_over;i++)  {
          ptr[0]=data[3]; ptr[1]=data[2]; 
          ptr[2]=data[1]; ptr[3]=data[0];
          data +=size;
          ptr  +=size;
      }
      return(items_written + (int)fwrite(block,size,left_over,fp));
    case 0x02030001:
      items_written=0;
      while (items_written+INT_BLOCK<=items) {
        for(ptr=block,i=0;i<INT_BLOCK;i++)  {
          ptr[0]=data[2]; ptr[1]=data[3]; 
          ptr[2]=data[0]; ptr[3]=data[1];
          data +=size;
          ptr  +=size;
        }
        cur_written=(int)fwrite(block,size,INT_BLOCK,fp);
        items_written+=cur_written;
        if (cur_written!=INT_BLOCK) return(items_written);
      }
      left_over=items-items_written;
      for(ptr=block,i=0;i<left_over;i++)  {
          ptr[0]=data[2]; ptr[1]=data[3]; 
          ptr[2]=data[0]; ptr[3]=data[1];
          data +=size;
          ptr  +=size;
      }
      return(items_written + (int)fwrite(block,size,left_over,fp));
    case 0x01000302:
      items_written=0;
      while (items_written+INT_BLOCK<=items) {
        for(ptr=block,i=0;i<INT_BLOCK;i++)  {
          ptr[0]=data[1]; ptr[1]=data[0]; 
          ptr[2]=data[3]; ptr[3]=data[2];
          data +=size;
          ptr  +=size;
        }
        cur_written=(int)fwrite(block,size,INT_BLOCK,fp);
        items_written+=cur_written;
        if (cur_written!=INT_BLOCK) return(items_written);
      }
      left_over=items-items_written;
      for(ptr=block,i=0;i<left_over;i++)  {
          ptr[0]=data[1]; ptr[1]=data[0]; 
          ptr[2]=data[3]; ptr[3]=data[2];
          data +=size;
          ptr  +=size;
      }
      return(items_written + (int)fwrite(block,size,left_over,fp));
    }
    break;
  }

  return 0;  //gjg?
}

/*****************************************************************/
