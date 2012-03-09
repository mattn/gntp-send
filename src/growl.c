#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5.h"
#include "tcp.h"
#include "growl.h"

static const char hex_table[] = "0123456789ABCDEF";
static char*
string_to_hex_alloc(const char* str, int len) {
  int n, l;
  char* tmp = (char*)calloc(1, len * 2 + 1);
  if (tmp) {
    for (l = 0, n = 0; l < len; l++) {
      tmp[n++] = hex_table[(str[l] & 0xF0) >> 4];
      tmp[n++] = hex_table[str[l] & 0x0F];
    }
  }
  return tmp;
}

static volatile int growl_init_ = 0;

GROWL_EXPORT
int growl_init() {
  if (growl_init_ == 0) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
      return -1;
    }
#endif
    srand(time(NULL));
    growl_init_ = 1;
  }
  return 1;
}

GROWL_EXPORT
void growl_shutdown() {
  if (growl_init_ == 1) {
#ifdef _WIN32
    WSACleanup();
#endif
  }
}

static char*
gen_salt_alloc(int count) {
  char* salt = (char*)malloc(count + 1);
  if (salt) {
    int n;
    for (n = 0; n < count; n++) salt[n] = (((int)rand()) % 255) + 1;
    salt[n] = 0;
  }
  return salt;
}

static char*
gen_password_hash_alloc(const char* password, const char* salt) {
  md5_context md5ctx;
  char md5tmp[20] = {0};
  char* md5digest;

  md5_starts(&md5ctx);
  md5_update(&md5ctx, (uint8_t*)password, strlen(password));
  md5_update(&md5ctx, (uint8_t*)salt, strlen(salt));
  md5_finish(&md5ctx, (uint8_t*)md5tmp);

  md5_starts(&md5ctx);
  md5_update(&md5ctx, (uint8_t*)md5tmp, 16);
  md5_finish(&md5ctx, (uint8_t*)md5tmp);
  md5digest = string_to_hex_alloc(md5tmp, 16);

  return md5digest;
}

static char*
growl_generate_authheader_alloc(const char*const password) {
  char* authheader = NULL;

  if (password) {
    char *salt = gen_salt_alloc(8);
    if (salt) {
      char *keyhash = gen_password_hash_alloc(password, salt);
      if (keyhash) {
        char* salthash = string_to_hex_alloc(salt, 8);
        if (salthash) {
          authheader = (char*)malloc(strlen(keyhash) + strlen(salthash) + 7);
          if (authheader) {
            sprintf(authheader, " MD5:%s.%s", keyhash, salthash);
          }
          free(salthash);
        }
        free(keyhash);
      }
      free(salt);
    }
  }
  
  return authheader;
}


