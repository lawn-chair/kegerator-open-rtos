#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libesphttpd/httpd.h"
#include "libesphttpd/httpdespfs.h"

#include "jsmn.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "kegerator.h"
#include "httpd.h"
#include "json.h"

// The URLs that the HTTP server can handle.
HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.html"},
	{"/api/temp", cgiTempEndpoint, NULL},
	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};


int ICACHE_FLASH_ATTR cgiTempEndpoint(HttpdConnData *conn)
{
    if(conn->conn == NULL)
    {
        return HTTPD_CGI_DONE;
    }

    if(conn->requestType == HTTPD_METHOD_POST) {
        jsmntok_t t[5];
        jsmn_parser p;
        jsmn_init(&p);
        int r = jsmn_parse(&p, conn->post->buff, strlen(conn->post->buff), t, sizeof(t)/sizeof(t[0]));

        if(r < 1 || t[0].type != JSMN_OBJECT)
        {
            httpdStartResponse(conn, 400);
            httpdEndHeaders(conn);
            httpdSend(conn, "{\"error\": \"invalid json\"}", 25);
            return HTTPD_CGI_DONE;
        }

        if(jsoneq(conn->post->buff, &t[1], "setpoint") == 0)
        {
            int32_t value;
            if(*(conn->post->buff + t[2].start) == '+' && t[2].end - t[2].start == 1)
            {
                value = setpoint + 1;
            } else if(*(conn->post->buff + t[2].start) == '-' && t[2].end - t[2].start == 1)
            {
                value = setpoint - 1;
            } else {
                value = strntoi(conn->post->buff + t[2].start, t[2].end - t[2].start);
            }
            if(value >= MIN_TEMP && value <= MAX_TEMP)
            {
               if(xSemaphoreTake(temp_mutex, portMAX_DELAY))
               {
                   setpoint = value;
                   setpoint_dirty++;
                   xSemaphoreGive(temp_mutex);
               }
            }
            else
            {
                httpdStartResponse(conn, 400);
                httpdEndHeaders(conn);
                httpdSend(conn, "{\"error\": \"invalid setpoint\"}", 30);
                return HTTPD_CGI_DONE;
           }
        } else {
            httpdStartResponse(conn, 400);
            httpdEndHeaders(conn);
            httpdSend(conn, "{\"error\": \"no setpoint\"}", 24);
            return HTTPD_CGI_DONE;
        }
    }

    httpdStartResponse(conn, 200);
    httpdHeader(conn, "Content-Type", "application/json");
    httpdEndHeaders(conn);

    char buf[32];
    int len = sprintf(buf, "{\"temp\": %d, \"setpoint\": %d}", current_temp, setpoint);
    httpdSend(conn, buf, len);
    return HTTPD_CGI_DONE;
}
