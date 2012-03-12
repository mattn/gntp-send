#include <growl.hpp>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Growl::Growl(
    const Growl_Protocol _protocol,
    const char *const _password,
    const char *const _application,
    const char **_notifications,
    const int _notifications_count)
      : server(strdup("localhost")),
        password(_password ? strdup(_password) : NULL),
        protocol(_protocol), application(strdup(_application)) {

  Register(_notifications, _notifications_count);
}

Growl::Growl(
    const Growl_Protocol _protocol,
    const char *const _server,
    const char *const _password,
    const char *const _application,
    const char **_notifications,
    const int _notifications_count)
      : server(strdup(_server)),
        password(_password ? strdup(_password) : NULL),
        protocol(_protocol), application(strdup(_application)) {

  Register(_notifications, _notifications_count);
}

void
Growl::Register(
    const char **const notifications,
    const int notifications_count,
    const char *const icon) {

  if (protocol == GROWL_TCP) {
    growl_tcp_register(
        server,
        application,
        notifications,
        notifications_count,
        password,
        icon);
  } else if (protocol == GROWL_UDP) {
    growl_udp_register(
        server,
        application,
        notifications,
        notifications_count,
        password);
  }
}

Growl::~Growl() {
  free(server);
  free(password);
  free(application);
}

void
Growl::Notify(
    const char *const notification,
    const char *const title,
    const char* const message) {

  Notify(notification, title, message, NULL, NULL);
}


void
Growl::Notify(
    const char *const notification,
    const char *const title,
    const char* const message,
    const char *const url,
    const char *const icon) {

  if (protocol == GROWL_TCP) {
    growl_tcp_notify(
        server,
        application,
        notification,
        title,
        message,
        password,
        url,
        icon);
  } else if (protocol == GROWL_UDP) {
    growl_udp_notify(
        server,
        application,
        notification,
        title,
        message,
        password);
  }
}

void
Growl::Notify(
    const char *const notification,
    const char *const title,
    const char* const message,
    const char *const url,
    const unsigned char *const icon_data,
    const long icon_size) {

  if (protocol == GROWL_TCP) {
    growl_tcp_notify_with_data(
        server,
        application,
        notification,
        title,
        message,
        password,
        url,
        icon_data,
        icon_size);
  } else if (protocol == GROWL_UDP) {
    growl_udp_notify(
        server,
        application,
        notification,
        title,
        message,
        password);
  }
}

/* vim:set et sw=2 ts=2 ai: */
