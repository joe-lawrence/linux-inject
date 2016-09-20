/*
 * GHOST vulnerability check
 * http://www.openwall.com/lists/oss-security/2015/01/27/9
 * Usage: gcc GHOST.c -o GHOST && ./GHOST
 */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define CANARY "in_the_coal_mine"

struct {
        char buffer[1024];
        char canary[sizeof(CANARY)];
} temp = { "buffer", CANARY };

int main(void) {

	int i;

	for (i=0; ; i++) {

                struct hostent resbuf;
                struct hostent *result;
                int herrno;
                int retval;

                /*** strlen (name) = size_needed - sizeof (*host_addr) - sizeof (*h_addr_ptrs) - 1; ***/
                size_t len = sizeof(temp.buffer) - 16*sizeof(unsigned char) - 2*sizeof(char *) - 1;
                char name[sizeof(temp.buffer)];
                memset(name, '0', len);
                name[len] = '\0';

                retval = gethostbyname_r(name, &resbuf, temp.buffer, sizeof(temp.buffer), &result, &herrno);

                if (strcmp(temp.canary, CANARY) != 0) {
                        printf("%d\t\e[31mvulnerable\e[0m\n", i);
                }
                if (retval == ERANGE) {
                        printf("%d\t\e[32mnot vulnerable\e[0m\n", i);
                }

                sleep(5);
        }

        return 0;
}
