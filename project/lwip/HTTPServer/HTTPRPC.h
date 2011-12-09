#ifndef HTTPRPC_H
#define HTTPRPC_H

#include "HTTPServer.h"
#include "platform.h"
#ifdef MBED_RPC
#include "rpc.h"

/**
 * A datastorage helper for the HTTPRPC class
 */
class HTTPRPCData : public HTTPData {
  public: char result[255];
};

/**
 * Thsi class enables you to make rpc calls to your mbed.
 * Furthermore it is a good example how to write a HTTPHandler for small data chunks.
 */
class HTTPRPC : public HTTPHandler {
  public:
    /**
     * We have to know the prefix for the RPCHandler.
     * A good default choice is /rpc so we made this default.
     */
    HTTPRPC(const char *path = "/rpc") : HTTPHandler(path) {}
    HTTPRPC(HTTPServer *server, const char *path = "/rpc") : HTTPHandler(path) { server->addHandler(this); }
    
  private:
    /**
     * If you need some Headerfeelds you have tor register the feelds here.
     */
//    virtual void reg(HTTPServer *svr) {
//      svr->registerField("Content-Length");
//    }

    /**
     * If a new RPCRequest Header is complete received the server will call this function.
     * This is the right place for preparing our datastructures.
     * Furthermore we will execute the rpc call and store the anwere.
     * But we will not send a response. This will be hapen in the send method.
     */
    virtual HTTPStatus init(HTTPConnection *con) const {
      HTTPRPCData *data = new HTTPRPCData();
      con->data = data;
      char *query = con->getURL()+strlen(_prefix);
      clean(query);
      rpc(query, data->result);

      const char *nfields = "Cache-Control: no-cache, no-store, must-revalidate\r\nPragma: no-cache\r\nExpires: Thu, 01 Dec 1994 16:00:00 GM";
      char *old = (char *)con->getHeaderFields();
      int oldlen = strlen(old);
      int atrlen = strlen(nfields);
      char *fields = new char[atrlen+oldlen+3];
      strcpy(fields,old);
      fields[oldlen+0] = '\r';
      fields[oldlen+1] = '\n';
      strcpy(&fields[oldlen+2], nfields);
      fields[atrlen+2+oldlen] = '\0';
      con->setHeaderFields(fields);
      if(*old) {
        delete old;
      }

      con->setLength(strlen(data->result));
      return HTTP_OK;
    }

    /**
     * If we got an POST request the send method will not be executed.
     * If we want to send data we have to trigger it the first time by ourself.
     * So we execute the send method.
     *
     * If the rpc call is the content of the POST body we would not be able to execute it.
     * Were parsing only the URL.
     */
    virtual HTTPHandle data(HTTPConnection *con, void *, int) const {
      return send(con, con->sndbuf());
    }

    /**
     * Send the result back to the client.
     * If we have not enought space wait for next time.
     */
    virtual HTTPHandle send(HTTPConnection *con, int maximum) const {
      HTTPRPCData *data = static_cast<HTTPRPCData *>(con->data);    
      if(maximum>64) {
        con->write(data->result, con->getLength());
        return HTTP_SuccessEnded;
      } else {
        // To less memory.
        return HTTP_SenderMemory;
      }
    }

    /**
     * To reduce memory usage we sodify the URL directly 
     * and replace '%20',',','+','=' with spaces.
     */
    inline void clean(char *str) const {
      while(*str++) {
        if(*str=='%'&&*(str+1)=='2'&&*(str+2)=='0') {
          *str = ' ';
          *(str+1) = ' ';
          *(str+2) = ' ';
        }
        if(*str==','||*str=='+'||*str=='=') {
          *str = ' ';
        }
      }
    }
};
#endif

#endif
