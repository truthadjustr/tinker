#include <windows.h> 
#include <stdio.h>
#include <tchar.h>
 
#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 
#define INSTANCES 4 
#define PIPE_TIMEOUT 5000
//#define BUFSIZE 4096
#define BUFSIZE 5
#define BUFSIZE2 50
 
typedef struct 
{ 
   OVERLAPPED oOverlap; 
   HANDLE hPipeInst; 
   //TCHAR chRequest[BUFSIZE]; // these are only chunks!
   DWORD cbRead;
   TCHAR chReply[BUFSIZE2];   
   DWORD cbToWrite; 
   TCHAR req[64];  // store here data received from client
   TCHAR resp[64]; // store here data to send to client
   int req_idx;
   DWORD nwritten;
   int totalwritten;
   DWORD cbRet;
   DWORD dwState; 
   BOOL fPendingIO; 
   TCHAR chRequest[BUFSIZE]; // these are only chunks!
} PIPEINST, *LPPIPEINST; 
 
VOID DisconnectAndReconnect(DWORD); 
BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED); 
VOID GetAnswerToRequest(LPPIPEINST); 
 
PIPEINST Pipe[INSTANCES]; 
HANDLE hEvents[INSTANCES]; 

#ifdef _MSC_VER
#include <strsafe.h>
int _tmain(VOID) 
#else 
#define _tprintf printf
int main (int argc, char *argv[])
#endif
{ 
   DWORD i, dwWait, cbRet, dwErr; 
   BOOL fSuccess; 
   LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe"); 
 
// The initial loop creates several instances of a named pipe 
// along with an event object for each instance.  An 
// overlapped ConnectNamedPipe operation is started for 
// each instance. 
 
   for (i = 0; i < INSTANCES; i++) 
   { 
 
   // Create an event object for this instance. 
 
      hEvents[i] = CreateEvent( 
         NULL,    // default security attribute 
         TRUE,    // manual-reset event 
         TRUE,    // initial state = signaled 
         NULL);   // unnamed event object 

      if (hEvents[i] == NULL) 
      {
         printf("CreateEvent failed with %d.\n", GetLastError()); 
         return 0;
      }
 
      Pipe[i].oOverlap.hEvent = hEvents[i]; 
 
      Pipe[i].hPipeInst = CreateNamedPipe( 
         lpszPipename,            // pipe name 
         PIPE_ACCESS_DUPLEX |     // read/write access 
         FILE_FLAG_OVERLAPPED,    // overlapped mode 
         PIPE_TYPE_MESSAGE |      // message-type pipe 
		  //PIPE_TYPE_BYTE |
         PIPE_READMODE_MESSAGE |  // message-read mode 
         //PIPE_READMODE_BYTE |  // byte-read mode 
         PIPE_WAIT,               // blocking mode 
         INSTANCES,               // number of instances 
         BUFSIZE*sizeof(TCHAR),   // output buffer size    !!!!!!
         BUFSIZE*sizeof(TCHAR),   // input buffer size     !!!!!! 
         PIPE_TIMEOUT,            // client time-out 
         NULL);                   // default security attributes 

	  dwErr = GetLastError();
	  if (dwErr == ERROR_ALREADY_EXISTS) {
		  printf("pipe: ERROR_ALREADY_EXISTS\n");
	  }

      if (Pipe[i].hPipeInst == INVALID_HANDLE_VALUE) 
      {
         printf("CreateNamedPipe failed with %d.\n", GetLastError());
         return 0;
      }
 
   // Call the subroutine to connect to the new client
 
      Pipe[i].fPendingIO = ConnectToNewClient(Pipe[i].hPipeInst, &Pipe[i].oOverlap); 
 
      Pipe[i].dwState = Pipe[i].fPendingIO ? 
         CONNECTING_STATE : // still connecting 
         READING_STATE;     // ready to read 
   } 
 
   while (1) 
   { 
   // Wait for the event object to be signaled, indicating 
   // completion of an overlapped read, write, or 
   // connect operation. 
 
      dwWait = WaitForMultipleObjects( 
         INSTANCES,    // number of event objects 
         hEvents,      // array of event objects 
         FALSE,        // does not wait for all 
         INFINITE);    // waits indefinitely 
 
   // dwWait shows which pipe completed the operation. 
 
      i = dwWait - WAIT_OBJECT_0;  // determines which pipe 
      if (i < 0 || i > (INSTANCES - 1)) 
      {
         printf("Index out of range.\n"); 
         return 0;
      }
 
   // Get the result if the operation was pending. 
 
      if (Pipe[i].fPendingIO) 
      { 
         fSuccess = GetOverlappedResult( 
            Pipe[i].hPipeInst, // handle to pipe 
            &Pipe[i].oOverlap, // OVERLAPPED structure 
            &Pipe[i].cbRet /* &cbRet */,            // bytes transferred 
            FALSE);            // do not wait 

		 dwErr = GetLastError();

         switch (Pipe[i].dwState) 
         { 
         // Pending connect operation 
            case CONNECTING_STATE: 
               if (! fSuccess) 
               {
                   printf("Error %d.\n", GetLastError()); 
                   return 0;
               }
               Pipe[i].dwState = READING_STATE; 
               break; 
 
         // Pending read operation 
            case READING_STATE: 
				if (!fSuccess || /* cbRet */ Pipe[i].cbRet == 0)
				{
					if (dwErr == ERROR_MORE_DATA) {
						printf("more data to read...\n");
						break;
					}
					else {
						DisconnectAndReconnect(i);
					}
                  continue; 
               }

               Pipe[i].cbRead = /* cbRet */ Pipe[i].cbRet;
               Pipe[i].dwState = WRITING_STATE; 
               break; 
 
         // Pending write operation 
            case WRITING_STATE: 
               if (! fSuccess || /* cbRet */ Pipe[i].cbRet != Pipe[i].cbToWrite) 
               { 
                  DisconnectAndReconnect(i); 
                  continue; 
               } 
               Pipe[i].dwState = READING_STATE; 
               break; 
 
            default: 
            {
               printf("Invalid pipe state.\n"); 
               return 0;
            }
         }  
      } 
 
   // The pipe state determines which operation to do next. 
 
      switch (Pipe[i].dwState) 
      { 
      // READING_STATE: 
      // The pipe instance is connected to the client 
      // and is ready to read a request from the client. 
 
         case READING_STATE: 
			 if (Pipe[i].cbRet > 0 && Pipe[i].req_idx == 0) {
				 memcpy(Pipe[i].req + Pipe[i].req_idx, Pipe[i].chRequest, Pipe[i].cbRet);
				 Pipe[i].req_idx += Pipe[i].cbRet;
			 }

			 fSuccess = ReadFile(
				 Pipe[i].hPipeInst,
				 Pipe[i].chRequest,
				 BUFSIZE*sizeof(TCHAR), /* #bytes to read */
               &Pipe[i].cbRead, 
               &Pipe[i].oOverlap); 
 
            dwErr = GetLastError(); 
         // The read operation completed successfully. 
 
            if (fSuccess && Pipe[i].cbRead != 0) 
            { 
               Pipe[i].fPendingIO = FALSE; 
               Pipe[i].dwState = WRITING_STATE; 
				memcpy(Pipe[i].req + Pipe[i].req_idx ,Pipe[i].chRequest,Pipe[i].cbRead);
				Pipe[i].req_idx += Pipe[i].cbRead;
               continue; 
            } 
 
         // The read operation is still pending. 
 
            if (! fSuccess && (dwErr == ERROR_IO_PENDING)) 
            { 
               Pipe[i].fPendingIO = TRUE; 
               continue; 
            } 

			if (!fSuccess && (dwErr == ERROR_MORE_DATA)) {
				//printf("ERROR_MORE_DATA");
				Pipe[i].fPendingIO = FALSE; 
				Pipe[i].dwState = READING_STATE; 
				memcpy(Pipe[i].req + Pipe[i].req_idx ,Pipe[i].chRequest,BUFSIZE * sizeof(TCHAR));
				Pipe[i].req_idx += BUFSIZE * sizeof(TCHAR);
				continue;
			}

 
         // An error occurred; disconnect from the client. 
 
            DisconnectAndReconnect(i); 
            break; 
 
      // WRITING_STATE: 
      // The request was successfully read from the client. 
      // Get the reply data and write it to the client. 
 
         case WRITING_STATE: 
            GetAnswerToRequest(&Pipe[i]); 
			// doing a loop here would nullify the point of overlapped-io?!?? 
			// i dunno

				fSuccess = WriteFile( /* the receiver can only receive 5 bytes at a time */
					Pipe[i].hPipeInst,
					Pipe[i].chReply + Pipe[i].totalwritten,
					Pipe[i].cbToWrite,
					// &cbRet, cbRet is global and could get corrupted?
					&Pipe[i].nwritten, /* this won't get updated if fSuccess is FALSE */
					&Pipe[i].oOverlap);
			
				Pipe[i].totalwritten += Pipe[i].nwritten;
				dwErr = GetLastError(); 

         // The write operation completed successfully. 
			//ERROR_NO_DATA => 232

				if (fSuccess && Pipe[i].totalwritten < 49) continue;

            //if (fSuccess && cbRet == Pipe[i].cbToWrite) 
            if (fSuccess && Pipe[i].totalwritten == Pipe[i].cbToWrite) 
            { 
               Pipe[i].fPendingIO = FALSE; 
               Pipe[i].dwState = READING_STATE; 
               continue; 
            } 

			if (!fSuccess && dwErr == ERROR_IO_PENDING) {
				Pipe[i].fPendingIO = FALSE;
				Pipe[i].dwState = READING_STATE;
				continue;
			}
 

         // The write operation is still pending. 
			/*
            dwErr = GetLastError(); 
            if (! fSuccess && (dwErr == ERROR_IO_PENDING)) 
            { 
				Pipe[i].totalwritten += Pipe[i].oOverlap.InternalHigh;
			    Pipe[i].fPendingIO = FALSE;
               continue; 
            } 
			*/ 
         // An error occurred; disconnect from the client. 
 
            DisconnectAndReconnect(i); 
            break; 
 
         default: 
         {
            printf("Invalid pipe state.\n"); 
            return 0;
         }
      } 
  } 
 
  return 0; 
} 
 
 
// DisconnectAndReconnect(DWORD) 
// This function is called when an error occurs or when the client 
// closes its handle to the pipe. Disconnect from this client, then 
// call ConnectNamedPipe to wait for another client to connect. 
 
