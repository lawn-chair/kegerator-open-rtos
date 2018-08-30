#ifndef __HTTPD_H__
#define __HTTPD_H__

extern HttpdBuiltInUrl builtInUrls[];

int ICACHE_FLASH_ATTR cgiTempEndpoint(HttpdConnData *);
#endif