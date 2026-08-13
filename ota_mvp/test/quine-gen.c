// convert the contents of stdin to their ASCII values (e.g.,
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>

int main(void)
{
    char buf[4096];
    int len = 0;

    char c = getchar();

    printf("char prog[] = {\n");
    while (c != EOF)
    {
        if ((len + 1) % 8 == 0)
        {
            printf("\t%d,\n", c);
        }
        else
        {
            printf("\t%d, ", c);
        }

        buf[len] = c;
        len++;
        c = getchar();
    }
    printf("0 };\n");

    for (int i = 0; i < len; i++)
    {
        printf("%c", buf[i]);
    }

    return 0;
}