VOID DisconnectAndReconnect(DWORD i) 
{ 
// Disconnect the pipe instance. 
 
   if (! DisconnectNamedPipe(Pipe[i].hPipeInst) ) 
   {
      printf("DisconnectNamedPipe failed with %d.\n", GetLastError());
   }
 
// Call a subroutine to connect to the new client. 
 
   Pipe[i].fPendingIO = ConnectToNewClient( Pipe[i].hPipeInst, &Pipe[i].oOverlap); 
 
   Pipe[i].dwState = Pipe[i].fPendingIO ? 
      CONNECTING_STATE : // still connecting 
      READING_STATE;     // ready to read 
} 
 
// ConnectToNewClient(HANDLE, LPOVERLAPPED) 
// This function is called to start an overlapped connect operation. 
// It returns TRUE if an operation is pending or FALSE if the 
// connection has been completed. 
 
BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo) 
{ 
   BOOL fConnected, fPendingIO = FALSE; 
 
// Start an overlapped connection for this pipe instance. 
   fConnected = ConnectNamedPipe(hPipe, lpo); 
 
// Overlapped ConnectNamedPipe should return zero. 
   if (fConnected) 
   {
      printf("ConnectNamedPipe failed with %d.\n", GetLastError()); 
      return 0;
   }
 
   switch (GetLastError()) 
   { 
   // The overlapped connection in progress. 
      case ERROR_IO_PENDING: 
         fPendingIO = TRUE; 
         break; 
 
   // Client is already connected, so signal an event. 
 
      case ERROR_PIPE_CONNECTED: 
         if (SetEvent(lpo->hEvent)) 
            break; 
 
   // If an error occurs during the connect operation... 
      default: 
      {
         printf("ConnectNamedPipe failed with %d.\n", GetLastError());
         return 0;
      }
   } 
 
   return fPendingIO; 
}

VOID GetAnswerToRequest(LPPIPEINST pipe)
{
   //_tprintf( TEXT("[%d] %s\n"), pipe->hPipeInst, pipe->chRequest); // %s able to print? so chRequest is null delimited
   _tprintf( TEXT("[%d] %s\n"), pipe->hPipeInst, pipe->req); // %s able to print? so chRequest is null delimited
#ifdef _MSC_VER
   StringCchCopy( pipe->chReply, BUFSIZE2, TEXT("Default answer from server") );
#else
   strncpy (pipe->chReply, TEXT("Default answer from server"), BUFSIZE2);
#endif
   //pipe->cbToWrite = (lstrlen(pipe->chReply)+1)*sizeof(TCHAR);
   pipe->cbToWrite = 3;
}
