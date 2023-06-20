#include <stdio.h>
#include <stdlib.h>
#include "os.h"

#if OS == OS_WINDOWS
#include <conio.h>
#endif

void proper_exit(int exit_code)
{
        printf("\nProcess returned %d (0x%x).\n", exit_code, exit_code);

        #if OS == OS_WINDOWS
        printf("Press any key to continue ...");
        (void) getch();
        #else
        printf("Press enter to continue ...");
        (void) getchar();
        #endif

        exit(exit_code);
}
