#ifndef HTTPATTRIBUTE_H
#define HTTPATTRIBUTE_H

#include "HTTPServer.h"

/**
 * A simple HTTPHandler which will add new fields to the httpresult header.
 * It can be used for adding caching strtegies.
 */
class HTTPFields : public HTTPHandler {
  public:
    /**
     * Create new HTTPFields
     * @param prefix The prefix path on witch we will execute the Handler. Means add the fields.
     * @param fields The Fields wicht are added to all Handlers with the same prefix and which are added after this one.
     */
    HTTPFields(const char *prefix, const char *fields) : HTTPHandler(prefix) { _fields = fields; }
    HTTPFields(HTTPServer *server, const char *prefix, const char *fields) : HTTPHandler(prefix) { _fields = fields; server->addHandler(this); }

  private:
    /**
     * The Action methon should work on a Connection.
     * If the result is HTTP_AddFields the Server will know that we modified the connection header.
     * If the result is HTTP_Deliver the server will use this object to anwere the request.
     *
     * In this case we add new fields to the header.
     */
    virtual HTTPHandle action(HTTPConnection *con) const {
      char *old = (char *)con->getHeaderFields();
      int oldlen = strlen(old);
      int atrlen = strlen(_fields);
      char *fields = new char[atrlen+oldlen+3];
      strcpy(fields,old);
      fields[oldlen+0] = '\r';
      fields[oldlen+1] = '\n';
      strcpy(&fields[oldlen+2], _fields);
      fields[atrlen+2+oldlen] = '\0';
      con->setHeaderFields(fields);
      if(*old) {
        delete old;
      }
      return HTTP_AddFields;
    }

    const char *_fields;
};

#endif

