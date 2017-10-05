#include <stdlib.h>
#include <stdio.h>

static const char *usage = "";

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
    return -1;
}

