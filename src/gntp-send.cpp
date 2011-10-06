#include <memory>
#include <growl.hpp>

int main(int argc, char **argv) {
  static_cast<void>(argc); static_cast<void>(argv); // prevent unused warnings
  const char *n[2] = { "alice" , "bob" };
  std::auto_ptr<Growl> growl(
      new Growl(GROWL_TCP,0,"gntp_send++",(const char **const)n,2));
  growl->Notify("bob","title","message");
}