GROWL_EXPORT
int
growl_tcp_register(
    const char *const server,
    const char *const appname,
    const char **const notifications,
    const int notifications_count,
    const char *const password,
    const char* const icon) {
  int sock = -1;
  int i = 0;
  char *authheader;
  char *iconid = NULL;
  FILE *iconfile = NULL;
  size_t iconsize;
  uint8_t buffer[1024];

  growl_init();
  authheader = growl_generate_authheader_alloc(password);
  sock = growl_tcp_open(server);
  if (sock > 0) {
    if (icon && icon != "") {
      size_t bytes_read;
      md5_context md5ctx;
      uint8_t md5tmp[20];
      iconfile = fopen(icon, "rb");
      if (iconfile) {
        fseek(iconfile, 0, SEEK_END);
        iconsize = ftell(iconfile);
        fseek(iconfile, 0, SEEK_SET);
        memset(md5tmp, 0, sizeof(md5tmp));
        md5_starts(&md5ctx);
        while (!feof(iconfile)) {
          bytes_read = fread(buffer, 1, 1024, iconfile);
          if (bytes_read) md5_update(&md5ctx, buffer, bytes_read);
        }
        fseek(iconfile, 0, SEEK_SET);
        md5_finish(&md5ctx, md5tmp);
        iconid = string_to_hex_alloc((const char*) md5tmp, 16);
      }
    }
  
    growl_tcp_write(sock, "GNTP/1.0 REGISTER NONE %s", authheader );
    growl_tcp_write(sock, "Application-Name: %s", appname);
    if (iconid) {
      growl_tcp_write(sock, "Application-Icon: x-growl-resource://%s", iconid);  
    } else if (icon && icon != "") {
      growl_tcp_write(sock, "Application-Icon: %s", icon);  
    }
    growl_tcp_write(sock, "Notifications-Count: %d", notifications_count);
    growl_tcp_write(sock, "%s", "");
  
    for (i = 0; i < notifications_count; i++) {
      growl_tcp_write(sock, "Notification-Name: %s", notifications[i]);
      growl_tcp_write(sock, "Notification-Display-Name: %s", notifications[i]);
      growl_tcp_write(sock, "Notification-Enabled: True");
      if (iconid) {
        growl_tcp_write(sock, "Notification-Icon: x-growl-resource://%s", iconid);  
      } else if (icon && icon != "") {
        growl_tcp_write(sock, "Notification-Icon: %s", icon);  
      }
      growl_tcp_write(sock, "%s", "");
    }
  
    if (iconid) {
      growl_tcp_write(sock, "Identifier: %s", iconid);
      growl_tcp_write(sock, "Length: %d", iconsize);
      growl_tcp_write(sock, "%s", "");
  
      while (!feof(iconfile)) {
        size_t bytes_read = fread(buffer, 1, 1024, iconfile);
        if (bytes_read) growl_tcp_write_raw(sock, (const unsigned char *)buffer, bytes_read);
      }
      growl_tcp_write(sock, "%s", "");
    }
    growl_tcp_write(sock, "%s", "");
  
    while (1) {
      char* line = growl_tcp_read(sock);
      if (!line) {
        growl_tcp_close(sock);
        sock = -1;
        break;
      } else {
        int len = strlen(line);
        /* fprintf(stderr, "%s\n", line); */
        if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0 && strncmp(line + 15, " NONE", 5) != 0) {
          fprintf(stderr, "failed to register notification\n");
          free(line);
          break;
        }
        free(line);
        if (len == 0) break;
      }
    }
    growl_tcp_close(sock);
    sock = 0;
  
  }
  
  if (iconfile) fclose(iconfile);
  if (iconid) free(iconid);
  free(authheader);
  
  return (sock == 0) ? 0 : -1;
}


GROWL_EXPORT
int growl_tcp_notify(
    const char *const server,
    const char *const appname,
    const char *const notify,
    const char *const title,
    const char *const message,
    const char *const password,
    const char* const url,
    const char* const icon) {
  int sock = -1;

  char *authheader = growl_generate_authheader_alloc(password);
  char *iconid = NULL;
  FILE *iconfile = NULL;
  size_t iconsize;
  uint8_t buffer[1024];

  growl_init();

  sock = growl_tcp_open(server);
  if (sock > 0) {
    if (icon && icon != "") 
    {
      size_t bytes_read;
      md5_context md5ctx;
      uint8_t md5tmp[20];
      iconfile = fopen(icon, "rb");
      if (iconfile)
      {
        fseek(iconfile, 0, SEEK_END);
        iconsize = ftell(iconfile);
        fseek(iconfile, 0, SEEK_SET);
        memset(md5tmp, 0, sizeof(md5tmp));
        md5_starts(&md5ctx);
        while (!feof(iconfile)) {
          bytes_read = fread(buffer, 1, 1024, iconfile);
          if (bytes_read) md5_update(&md5ctx, buffer, bytes_read);
        }
        fseek(iconfile, 0, SEEK_SET);
        md5_finish(&md5ctx, md5tmp);
        iconid = string_to_hex_alloc((const char*) md5tmp, 16);
      }
    }
  
    growl_tcp_write(sock, "GNTP/1.0 NOTIFY NONE %s", authheader ? authheader : "");
    growl_tcp_write(sock, "Application-Name: %s", appname);
    growl_tcp_write(sock, "Notification-Name: %s", notify);
    growl_tcp_write(sock, "Notification-Title: %s", title);
    growl_tcp_write(sock, "Notification-Text: %s", message);
    if (iconid) {
      growl_tcp_write(sock, "Notification-Icon: x-growl-resource://%s", iconid);  
    } else if (icon && icon != "") {
      growl_tcp_write(sock, "Notification-Icon: %s", icon);  
    }
    if (url) growl_tcp_write(sock, "Notification-Callback-Target: %s", url  );
  
    if (iconid) {
      growl_tcp_write(sock, "Identifier: %s", iconid);
      growl_tcp_write(sock, "Length: %d", iconsize);
      growl_tcp_write(sock, "%s", "");
      while (!feof(iconfile)) {
        size_t bytes_read = fread(buffer, 1, 1024, iconfile);
        if (bytes_read) growl_tcp_write_raw(sock, (const unsigned char *)buffer, bytes_read);
      }
      growl_tcp_write(sock, "%s", "");
    }
    growl_tcp_write(sock, "%s", "");
  
  
    while (1) 
    {
      char* line = growl_tcp_read(sock);
      if (!line) 
      {
        growl_tcp_close(sock);
        sock = -1;
        break;
      } else {
        int len = strlen(line);
        /* fprintf(stderr, "%s\n", line); */
        if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0 && strncmp(line + 15, " NONE", 5) != 0) {
          fprintf(stderr, "failed to post notification\n");
          free(line);
          break;
        }
        free(line);
        if (len <= 0) break; // FIX: if there invalid character in the stream len = -1
      }
    }
    growl_tcp_close(sock);
    sock = 0;
  }

  if (iconfile) fclose(iconfile);
  if (iconid) free(iconid);
  free(authheader);

  return (sock == 0) ? 0 : -1;
}


