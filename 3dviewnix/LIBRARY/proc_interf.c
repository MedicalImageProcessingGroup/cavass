/*
  Copyright 1993-2013, 2017 Medical Image Processing Group
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
 *                                                                      *
 *      Filename  : proc_interface.c                                    *
 *      Ext Funcs : VReadGlobalcomFile, VWriteGlobalcomFile,            *
 *                  VAddBackgroundProcessInformation,                   *
 *                  VDeleteBackgroundProcessInformation, VCancelEvents, *
 *                  VSelectEvents, VWriteError, VGetInputFiles,         *
 *                  VPrintFatalError, VDecodeError, VComputeLine,       *
 *                  VCopyString, VCreateOutputFilename, VAllocateMemory,*
 *                  VReadTapePathName, VWriteTapePathName,              *
 *                  VCallProcess.                                       *
 *      Int Funcs : v_read_wins_id_com,v_write_wins_id_com,             *
 *                  v_read_color_com,v_list_bg_process,                 *
 *                  v_WriteErrorToStdio,v_print_fatal_error,            *
 *                  v_check_button_occurance.                           *
 *                                                                      *
 ************************************************************************/

#include "Vlibrary.h"
#include <stdlib.h>
#include <sys/types.h>
#if ! defined (WIN32) && ! defined (_WIN32)
    #include <unistd.h>
#endif
#include "3dv.h"


#define MIN_COMMON_ERROR_CODE 0 
#define MAX_COMMON_ERROR_CODE 8
#define MIN_DATA_INTERFACE_ERROR_CODE 100
#define MAX_DATA_INTERFACE_ERROR_CODE 107
#define MIN_GRAPHICS_INTERFACE_ERROR_CODE 200
#define MAX_GRAPHICS_INTERFACE_ERROR_CODE 291
#define MIN_PROCESS_INTERFACE_ERROR_CODE 400
#define MAX_PROCESS_INTERFACE_ERROR_CODE 401
#define COMMON_ERROR_ITEMS 10
#define DATA_INTERFACE_ERROR_ITEMS 8
#define GRAPHICS_INTERFACE_ITEMS 92 
#define PROCESS_INTERFACE_ERROR_ITEMS 2

int getpid();
int kill(int pid, int code);

static int v_WriteErrorToStdio ( char* msg1, char msg[4][200], FILE* fp );


/************************************************************************
 *                                                                      *
 *      Function        : VAddBackgroundProcessInformation              *
 *      Description     : This function will add the new command        *
 *                        triggered the background process, the         *
 *                        current process id, and the current date to   *
 *                        the BG_STATUS.COM file which is an ASCII file,*
 *                        and each line contains command name, process  *
 *                        id, and date. You should call this function at*
 *                        very beginning of your program source code if *
 *                        you plan to run this program in background,   *
 *                        so you can write the information about this   *
 *                        process to the file BG_STATUS.COM.            *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *                         4 - file open error.                         *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDeleteBackgroundProcessInformation.          *
 *      History         : Written on June 7, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VAddBackgroundProcessInformation ( char* command )
{
        FILE *fp ;
        time_t btime ;
        char *date ;
        int pid, error ;

        if (command == NULL)
            v_print_fatal_error("VAddBackgroundProcessInformation",
                "The pointer of command should not be NULL.", 0);
        pid=getpid() ;
        fp=fopen("BG_STATUS.COM","a+b") ;
        if (fp == NULL) return(4) ;
        time(&btime) ;
        date=ctime(&btime) ;
        error=fprintf(fp,"%s %d %s\n",command,pid,date) ;
        if (error == 0) return(3) ;
        fclose(fp) ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VDeleteBackgroundProcessInformation           *
 *      Description     : This function will remove the the information *
 *                        about a background process from BG_STATUS.COM * 
 *                        file. If BG_STATUS.COM does not exist, this   *
 *                        function will return 4, if the write error    *
 *                        occurred, then this function will return 3,   *
 *                        or if the read error occurred, then this      *
 *                        function will return 2. Otherwise this        *
 *                        function will remove the ID of current process*
 *                        coresponding command and date from the file   *
 *                        BG_STATUS.COM and return 0. You should call   *
 *                        this function before you exit your program if *
 *                        you plan to run this program in background, so*
 *                        you can remove the information about this     *
 *                        process from the file BG_STATUS.COM.          *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         4 - file open error.                         *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VAddBackgroundProcessInformation.             *
 *      History         : Written on June 7, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VDeleteBackgroundProcessInformation ( void )
{
        FILE *fp, *fp1 ;
        char command[20], /*date[30],*/ line[100], temp[100] ;
        int temp_pid, pid, error ;
        int i, j, k ;
            
        fp=fopen("BG_STATUS.COM","rb") ;
        if (fp == NULL) return(4) ;
        fp1=fopen("BG_TEMP","wb") ;
        pid=getpid() ;
        while (1) {
            j=0 ;
            while (1) {
                i=getc(fp) ;
                if (i != '\n' && i != '\0' && i != EOF) line[j++]=i ;
                else {
                    line[j]='\0' ;
                    j=0 ;
                    break ;
                }
            }
            if (i == EOF) break ;
            if (line[0]!=0) {
              for (i=0; line[i] != ' '; i++) command[i]=line[i] ;
              for(j=i+1, k=0; line[j] != ' '; j++, k++) temp[k]=line[j] ;
              temp[k]='\0' ;
              temp_pid=(int)atoi(temp) ;
              if (pid != temp_pid) {
                error=fprintf(fp1,"%s\n",line) ;
                if (error == 0) return(3) ;
              }
            }
        }
        fclose(fp) ;
        fclose(fp1) ;
        system("mv BG_TEMP BG_STATUS.COM") ;
        return(0) ;
}



