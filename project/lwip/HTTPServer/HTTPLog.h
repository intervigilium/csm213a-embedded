#ifndef HTTPLOG_H
#define HTTPLOG_H

#include "HTTPServer.h"

class HTTPLog : public HTTPHandler {
  public:
    HTTPLog(const char *prefix) : HTTPHandler(prefix) {}
    HTTPLog(HTTPServer *server, const char *prefix) : HTTPHandler(prefix) { server->addHandler(this); }

  private:
    virtual HTTPHandle action(HTTPConnection *con) const {
      struct ip_addr ip = con->pcb()->remote_ip;
      printf("%hhu.%hhu.%hhu.%hhu %s %s", (ip.addr)&0xFF, (ip.addr>>8)&0xFF, (ip.addr>>16)&0xFF, (ip.addr>>24)&0xFF, (con->getType() == POST? "POST" : "GET"), con->getURL());
      return HTTP_AddFields;
    }
};

#endif
