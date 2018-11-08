#include <windows.h>
#include <stdio.h>

struct {
    char *desc;
    int nbytes;
} T[] = {
    {"int",sizeof(int)},
    {"char",sizeof(char)},
    {"long",sizeof(long)},
    {"short",sizeof(short)},
    {"wchar_t",sizeof(wchar_t)},
    {"WORD",sizeof(WORD)},
    {"DWORD",sizeof(DWORD)},
    {"BYTE",sizeof(BYTE)},
    {"void*",sizeof(void*)},
    {"int*",sizeof(int*)},
    {"char*",sizeof(char*)},
};

int main () {

    int n = sizeof(T)/sizeof(T[0]);
    for (int i = 0;i < n; i++) {
        printf ("%10s -> %d\n",T[i].desc,T[i].nbytes);
    }

    return 0;
}
