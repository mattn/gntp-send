#ifdef _WIN32
#include <windows.h>
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
  char* auth_header = NULL;

  if (password && *password) {
    char *salt = gen_salt_alloc(8);
    if (salt) {
      char *keyhash = gen_password_hash_alloc(password, salt);
      if (keyhash) {
        char* salthash = string_to_hex_alloc(salt, 8);
        if (salthash) {
          auth_header = (char*)malloc(strlen(keyhash) + strlen(salthash) + 7);
          if (auth_header) {
            sprintf(auth_header, " MD5:%s.%s", keyhash, salthash);
          }
          free(salthash);
        }
        free(keyhash);
      }
      free(salt);
    }
  }

  return auth_header;
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
  char *auth_header;
  char *icon_id = NULL;
  FILE *icon_file = NULL;
  long icon_size = 0;
  uint8_t buffer[1024];

  growl_init();
  auth_header = growl_generate_authheader_alloc(password);
  sock = growl_tcp_open(server);
  if (sock == -1) goto leave;

  if (icon) {
    size_t bytes_read;
    md5_context md5ctx;
    uint8_t md5tmp[20];
    icon_file = fopen(icon, "rb");
    if (icon_file) {
      fseek(icon_file, 0, SEEK_END);
      icon_size = ftell(icon_file);
      fseek(icon_file, 0, SEEK_SET);
      memset(md5tmp, 0, sizeof(md5tmp));
      md5_starts(&md5ctx);
      while (!feof(icon_file)) {
        bytes_read = fread(buffer, 1, 1024, icon_file);
        if (bytes_read) md5_update(&md5ctx, buffer, bytes_read);
      }
      fseek(icon_file, 0, SEEK_SET);
      md5_finish(&md5ctx, md5tmp);
      icon_id = string_to_hex_alloc((const char*) md5tmp, 16);
    }
  }

  growl_tcp_write(sock, "GNTP/1.0 REGISTER NONE %s", auth_header ? auth_header : "");
  growl_tcp_write(sock, "Application-Name: %s", appname);
  if (icon_id) {
    growl_tcp_write(sock, "Application-Icon: x-growl-resource://%s", icon_id);
  } else if (icon) {
    growl_tcp_write(sock, "Application-Icon: %s", icon);
  }
  growl_tcp_write(sock, "Notifications-Count: %d", notifications_count);
  growl_tcp_write(sock, "%s", "");

  for (i = 0; i < notifications_count; i++) {
    growl_tcp_write(sock, "Notification-Name: %s", notifications[i]);
    growl_tcp_write(sock, "Notification-Display-Name: %s", notifications[i]);
    growl_tcp_write(sock, "Notification-Enabled: True");
    if (icon_id) {
      growl_tcp_write(sock, "Notification-Icon: x-growl-resource://%s", icon_id);
    } else if (icon) {
      growl_tcp_write(sock, "Notification-Icon: %s", icon);
    }
    growl_tcp_write(sock, "%s", "");
  }

  if (icon_id) {
    growl_tcp_write(sock, "Identifier: %s", icon_id);
    growl_tcp_write(sock, "Length: %ld", icon_size);
    growl_tcp_write(sock, "%s", "");

    while (!feof(icon_file)) {
      size_t bytes_read = fread(buffer, 1, 1024, icon_file);
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
      goto leave;
    } else {
      int len = strlen(line);
      /* fprintf(stderr, "%s\n", line); */
      if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0) {
        if (strncmp(line + 15, " NONE", 5) != 0) {
          fprintf(stderr, "failed to register notification\n");
          free(line);
          goto leave;
        }
      }
      free(line);
      if (len == 0) break;
    }
  }
  growl_tcp_close(sock);
  sock = 0;

