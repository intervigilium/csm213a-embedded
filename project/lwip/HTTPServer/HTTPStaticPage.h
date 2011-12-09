#ifndef HTTPSTATICPAGE_H
#define HTTPSTATICPAGE_H

#include "HTTPServer.h"

/**
 * A datastorage helper for the HTTPStaticPage class.
 * Stores only the left upload size.
 */
class HTTPStaticPageData : public HTTPData {
  public: int left;
};

/**
 * This class Provide a Handler to send Static HTTP Pages from the bin file.
 */
class HTTPStaticPage : public HTTPHandler {
  public:
    /**
     * Constructer takes the pagename and the pagedata.
     * As long as the pagedata is NULL terminated you have not to tell the data length.
     * But if you want to send binary data you should tell us the size.
     */
    HTTPStaticPage(const char *path, const char *page, int length = 0)
     : HTTPHandler(path), _page(page) {
      _size = (length)?length:strlen(_page);
    }

    HTTPStaticPage(HTTPServer *server, const char *path, const char *page, int length = 0)
     : HTTPHandler(path), _page(page) {
      _size = (length)?length:strlen(_page);
      server->addHandler(this);
    }

  private:
    /**
     * A this Static Page is requested!
     * Prepare a datastorage helper "HTTPStaticPageHelper" to store the left data size.
     * And return HTTP_OK
     */
    virtual HTTPStatus init(HTTPConnection *con) const {
      HTTPStaticPageData *data = new HTTPStaticPageData();
      con->data = data;
      data->left = _size;
      con->setLength(_size);
      return HTTP_OK;
    }

    /**
     * Send the maximum data out to the client. 
     * If the file is complete transmitted close connection by returning HTTP_SuccessEnded
     */
    virtual HTTPHandle send(HTTPConnection *con, int maximum) const {
      HTTPStaticPageData *data = static_cast<HTTPStaticPageData *>(con->data);
      int len = min(maximum, data->left);
      err_t err;

      do {
        err = con->write((void*)&_page[_size - data->left], len, 1);
        if(err == ERR_MEM) {
          len >>= 1;
        }
      } while(err == ERR_MEM && len > 1);
      if(err == ERR_OK) {
        data->left -= len;
      }

      return (data->left)? HTTP_Success : HTTP_SuccessEnded;
    }

    /** Pointer to the page data */
    const char *_page;
    
    /** page data size*/
    int   _size;
};

#endif
