#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int i = 0;
    while (1)
    {
        printf("%d", i);
        fflush(stdout);
        sleep(1);
        i++;
    }
}