/************************************************************************
 *                                                                      *
 *      Function        : VWriteError                                   *
 *      Description     : This function will write output process name, *
 *                        function name, date error occurred, and error *
 *                        message to the disk file "3DVIEWNIX.ERR" to   *
 *                        keep records. If the 3DVIENWIX.ERR file can   *
 *                        not be written, then this function will write *
 *                        the message to the standard output device, and*
 *                        exit from the current process. This function  *
 *                        will output error message to two disk files.  *
 *                        One is 3DVIEWNIX.ERR under the current        *
 *                        directory and the other one is 3DVIEWNIX.ERR  *
 *                        under the 3DVIEWNIX/FILES directory which the *
 *                        system manager should change the write        *
 *                        by using chmod command when the 3DVIEWNIX is  *
 *                        installed.                                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         4 - file open error.                         *
 *                         7 - environment variable does not exist.     *
 *      Parameters      :  process - Specifies the name of the process  *
 *                              produced the error.                     *
 *                         function - Specifies the name of function    *
 *                              produced the error.                     *
 *                         msg - Specifies the error message to be      *
 *                              output to the file 3DVIEWNIX.ERR.       *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on January 12, 1989 by Hsiu-Mei Hung. *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VWriteError ( char* process, char* function, char* msg )
#if 0
char *process ;         /* process name to be printed */
char *function ;        /* function name to be printed */
char *msg ;             /* error message to be printed */
#endif
{
        FILE *fp;
        char *date ;    /* the date and time when an error is occurred */
        char msg1[4][200];
        time_t clock ;
        int error ;

        if (process == NULL) 
            v_print_fatal_error("VWriteError",
                "The pointer of process should not be NULL.",0) ;
        if (function == NULL) 
            v_print_fatal_error("VWriteError",
                "The pointer of function should not be NULL.",0) ;
        if (msg == NULL) 
            v_print_fatal_error("VWriteError",
                "The pointer of msg should not be NULL.",0) ;
        time(&clock) ;
        date=(char *)ctime(&clock) ;
        fp=fopen("3DVIEWNIX.ERR","ab") ;
        if (fp == NULL) return(4) ;
/*      env=getenv("VIEWNIX_ENV") ;
        if (env == NULL) return(7) ;
        sprintf(filename,"%s/%s/3DVIEWNIX.ERR",env,FILES_PATH) ;
        fp1=fopen(filename,"ab") ;
        if (fp1 == NULL) return(4) ;
*/
        sprintf(msg1[0],"\nProcess Name   : %s\n",process) ;
        sprintf(msg1[1],"Function Name  : %s\n",function) ;
        sprintf(msg1[2],"Error Occurred : %s",date) ;
        sprintf(msg1[3],"Error Message  : %s\n\n",msg) ;
        error=fprintf(fp,"%s",msg1[0]) ;
        if (error == 0) v_WriteErrorToStdio("under the current directory",
                                             msg1,fp) ;
        fclose(fp) ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_WriteErrorToStdio                           *
 *      Description     : This function will writes the error to the    *
 *                        standard I/O. This is done when the error     *
 *                        cannot be written to 3DVIEWNIX.ERR file.      *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  msg - Specifies the error message to be      *
 *                              output to standard error stream.        *
 *                         msg1 - Specifies the directory message.      *
 *                         fp - Specifies the file pointer to be closed.*
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VWriteError.                                  *
 *      History         : Written on April 25, 1991 by Hsiu-Mei Hung.   *
 *                                                                      *
 ************************************************************************/
static int v_WriteErrorToStdio ( char* msg1, char msg[4][200], FILE* fp )
{
        char msg2[100] ;
        int i ;

        sprintf(msg2,
                "Can not write error message to disk file 3DVIEWNIX.ERR %s",
                msg1) ;
        printf("%s\n",msg2) ;
        for (i=0; i<4; i++) printf("%s",msg[i]) ;
        fclose(fp) ;
        exit(-1) ;

}


/************************************************************************
 *                                                                      *
 *      Function        : v_print_fatal_error                           *
 *      Description     : This function will print out the proper       *
 *                        message to the standard output device and exit*
 *                        from the current process.                     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  function - SPpcifies the name of the function* 
 *                              calling this function.                  *
 *                         msg - Specifies a string of message to be    *
 *                              output.                                 *
 *                         value - Specifies the value user assigned.   *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on October 1, 1990 by Hsiu-Mei Hung.  *
 *                        Modified on May 10, 1993 by Krishna Iyer.     *
 *                                                                      *
 ************************************************************************/
void v_print_fatal_error ( char* function, char* msg, int value )
{
        char *date ;    /* the date and time when an error is occurred */
        time_t clock ;

        if (function == NULL) 
            v_print_fatal_error("v_print_fatal_error",
                "The pointer of function should not be NULL.",0) ;
        if (msg == NULL) 
            v_print_fatal_error("v_print_fatal_error",
                "The pointer of msg should not be NULL.",0) ;
        time(&clock) ;
        date=(char *)ctime(&clock) ;
        printf("Error occurs in the function %s on %s",function,date) ;
        printf("%s\n",msg) ;
        printf("Please note that the value assigned was %d\n",value) ;
        exit(-1);
}


/************************************************************************
 *                                                                      *
 *      Function        : VPrintFatalError                              *
 *      Description     : This function will print out the proper       *
 *                        message to the standard output device and exit*
 *                        from the current process.                     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  function - SPpcifies the name of the function*
 *                              calling this function.                  *
 *                         msg - Specifies a string of message to be    *
 *                              output.                                 *
 *                         value - Specifies the value user assigned.   *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on October 1, 1990 by Hsiu-Mei Hung.  *
 *                                                                      *
 ************************************************************************/
int VPrintFatalError ( char* function, char* msg, int value )
{
        char *date ;    /* the date and time when an error is occurred */
        time_t clock ;
 
        if (function == NULL)
            v_print_fatal_error("v_print_fatal_error",
                "The pointer of function should not be NULL.",0) ;
        if (msg == NULL)
            v_print_fatal_error("v_print_fatal_error",
                "The pointer of msg should not be NULL.",0) ;
        time(&clock) ;
        date=(char *)ctime(&clock) ;
        printf("Error occurs in the function %s on %s",function,date) ;
        printf("%s\n",msg) ;
        printf("Please note that the the value assigned was %d\n",value) ;
        kill(getpid(),LIB_EXIT) ;

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDecodeError                                  *
 *      Description     : This function will return a string of message *
 *                        according to the error user specifies and     *
 *                        output the name of process and function where *
 *                        the error happened, and the current date to   *
 *                        the disk file 3DVIEWNIX.ERR. If this function *
 *                        can not write to 3DVIEWNIX.ERR, then this     *
 *                        function will print to the standard output    *
 *                        device, and exit from the current process.    *
 *                        If the error is less than -1, then this       *
 *                        function will print the proper message in the *
 *                        standard output device and exit from the      *
 *                        current process.                              *
 *      Return Value    :  0 - work successfully.                       *
 *                         4 - cannot open ERROR_CODES file.            *
 *                         7 - environment variable does not exist.     *
 *      Parameters      :  process - Specifies the name of the current  *
 *                              process.                                *
 *                         function - Specifies the name of the function*
 *                              calling this function.                  *
 *                         error - Specifies the code number of error.  *
 *                         msg - Returns ta string of message.          *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on October 1, 1990 by Hsiu-Mei Hung.  *
 *                        Modified on May 22, 1993 by Krishna Iyer.     *
 *                                                                      *
 ************************************************************************/
int VDecodeError ( char* process, char* function, int error, char msg[200] )
{
        char msg1[200], c, filename[80], *env, msg2[200] ;
        FILE *fp ;
        int num, i, j, result ;

        if (function == NULL) 
            v_print_fatal_error("VDecodeError",
                "The pointer of function should not be NULL.",0) ;
        if (process == NULL) 
            v_print_fatal_error("VDecodeError",
                "The pointer of process should not be NULL.",0) ;
        if (error < MIN_COMMON_ERROR_CODE || 
            (error > MAX_COMMON_ERROR_CODE && 
             error < MIN_DATA_INTERFACE_ERROR_CODE) || 
            (error > MAX_DATA_INTERFACE_ERROR_CODE && 
             error < MIN_GRAPHICS_INTERFACE_ERROR_CODE) || 
            (error > MAX_GRAPHICS_INTERFACE_ERROR_CODE && 
             error < MIN_PROCESS_INTERFACE_ERROR_CODE) ||
            error > MAX_PROCESS_INTERFACE_ERROR_CODE) {
            sprintf(msg1,"The error code is invalid. See MIPG Technical ") ;
            sprintf(msg1,"%s Report 178 - APPENDIX A",msg1) ;
            v_print_fatal_error("VDecodeError",msg1,error) ;
        }
        if (msg == NULL) 
            v_print_fatal_error("VDecodeError",
                "The pointer of msg should not be NULL.",0) ;
        env=getenv("VIEWNIX_ENV") ;
        if (env == NULL) return(7) ;
        sprintf(filename,"%s/%s/ERROR_CODES",env,FILES_PATH) ;
        fp=fopen(filename,"rb") ;
        if (fp == NULL) return(4) ;
        num=0 ;
        if (error < MAX_COMMON_ERROR_CODE) num=error+2 ;
        else {
            if (error < MAX_DATA_INTERFACE_ERROR_CODE) 
                num=COMMON_ERROR_ITEMS+(error-MIN_DATA_INTERFACE_ERROR_CODE+1);
            else {
                if (error < MAX_GRAPHICS_INTERFACE_ERROR_CODE)
                    num=COMMON_ERROR_ITEMS+DATA_INTERFACE_ERROR_ITEMS+
                        (error-MIN_GRAPHICS_INTERFACE_ERROR_CODE+1);
                else {
                    if (error < MAX_PROCESS_INTERFACE_ERROR_CODE)
                        num=COMMON_ERROR_ITEMS+DATA_INTERFACE_ERROR_ITEMS+
                            GRAPHICS_INTERFACE_ITEMS+
                            (error-MIN_PROCESS_INTERFACE_ERROR_CODE+1);
                }
            }
        }
        for (i=0; i<num; i++) {
            j=0 ;
            strcpy(msg2,"") ;
            while (True) {
                c=getc(fp) ;
                if (c == '\n') break ;
                msg2[j++]=c ;
            }
            msg2[j]='\0' ;
        }
        result=VWriteError(process,function,msg2) ;
        fclose(fp) ;
        if(lib_cmap_created == 1) 
        {
         if(error==1 || error==2 || error==3 || error==4 || error==5 ||
         error==100 || error==101 || error==102 || error==103 || error==104 ||
         error==222 || error==235 || error==239 || error==247 || error==400 ||
         error==401)
         {
                if(lib_error_function_ptr) lib_error_function_ptr(msg2);
                return(279);
         }
        }
        strcpy(msg,msg2) ;
        return(result) ;
}

        
#if defined (WIN32) || defined (_WIN32)

int kill(int pid, int code)
{
	exit(code);
}

#endif
