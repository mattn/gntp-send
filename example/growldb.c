#include <stdlib.h>
#include <sqlite3ext.h>
#include <growl.h>

SQLITE_EXTENSION_INIT1
static void
growl_func(sqlite3_context *context, int argc, sqlite3_value **argv) {
  if (argc == 1) {
    const char *text  = (const char *)sqlite3_value_text(argv[0]);
    growl("localhost", "sqlite3", "sqlite3-trigger", "database-update", text, NULL, NULL, NULL);
  }
}
__declspec(dllexport) int
sqlite3_extension_init(sqlite3 *db, char **errmsg, const sqlite3_api_routines *api) {
  SQLITE_EXTENSION_INIT2(api);
  return sqlite3_create_function(db, "growl", 1, SQLITE_UTF8, (void*)db, growl_func, NULL, NULL);
}
