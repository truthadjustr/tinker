#include <windows.h> 
#include <stdio.h>
#include <tchar.h>
#include <assert.h>

//#define BUFSIZE 512
#define BUFSIZE 7

#ifdef _MSC_VER
#include <conio.h>
int _tmain(int argc, TCHAR *argv[]) 
#else
extern "C" void _getch(void);
#define _tprintf printf
int main(int argc, char *argv[]) 
#endif
{ 
   HANDLE hPipe; 
   LPCTSTR lpvMessage=TEXT("Dexxxlt message from client."); 
   TCHAR  chBuf[45]; 
   BOOL   fSuccess = FALSE; 
   DWORD  cbRead, cbToWrite, cbWritten, dwMode, index,dwErr; 
   LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe"); 

   if( argc > 1 )
      lpvMessage = argv[1];
 
// Try to open a named pipe; wait for it, if necessary. 
 
   while (1) 
   { 
      hPipe = CreateFile( 
         lpszPipename,   // pipe name 
         GENERIC_READ |  // read and write access 
         GENERIC_WRITE, 
         0,              // no sharing 
         NULL,           // default security attributes
         OPEN_EXISTING,  // opens existing pipe 
         0,              // default attributes 
         NULL);          // no template file 

	  dwErr = GetLastError();
   // Break if the pipe handle is valid. 
 
      if (hPipe != INVALID_HANDLE_VALUE) {
         break; 
      } 
 
      // Exit if an error other than ERROR_PIPE_BUSY occurs. 
 
      if (dwErr != ERROR_PIPE_BUSY) 
      {
         _tprintf( TEXT("Could not open pipe. GLE=%d\n"), GetLastError() ); 
         return -1;
      }
 
      // All pipe instances are busy, so wait for 20 seconds. 
	  printf("All pipe instances are busy, so wait for 20 seconds.\n");
 
      if ( ! WaitNamedPipe(lpszPipename, 20000)) 
      { 
         printf("Could not open pipe: 20 second wait timed out."); 
         return -1;
      } 
   } 
 
// The pipe connected; change to message-read mode. 
 
   dwMode = PIPE_READMODE_MESSAGE; 
   //dwMode = PIPE_READMODE_BYTE; 
   fSuccess = SetNamedPipeHandleState( 
      hPipe,    // pipe handle 
      &dwMode,  // new pipe mode 
      NULL,     // don't set maximum bytes 
      NULL);    // don't set maximum time 
   if ( ! fSuccess) 
   {
      _tprintf( TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError() ); 
      return -1;
   }
 
// Send a message to the pipe server. 
 
   cbToWrite = (lstrlen(lpvMessage)+1)*sizeof(TCHAR);
   _tprintf( TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage); 

   cbWritten = 0;
   index = cbWritten;

   do {
	   fSuccess = WriteFile(
		   hPipe,                  // pipe handle 
		   lpvMessage + index,             // message 
		   cbToWrite - index,              // message length 
		   &cbWritten,             // bytes written 
		   NULL);                  // not overlapped 
	
	   dwErr = GetLastError();
	   if (dwErr == ERROR_PIPE_NOT_CONNECTED) break;
	   index += cbWritten;

   } while (!fSuccess);

   assert(cbToWrite == index);

   if ( ! fSuccess) 
   {
      _tprintf( TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError() ); 
      return -1;
   }

   printf("\nMessage sent to server, receiving reply as follows:\n");
   cbRead = 0;
   index = cbRead;
   do 
   { 
   // Read from the pipe. 
 
      fSuccess = ReadFile( 
         hPipe,    // pipe handle 
         chBuf + index,    // buffer to receive reply 
         BUFSIZE*sizeof(TCHAR),  // size of buffer 
         &cbRead,  // number of bytes read 
         NULL);    // not overlapped 

	  index += cbRead;
	  dwErr = GetLastError();

      if ( ! fSuccess && dwErr != ERROR_MORE_DATA )
         break; 
 
      if (fSuccess) _tprintf( TEXT("\"%s\"\n"), chBuf ); 
#ifndef _WIN32
      printf("pausing read 5 bytes, press enter to read next 5 bytes\n");
      _getch();
#endif
   //} while ( ! fSuccess);  // repeat loop if ERROR_MORE_DATA 
   } while ( fSuccess); 

   if ( ! fSuccess)
   {
      _tprintf( TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError() );
      return -1;
   }

#ifndef _WIN32
   printf("\n<End of message, press ENTER to terminate connection and exit>");
   _getch();
#endif 

   CloseHandle(hPipe); 
 
   return 0; 
}
