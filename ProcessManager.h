/*
  Copyright 1993-2013 Medical Image Processing Group
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
 * \file   ProcessManager.h
 * \brief  Definition and implementation of ProcessManager class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ProcessManager_h
#define __ProcessManager_h
//----------------------------------------------------------------------
/**
 * \brief Definition of ProcessManager class.
 *
 * Process types: (1) foreground and (2) background.
 *
 * A foreground process causes a modal progress dialog box to be immediately
 * displayed.  The user waits until the process runs to completion or
 * the user presses a cancel button in the dialog box.
 *
 * Background processes also cause a dialog box to be immediately displayed
 * but this dialog box is _not_ modal.
 * <pre>
 * example:
 *     ...
 *     #include  "ProcessManager.h"
 *     ...
 *     ProcessManager  p( "interpolate started", cmd, foreground );
 *     ...
 * </pre>
 */
class ProcessManager {
protected:
    //------------------------------------------------------------------
    //------------------------------------------------------------------
    /**
     * \brief Definition of Process class.  Process executes the specified
     *        command, provides access to stdin and stderr of the process
     *        (to obtain information and error messages), and is notified
     *        when the process terminates.
     */
    class Process : public wxProcess {
    protected:
        const wxString  mCommand;      ///< command to be executed
        int             mFinalStatus;  ///< final status when process terminates
        int             mPid;          ///< process id
        bool            mWasCanceled;  ///< true if user prematurely terminates
    public:
        /**
         * \brief Process ctor which causes process I/O to be made available
         *        and executes the command/creates the actual process.
         * \param command is the command to be executed.
         */
        Process ( const wxString& command )
            : mCommand(command), mFinalStatus(0), mPid(0), mWasCanceled(false)
        {
            Redirect();
            mPid = ::wxExecute( command, wxEXEC_ASYNC, this );
        }
        /**
         * \brief   Accessor for the cancel flag.
         * \returns True if the user canceled; false otherwise.
         */
        bool getCancel ( void ) const { return mWasCanceled; }
        /**
         * \brief   Accessor for the process id.
         * \returns The process id.
         */
        int getPid ( void ) const { return mPid; }
        /**
         * \brief   Accessor for process final return status value.
         * \returns The final return status value.  (This value is only
         *          value after the process has actually completed.
         *          use wxProcess::Exists(pid) to determine if the process
         *          is still running.)
         */
        int getStatus ( void ) const { return mFinalStatus; }
        /**
         * \brief   This method returns a line read from stdin/cin.
         * \returns A line read from stdin/cin or "".  (Use
         *          IsInputAvailable() to determine if anything is available.)
         */
        wxString readFromInput ( void ) {
            if (IsInputAvailable()) {
                wxTextInputStream  tis( *GetInputStream() );
                return tis.ReadLine();
            }
            return "";
        }
        /**
         * \brief   This method returns a line read from stderr/cerr.
         * \returns A line read from stderr/cerr or "".  (Use
         *          IsErrorAvailable() to determine if anything is available.)
         */
        wxString readFromError ( void ) {
            if (IsErrorAvailable()) {
                wxTextInputStream  tis( *GetErrorStream() );
                return tis.ReadLine();
            }
            return "";
        }
        /**
         * \brief This method is overridden so we can be notified when the
         *        process terminates.
         * \param pid is the process id.
         * \param status is the completion status of the process.
         */
        void OnTerminate ( int pid, int status ) {
            assert( pid == mPid );
            mFinalStatus = status;
        }
    };
    //------------------------------------------------------------------
    //------------------------------------------------------------------
    int   mFinalStatus;  ///< final status when process terminates
    bool  mWasCanceled;  ///< true if user prematurely terminates
public:
    /** \brief ProcessManager ctor.
     *  \param message specifies the dialog box text.
     *  \param command is the command to be executed.
     *  \param foreground is true if the user requires a foreground 
     *         process/modal dialog box; false otherwise (modeless/non-modal
     *         dialog box).
     *  \param check is false when the caller wants the process manager
     *         to skip checking for the existence of the program
     *         before it runs the command (this is useful for shell
     *         commands or other commands in the user's path).
     *         a value of true indicates that the process manager
     *         should first check and if the program doesn't exist, the 
     *         user will be notified.
     *  \param notify is true if the user wants a dialog box to appear when the
     *         process completes; false indicates that no such dialog box should
     *         appear.
     */
    ProcessManager ( const wxString& message, const wxString& command,
                     const bool isForeground=true, const bool check=false,
                     const bool notify=false )
        : mFinalStatus(-1), mWasCanceled(false)
    {
        if (check) {
            //make sure that the program is present.
			int whereBegin = command.GetChar(0)=='"';
			int  whereEnd = whereBegin? command.AfterFirst('"').Find( '"')+1:
				command.Find( ' ' );
            wxString  program = "";
            if (whereEnd == -1) {
                //no space in command so the entire command must be the file name
                program = message;
            } else {
                //space present in command to extract the file name
                program = command.Mid( whereBegin, whereEnd-whereBegin );
            }
            #if defined (WIN32) || defined (_WIN32)
                if (!program.Matches("*.exe") && !program.Matches("*.EXE"))
                    program = wxString::Format( "%s.exe", (const char *)program.c_str() );
            #endif
            if (!::wxFileExists( program )) {
                wxString  tmp = wxString::Format( "Sorry.\n\nCan't find the program: %s.\n\nPlease make sure the home preference is set correctly.",
                    (const char *)program.c_str() );
                wxMessageBox( tmp, "Missing program", wxICON_ERROR | wxOK );
//                mFinalStatus = -1;
//                mWasCanceled = false;
                return;
            }
        }

        int  style = /*wxPD_AUTO_HIDE |*/ wxPD_SMOOTH | wxPD_CAN_ABORT
                   | wxPD_ELAPSED_TIME;
        if (isForeground)    style |= wxPD_APP_MODAL;
        wxProgressDialog*  pd = new wxProgressDialog( "Progress...", message,
                                                      100, NULL, style );
        Process*   p   = new Process( command );
        const int  pid = p->getPid();
        if (!pid) {
            wxMessageBox( "Can't execute the command.", "Sorry...",
                          wxOK | wxICON_ERROR );
//            mFinalStatus = -1;
//            mWasCanceled = false;
            delete pd;    pd = NULL;
            delete p;     p  = NULL;
            return;
        }
#ifndef HAS_PULSE
        int  pulse=2;  //for older progress dialog w/out pulse
#endif
        while (wxProcess::Exists(pid)) {
#ifndef HAS_PULSE
            bool  keepGoing = pd->Update( pulse );
            if (pulse==2)    pulse=8;
            else             pulse=2;
#else
            bool  keepGoing = pd->Pulse();
#endif
            if (!keepGoing) {
                if (wxMessageBox(_T("Do you really want to cancel?"),
                                 _T("Progress dialog question"),  // caption
                                 wxYES_NO | wxICON_QUESTION) == wxYES)
                {
                    wxProcess::Kill( pid, wxSIGKILL );
                    mFinalStatus = 0;
                    mWasCanceled = true;
                    break;
                } else {
                    pd->Resume();
                }
            }
            while (p->IsInputAvailable()) {
                wxString  msg = p->readFromInput();
                wxLogMessage( "%s", (const char *)msg.c_str() );  //be careful of % in msg
#ifndef HAS_PULSE
                pd->Update( 5, msg );
#else
                pd->Pulse( msg );
#endif
            }
            while (p->IsErrorAvailable()) {
                wxString  msg = p->readFromError();
                if (msg.Length()<1)    continue;
                msg = "error: " + msg;
                wxLogMessage( "%s", (const char *)msg.c_str() );  //be careful of % in msg
#ifndef HAS_PULSE
                pd->Update( 5, msg );
#else
                pd->Pulse( msg );
#endif
            }
            ::wxMilliSleep( 500 );
        }
#ifndef HAS_PULSE
        pd->Update( 5, "finished" );
#else
        pd->Pulse( "finished" );
#endif
        if (!mWasCanceled)    mFinalStatus = p->getStatus();
        delete pd;    pd = NULL;
        delete p;     p  = NULL;
        if (notify && !mWasCanceled)
            wxMessageBox( "Process has completed.", "Happy ending...",
                          wxOK | wxICON_INFORMATION );
    }
    /**
     * \brief   Accessor for the cancel flag.
     * \returns True if the user canceled; false otherwise.
     */
    bool getCancel ( void ) const {  return mWasCanceled;  }
    /**
     * \brief   Accessor for process final return status value.
     * \returns The final return status value.  (This value is only
     *          value after the process has actually completed.
     *          use wxProcess::Exists(pid) to determine if the process
     *          is still running.)
     */
    int getStatus ( void ) const {  return mFinalStatus;  }

};
//----------------------------------------------------------------------
class ParallelProcessManager {
protected:
    ProcessManager*  mProcess;
public:
    /** \brief Exactly the same as the ProcessManager ctor. */
    ParallelProcessManager ( const wxString& message, const wxString& command,
                     const bool isForeground=true, const bool check=false,
                     const bool notify=true )
    {
#if defined (WIN32) || defined (_WIN32)
        //have to add cmd /c for windows.
        // otherwise, mpiexec won't terminate (forever)!
        wxString  s = "cmd /c \"\"" + Preferences::getMPIDirectory() + "/mpiexec\"";

		  s += " -genv VIEWNIX_ENV \"" + Preferences::getHome() + "\"";
        //determine # of hosts, host names, and process counts
        int  hostCount=0, processCount=0;
        wxString  hostList = "";
		for (int i=0; i<Preferences::HostCount; i++) {
            wxString  tmp = Preferences::getHostName( i );
            if (tmp.Len()<=0)    continue;
            hostList += " " + tmp;
            tmp = Preferences::getHostProcessCount( i );
            hostList += " ";
            if (tmp.Len()>0) {
                hostList += tmp;
                int  x = 0;
                int  n = sscanf( (const char *)tmp.c_str(), "%d", &x );
                if (n!=1 || x<1) {  //conversion problem or bad value?
                    wxString  tmp = wxString::Format( "Number of processes must be greater than 0." );
                    wxMessageBox( tmp, "Bad number of processes specified!", wxICON_ERROR | wxOK );
                    return;
                }
                processCount += x;
            } else {
                hostList += "1";
                ++processCount;
            }
            ++hostCount;
        }
        if (hostCount<1) {
            wxChar  buff[ 256 ];
            ::wxGetHostName( buff, sizeof buff );
            wxString  tmp = wxString::Format( "Please specify the name of at least one system.\n\n(The name of your system is %s.)", buff );
            wxMessageBox( tmp, "No systems specified!", wxICON_ERROR | wxOK );
            return;
        }
        if (processCount<3) {
            wxMessageBox( "Specifying a total of less than\n3 parallel processes may cause problems.",
                "Potential problem detected.", wxICON_WARNING | wxOK );
        }
        //add -hosts # to command line
        s = wxString::Format( "%s -exitcodes -l -nopopup_debug -noprompt -hosts %d", (const char *)s.c_str(), hostCount );
        //add individual hosts and number of processes to command line
        s += hostList;
        s += " ";
        s += command;

		s += "\"";

        wxLogMessage( "%s", (const char *)s.c_str() );  //be careful of % in msg
        mProcess = new ProcessManager( message, s, isForeground, check, notify );
        wxLogMessage( "End parallel process." );
#else
        wxString  s = "\"" + Preferences::getMPIDirectory() + "/mpiexec\"";

        s += " -genv VIEWNIX_ENV \"" + Preferences::getHome() + "\"" + " -l ";
        //determine # of hosts, host names, and process counts
        int  hostCount=0, processCount=0;
        wxString  hostList = "";
		for (int i=0; i<Preferences::HostCount; i++) {
            wxString  hostName = Preferences::getHostName( i );
            if (hostName.Len()<=0)    continue;
            hostList += " " + hostName;
            wxString tmp = Preferences::getHostProcessCount( i );
            hostList += " ";
            if (tmp.Len()>0) {
                hostList += tmp;
                int  x = 0;
                int  n = sscanf( (const char *)tmp.c_str(), "%d", &x );
                if (n!=1 || x<1) {  //conversion problem or bad value?
                    wxString  tmp = wxString::Format( "Number of processes must be greater than 0." );
                    wxMessageBox( tmp, "Bad number of processes specified!", wxICON_ERROR | wxOK );
                    return;
                }

				if( i>0 )
					s += " : ";

				s = wxString::Format( "%s -n %d -host %s", (const char *)s.c_str(), x, (const char *)hostName.c_str() );
				s += " ";
				s += command;

                processCount += x;
            } else {
                hostList += "1";
                ++processCount;
            }
            ++hostCount;
        }
        if (hostCount<1) {
            wxString  tmp = "Please specify the name of at least one system.\n\n(The name of your system is " + wxGetHostName() + ".)";
            wxMessageBox( tmp, "No systems specified!", wxICON_ERROR | wxOK );
            return;
        }
        if (processCount<3) {
            wxMessageBox( "Specifying a total of less than\n3 parallel processes may cause problems.",
                "Potential problem detected.", wxICON_WARNING | wxOK );
        }	

		wxLogMessage( "%s", (const char *)s.c_str() );  //be careful of % in msg
        mProcess = new ProcessManager( message, s, isForeground, check, notify );
        wxLogMessage( "End parallel process." );

#endif
    }
    /**
     * \brief   Accessor for the cancel flag.
     * \returns True if the user canceled; false otherwise.
     */
    bool getCancel ( void ) const {  return mProcess->getCancel();  }
    /**
     * \brief   Accessor for process final return status value.
     * \returns The final return status value.  (This value is only
     *          value after the process has actually completed.
     *          use wxProcess::Exists(pid) to determine if the process
     *          is still running.)
     */
    int getStatus ( void ) const {  return mProcess->getStatus();  }
};

#endif
