#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "jsmn.h"

#include "json.h"

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int strntoi(const char *str, uint32_t len)
{
    int32_t ret = 0;
    int i;
    for(i = 0; i < len; i++)
    {
        if(str[i] == '\0')
        {
            return ret;
        }

        if(str[i] >= '0' && str[i] <= '9')
        {
            ret *= 10;
            ret += str[i] - '0';
        }
    }
    return ret;
}
