#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include <windows.h>

/* Generate object file: gcc -c myconio.c
 * In your C++ code, declare functions as:
 *
 * extern "C" {
 *     int getch (void);
 *     int getche (void);
 * }
 *
 * g++ -o yourapp yourapp.cpp myconio.o
 *
 * You can also combine several *.o into a *.a
 * ar rcs myconio.a myconio.o helpers.o
 *
 * And statically linked the same with:
 * g++ -o yourapp yourapp.cpp myconio.a
 * 
 * Or create a dynamic library by:
 * gcc -shared myconio.o -o myconio.dll
 * # note: In cygwin, it must be in dll suffix!
 * g++ -o win win.cpp -L./ -lmyconio
 * 
 * Of course, the app and the dll must be beside 
 * each other or use PATH (LD_LIBRARY_PATH not
 * honored here in cygwin).
 * 
 */

/* reads from keypress, doesn't echo */
int getch(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

/* reads from keypress, echoes */
int getche(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

void _getch(void) 
{
    getch();
}
