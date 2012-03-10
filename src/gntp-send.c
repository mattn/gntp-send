#ifdef _WIN32
#include <winsock2.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5.h"
#include "tcp.h"
#include "growl.h"

static char*
string_to_utf8_alloc(const char* str) {
#ifdef _WIN32
  unsigned int codepage;
  size_t in_len = strlen(str);
  wchar_t* wcsdata;
  char* mbsdata;
  size_t mbssize, wcssize;

  codepage = GetACP();
  wcssize = MultiByteToWideChar(codepage, 0, str, in_len,  NULL, 0);
  wcsdata = (wchar_t*) malloc((wcssize + 1) * sizeof(wchar_t));
  wcssize = MultiByteToWideChar(codepage, 0, str, in_len, wcsdata, wcssize + 1);
  wcsdata[wcssize] = 0;

  mbssize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) wcsdata, -1, NULL, 0, NULL, NULL);
  mbsdata = (char*) malloc((mbssize + 1));
  mbssize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) wcsdata, -1, mbsdata, mbssize, NULL, NULL);
  mbsdata[mbssize] = 0;
  free(wcsdata);
  return mbsdata;
#else
  return strdup(str);
#endif
}

static int	opterr = 1;
static int	optind = 1;
static int	optopt;
static char *optarg;

static int
getopts(int argc, char** argv, char* opts) {
  static int sp = 1;
  register int c;
  register char *cp;

  if(sp == 1) {
    if(optind >= argc ||
        argv[optind][0] != '-' || argv[optind][1] == '\0')
      return(EOF);
    else if(strcmp(argv[optind], "--") == 0) {
      optind++;
      return(EOF);
    }
  }
  optopt = c = argv[optind][sp];
  if(c == ':' || (cp=strchr(opts, c)) == NULL) {
    if(argv[optind][++sp] == '\0') {
      optind++;
      sp = 1;
    }
    return('?');
  }
  if(*++cp == ':') {
    if(argv[optind][sp+1] != '\0')
      optarg = &argv[optind++][sp+1];
    else if(++optind >= argc) {
      sp = 1;
      return('?');
    } else
      optarg = argv[optind++];
    sp = 1;
  } else {
    if(argv[optind][++sp] == '\0') {
      sp = 1;
      optind++;
    }
    optarg = NULL;
  }
  return(c);
}

int
main(int argc, char* argv[]) {
  int c;
  int rc;
  char* server = NULL;
  char* password = NULL;
  char* appname = "gntp-send";
  char* notify = "gntp-send notify";
  char* title = NULL;
  char* message = NULL;
  char* icon = NULL;
  char* url = NULL;
  int tcpsend = 1;
  int read_stdin = 0;

  opterr = 0;
  while ((c = getopts(argc, argv, "a:n:s:p:ui")) != -1) {
    switch (optopt) {
    case 'a': appname = optarg; break;
    case 'n': notify = optarg; break;
    case 's': server = optarg; break;
    case 'p': password = optarg; break;
    case 'u': tcpsend = 0; break;
    case 'i': read_stdin = 1; break;
    case '?': break;
    default: argc = 0; break;
    }
    optarg = NULL;
  }

  if (!read_stdin && ((argc - optind) < 2 || (argc - optind) > 4)) {
    fprintf(stderr, "%s: [-u] [-i] [-a APPNAME] [-n NOTIFY] [-s SERVER:PORT] [-p PASSWORD] title message [icon] [url]\n", argv[0]);
    exit(1);
  }

  if (!read_stdin) {
    title = string_to_utf8_alloc(argv[optind]);
    message = string_to_utf8_alloc(argv[optind + 1]);
    if ((argc - optind) >= 3) icon = string_to_utf8_alloc(argv[optind + 2]);
    if ((argc - optind) == 4) url = string_to_utf8_alloc(argv[optind + 3]);
  } else {
    char buf[BUFSIZ], *ptr;
    if (fgets(buf, sizeof(buf)-1, stdin) == NULL)
      exit(1);
    if ((ptr = strpbrk(buf, "\r\n")) != NULL) *ptr = 0;
    title = strdup(buf);
    while (fgets(buf, sizeof(buf)-1, stdin)) {
      if ((ptr = strpbrk(buf, "\r\n")) != NULL) *ptr = 0;
      if (!message) {
        message = malloc(strlen(buf) + 2);
        *message = 0;
      } else {
        strcat(message, "\n");
        message = realloc(message, strlen(message)+strlen(buf) + 2);
      }
      strcat(message, buf);
    }
    if ((argc - optind) >= 1) icon = string_to_utf8_alloc(argv[optind]);
    if ((argc - optind) == 2) url = string_to_utf8_alloc(argv[optind + 1]);
  }

  if (!server) server = "127.0.0.1";

  growl_init();  
  if (tcpsend) {
    rc = growl(server,appname,notify,title,message,icon,password,url);
  } else {
    rc = growl_udp(server,appname,notify,title,message,icon,password,url);
  }
  growl_shutdown();

  if (title) free(title);
  if (message) free(message);
  if (icon) free(icon);
  if (url) free(url);

  return rc;
}

/* vim:set et sw=2 ts=2 ai: */