GROWL_EXPORT
int
growl(
    const char *const server,
    const char *const appname,
    const char *const notify,
    const char *const title,
    const char *const message,
    const char *const icon,
    const char *const password,
    const char *url) {    

  int rc = growl_tcp_register(
      server, appname, (const char **const)&notify, 1, password, icon);
  if (rc == 0) {
    rc = growl_tcp_notify(server, appname, notify, title,  message, password, url, icon);
  }
  return rc;
}


void
growl_append_md5(
    unsigned char *const data,
    const int data_length,
    const char *const password) {

  md5_context md5ctx;
  char md5tmp[20] = {0};

  md5_starts(&md5ctx);
  md5_update(&md5ctx, (uint8_t*)data, data_length);
  if (password != NULL) {
    md5_update(&md5ctx, (uint8_t*)password, strlen(password));
  }
  md5_finish(&md5ctx, (uint8_t*)md5tmp);

  memcpy(data + data_length, md5tmp, 16);
}


GROWL_EXPORT
int growl_udp_register(
    const char *const server,
    const char *const appname,
    const char **const notifications,
    const int notifications_count,
    const char *const password) {

  int register_header_length = 22+strlen(appname);
  unsigned char *data;
  int pointer = 0;
  int rc = 0;
  int i = 0;

  uint8_t GROWL_PROTOCOL_VERSION  = 1;
  uint8_t GROWL_TYPE_REGISTRATION = 0;

  uint16_t appname_length = ntohs(strlen(appname));
  uint8_t _notifications_count = notifications_count;
  uint8_t default_notifications_count = notifications_count;
  uint8_t j;

  growl_init();

  for(i = 0; i < notifications_count; i++) {
    register_header_length += 3 + strlen(notifications[i]);
  }  
  data = (unsigned char*)calloc(1, register_header_length);
  if (!data) return -1;

  pointer = 0;
  memcpy(data + pointer, &GROWL_PROTOCOL_VERSION, 1);  
  pointer++;
  memcpy(data + pointer, &GROWL_TYPE_REGISTRATION, 1);
  pointer++;
  memcpy(data + pointer, &appname_length, 2);  
  pointer += 2;
  memcpy(data + pointer, &_notifications_count, 1);  
  pointer++;
  memcpy(data + pointer, &default_notifications_count, 1);  
  pointer++;
  sprintf((char*)data + pointer, "%s", appname);
  pointer += strlen(appname);

  for (i = 0; i < notifications_count; i++) {
    uint16_t notify_length = ntohs(strlen(notifications[i]));
    memcpy(data + pointer, &notify_length, 2);    
    pointer +=2;
    sprintf((char*)data + pointer, "%s", notifications[i]);
    pointer += strlen(notifications[i]);
  } 

  for (j = 0; j < notifications_count; j++) {
    memcpy(data + pointer, &j, 1);
    pointer++;
  }

  growl_append_md5(data, pointer, password);
  pointer += 16;

  rc = growl_tcp_datagram(server, data, pointer);
  free(data);
  return rc;
}