leave:
  if (icon_file) fclose(icon_file);
  if (icon_id) free(icon_id);
  free(auth_header);

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

  char *auth_header = growl_generate_authheader_alloc(password);
  char *icon_id = NULL;
  FILE *icon_file = NULL;
  long icon_size = 0;
  uint8_t buffer[1024];

  growl_init();

  sock = growl_tcp_open(server);
  if (sock == -1) goto leave;

  if (icon) {
    size_t bytes_read;
    md5_context md5ctx;
    uint8_t md5tmp[20];
    icon_file = fopen(icon, "rb");
    if (icon_file) {
      fseek(icon_file, 0, SEEK_END);
      icon_size = ftell(icon_file);
      fseek(icon_file, 0, SEEK_SET);
      memset(md5tmp, 0, sizeof(md5tmp));
      md5_starts(&md5ctx);
      while (!feof(icon_file)) {
        bytes_read = fread(buffer, 1, 1024, icon_file);
        if (bytes_read) md5_update(&md5ctx, buffer, bytes_read);
      }
      fseek(icon_file, 0, SEEK_SET);
      md5_finish(&md5ctx, md5tmp);
      icon_id = string_to_hex_alloc((const char*) md5tmp, 16);
    }
  }

  growl_tcp_write(sock, "GNTP/1.0 NOTIFY NONE %s", auth_header ? auth_header : "");
  growl_tcp_write(sock, "Application-Name: %s", appname);
  growl_tcp_write(sock, "Notification-Name: %s", notify);
  growl_tcp_write(sock, "Notification-Title: %s", title);
  growl_tcp_write(sock, "Notification-Text: %s", message);
  if (icon_id) {
    growl_tcp_write(sock, "Notification-Icon: x-growl-resource://%s", icon_id);
  } else if (icon) {
    growl_tcp_write(sock, "Notification-Icon: %s", icon);
  }
  if (url) growl_tcp_write(sock, "Notification-Callback-Target: %s", url);

  if (icon_id) {
    growl_tcp_write(sock, "%s", "");
    growl_tcp_write(sock, "Identifier: %s", icon_id);
    growl_tcp_write(sock, "Length: %ld", icon_size);
    growl_tcp_write(sock, "%s", "");
    while (!feof(icon_file)) {
      size_t bytes_read = fread(buffer, 1, 1024, icon_file);
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
      goto leave;
    } else {
      int len = strlen(line);
      /* fprintf(stderr, "%s\n", line); */
      if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0) {
        if (strncmp(line + 15, " NONE", 5) != 0) {
          fprintf(stderr, "failed to post notification\n");
          free(line);
          goto leave;
        }
      }
      free(line);
      if (len == 0) break;
    }
  }
  growl_tcp_close(sock);
  sock = 0;

leave:
  if (icon_file) fclose(icon_file);
  if (icon_id) free(icon_id);
  free(auth_header);

  return (sock == 0) ? 0 : -1;
}


GROWL_EXPORT
int growl_tcp_notify_with_data(
    const char *const server,
    const char *const appname,
    const char *const notify,
    const char *const title,
    const char *const message,
    const char *const password,
    const char* const url,
    const unsigned char* const icon_data,
    const long icon_size) {
  int sock = -1;

  char *auth_header = growl_generate_authheader_alloc(password);
  char *icon_id = NULL;

  growl_init();

  sock = growl_tcp_open(server);
  if (sock == -1) goto leave;

  if (icon_data) {
    md5_context md5ctx;
    uint8_t md5tmp[20];
    memset(md5tmp, 0, sizeof(md5tmp));
    md5_starts(&md5ctx);
    md5_update(&md5ctx, icon_data, icon_size);
    md5_finish(&md5ctx, md5tmp);
    icon_id = string_to_hex_alloc((const char*) md5tmp, 16);
  }

  growl_tcp_write(sock, "GNTP/1.0 NOTIFY NONE %s", auth_header ? auth_header : "");
  growl_tcp_write(sock, "Application-Name: %s", appname);
  growl_tcp_write(sock, "Notification-Name: %s", notify);
  growl_tcp_write(sock, "Notification-Title: %s", title);
  growl_tcp_write(sock, "Notification-Text: %s", message);
  if (url) growl_tcp_write(sock, "Notification-Callback-Target: %s", url);
  if (icon_id) {
    long rest = icon_size;
    unsigned char *ptr = (unsigned char *) icon_data;
    growl_tcp_write(sock, "Notification-Icon: x-growl-resource://%s", icon_id);
    growl_tcp_write(sock, "%s", "");
    growl_tcp_write(sock, "Identifier: %s", icon_id);
    growl_tcp_write(sock, "Length: %ld", icon_size);
    growl_tcp_write(sock, "%s", "");
    while (rest > 0) {
      long send_size = rest > 1024 ? 1024 : rest;
      growl_tcp_write_raw(sock, (const unsigned char *)ptr, send_size);
      ptr += send_size;
      rest -= send_size;
    }
    growl_tcp_write(sock, "%s", "");
  }
  growl_tcp_write(sock, "%s", "");

  while (1) {
    char* line = growl_tcp_read(sock);
    if (!line) {
      growl_tcp_close(sock);
      sock = -1;
      goto leave;
    } else {
      int len = strlen(line);
      /* fprintf(stderr, "%s\n", line); */
      if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0) {
        if (strncmp(line + 15, " NONE", 5) != 0) {
          fprintf(stderr, "failed to post notification\n");
          free(line);
          goto leave;
        }
      }
      free(line);
      if (len == 0) break;
    }
  }
  growl_tcp_close(sock);
  sock = 0;

leave:
  if (icon_id) free(icon_id);
  free(auth_header);

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
  if (password && *password) {
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

/* vim:set et sw=2 ts=2 ai: */