GROWL_EXPORT
int growl_udp_notify(
    const char *const server,
    const char *const appname,
    const char *const notify,
    const char *const title,
    const char *const message,
    const char *const password) {

  int notify_header_length = 28 + strlen(appname)+strlen(notify)+strlen(message)+strlen(title);
  unsigned char *data = (unsigned char*)calloc(1, notify_header_length);
  int pointer = 0;
  int rc = 0;

  uint8_t GROWL_PROTOCOL_VERSION  = 1;
  uint8_t GROWL_TYPE_NOTIFICATION = 1;

  uint16_t flags = ntohs(0);
  uint16_t appname_length = ntohs(strlen(appname));
  uint16_t notify_length = ntohs(strlen(notify));
  uint16_t title_length = ntohs(strlen(title));
  uint16_t message_length = ntohs(strlen(message));

  if (!data) return -1;

  growl_init();

  pointer = 0;
  memcpy(data + pointer, &GROWL_PROTOCOL_VERSION, 1);  
  pointer++;
  memcpy(data + pointer, &GROWL_TYPE_NOTIFICATION, 1);
  pointer++;
  memcpy(data + pointer, &flags, 2);
  pointer += 2;
  memcpy(data + pointer, &notify_length, 2);  
  pointer += 2;
  memcpy(data + pointer, &title_length, 2);  
  pointer += 2;
  memcpy(data + pointer, &message_length, 2);  
  pointer += 2;
  memcpy(data + pointer, &appname_length, 2);  
  pointer += 2;
  strcpy((char*)data + pointer, notify);
  pointer += strlen(notify);
  strcpy((char*)data + pointer, title);
  pointer += strlen(title);
  strcpy((char*)data + pointer, message);
  pointer += strlen(message);
  strcpy((char*)data + pointer, appname);
  pointer += strlen(appname);


  growl_append_md5(data, pointer, password);
  pointer += 16;


  rc = growl_tcp_datagram(server, data, pointer);
  free(data);
  return rc;
}


GROWL_EXPORT
int growl_udp(
    const char *const server,
    const char *const appname,
    const char *const notify,
    const char *const title,
    const char *const message,
    const char *const icon,
    const char *const password,
    const char *url) {

  int rc = growl_udp_register(server, appname, (const char **const)&notify, 1, password);
  if (rc == 0) {
    rc = growl_udp_notify(server, appname, notify, title,  message, password);
  }
  return rc;
  (void)icon; (void)url; /* prevent unused warnigns */
}

#ifdef _WIN32

static void
GrowlNotify_impl_(
    HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {

  char* server = "127.0.0.1:23053";
  char* password = NULL;
  char* appname = "gntp-send";
  char* notify = "gntp-send notify";
  char* title = NULL;
  char* message = NULL;
  char* icon = NULL;
  char* url = NULL;
  char* first = strdup(lpszCmdLine);
  char* ptr = first;
  #define SKIP(x)  while (*x && *x != ' ') x++; if (*x == ' ') *x++ = 0;
  server = ptr;  SKIP(ptr);
  appname = ptr; SKIP(ptr);
  notify = ptr;  SKIP(ptr);
  title = ptr;   SKIP(ptr);
  message = ptr; SKIP(ptr);
  icon = ptr;    SKIP(ptr);
  url = ptr;     SKIP(ptr);
  growl(server,appname,notify,title,message,icon,password,url);
  free(first);
}

void
GrowlNotify(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) return;
  GrowlNotify_impl_(hwnd, hinst, lpszCmdLine, nCmdShow);
  WSACleanup();
}
#endif
