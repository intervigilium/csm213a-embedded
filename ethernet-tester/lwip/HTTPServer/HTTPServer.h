#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "TCPConnection.h"
#include "TCPListener.h"
#include "NetServer.h"

#include <map>
#include <set>
#include <list>

#define HTTP_MAX_EMPTYPOLLS 100
#define GET 4
#define POST 5

//extern unsigned int gconnections;

using namespace std;

namespace mbed {

class HTTPServer;
class HTTPHandler;
class HTTPConnection;

/**
 * A simple HASH function to reduce the size of stored Header Fields
 * TODO: Make the Hash case insensetive.
 */
unsigned int hash(unsigned char *str);

/**
 * The Status of an HTTP Request
 * Nedded for HTTPHandler subclasses to define there reults in the HTTPHandler:init Method.
 */
enum HTTPStatus {
  HTTP_OK                          = 200,
  HTTP_BadRequest                  = 400,
  HTTP_Unauthorized                = 401,
  HTTP_Forbidden                   = 403,
  HTTP_NotFound                    = 404,
  HTTP_MethodNotAllowed            = 405,
  HTTP_InternalServerError         = 500,
  HTTP_NotImplemented              = 501,
};

/**
 * The result of a chunk of data used for the HTTPHandler Methodes data and send
 */
enum HTTPHandle {
  /** Execution Succeeded but more data expected. */
  HTTP_Success                     = 600,
  
  /** Running out of memory waiting for memory */
  HTTP_SenderMemory                = 601,

  
  /** Execution Succeeded and no more data expected. */
  HTTP_SuccessEnded                = 700,
  
  /** Execution failed. Close conection*/
  HTTP_Failed                      = 701,
  
  /** This module will deliver the data. */
  HTTP_Deliver                     = 800,
  
  /** This module has add header fields to the request. */
  HTTP_AddFields                   = 801,
};

/**
 * A parent object for a data storage container for all HTTPHandler objects.
 */
class HTTPData {
  public:
    HTTPData() {}
    virtual ~HTTPData() {}
};

/**
 * A HTTPHandler will serve the requested data if there is an object of a 
 * child class from HTTPHandler which is registert to an matching prefix.
 * To see how to implement your own HTTPHandler classes have a look at 
 * HTTPRPC HTTPStaticPage and HTTPFileSystemHandler.
 */
class HTTPHandler {
  public:
    HTTPHandler(const char *prefix) : _prefix(prefix) {};
    virtual ~HTTPHandler() {
      delete _prefix;
    };

  protected:
    /**
     * Register needed header fields by the HTTPServer.
     * Because of memory size the server will throw away all request header fields which are not registert.
     * Register the fields you need in your implementation of this method.
     */
    virtual void reg(HTTPServer *) {};
    
    /**
     * This Method returns if you will deliver the requested page or not.
     * It will only executed if the prefix is matched by the URL. 
     * If you want to add something to the headerfiles use this method and return HTTP_AddFields. See HTTPFields
     * This would be the right method to implement an Auth Handler.
     */
    virtual HTTPHandle action(HTTPConnection *) const                    {return HTTP_Deliver;}
    
    /**
     * If action returned HTTP_Deliver. 
     * This function will be executed and it means your handler will be deliver the requested data.
     * In this method is the right place to allocate the needed space for your request data and to prepare the sended Header.
     */
    virtual HTTPStatus init(HTTPConnection *) const                      {return HTTP_NotFound;}
    
    /**
     * If data from a post request is arrived for an request you accepted this function will be executed with the data.
     * @param data A pointer to the received data.
     * @param len The length of the received data.
     * @return Return an HTTPHandle. For example HTTP_SuccessEnded if you received all your needed data and want to close the conection (normally not the case).
     */
    virtual HTTPHandle data(HTTPConnection *, void *data, int len) const {return HTTP_SuccessEnded;}
    
    /**
     * If tere is new space in the sendbuffer this function is executed. You can send maximal Bytes of data.
     * @return Return an HTTPHandle. For example HTTP_SuccessEnded if you send out all your data and you want to close the connection.
     */
    virtual HTTPHandle send(HTTPConnection *, int) const                 {return HTTP_SuccessEnded;}
    
    /**
     * returns the Prefix from the HTTPHandler
     */
    const char *getPrefix() const                                        {return _prefix;}

    const char *_prefix;
    
  friend class HTTPServer;
  friend class HTTPConnection;
};

/**
 * For every incomming connection we have a HTTPConnection object which will handle the requests of this connection.
 */
class HTTPConnection : public TCPConnection {
  public:
    /**
     * Constructs a new connection object from a server.
     * It just need the server object to contact the handlers
     * and the tcp connection pcb.
     * @param parent The server which created the connection.
     * @param pcb The pcb object NetServers internal representation of an TCP Connection
     */
    HTTPConnection(HTTPServer *parent, struct tcp_pcb *pcb);
    /**
     * Default destructor. Simple cleanup.
     */
    virtual ~HTTPConnection();

    /**
     * Get the requested url.
     * Only set if a request ist received.
     */
    char *getURL() const                  { return _request_url; }
    
    /**
     * Gets a string of set fields to send with the answere header.
     */
    const char *getHeaderFields() const   { return (_request_headerfields)?_request_headerfields:""; }
    
    /**
     * Gets the length of the anwere in bytes. This is requiered for the HTTP Header.
     * It should be set over setLength by an HTTPHandler in the init() method.
     */
    const int  &getLength() const         { return _request_length; }
    
    /**
     * Gets POST or GET or 0 depends on wether ther is a request and what type is requested.
     */
    const char &getType() const           { return _request_type; }
    
    /**
     * Gets a value from a header field of the request header.
     * But you must have registerd this headerfield by the HTTPServer before.
     * Use the HTTPHandler::reg() method for the registration of important header fields for your Handler.
     */
    const char *getField(char *key) const;
    
    /**
     * For internal usage. Adds an header field value to its hash.
     * If it was registered You can see the Value with the getField method
     */
    void addField(char *key, char *value);
    
    /**
     * Sets the result length for an request shoud be set in an HTTPHandler.init() call.
     * This Value will be send with the response header before the first chunk of data is send.
     */
    void setLength(const int &value)      { _request_length = value; }
    
    /**
     * Set the response header field to a value.
     * Should be used in the HTTPHandler::init() method.
     * For example if you want to set caching methods.
     */
    void setHeaderFields(char *value)     { _request_headerfields = value; }
    
    /** Indicates that if a request is received the header is incomplete until now. */
    bool request_incomplete;
    
    /** If an request is complete HTTPHandler:init will be called and can store here its connection data. */
    HTTPData *data;
    
    /** The handler which handles the current request. Depends on the prefix of the URL. */
    HTTPHandler *request_handler;
    
    /** The status of the request. Will be set as result of HTTPHandler::init. */
    HTTPStatus request_status;

    /** The HTTTPServer which created this connection. */
    HTTPServer *parent;
  private:
    virtual void err(err_t err);
    virtual err_t poll();
    virtual err_t sent(u16_t len);
    virtual err_t recv(struct pbuf *q, err_t err);

    /** We will not make any DNS requests. */
    virtual void dnsreply(const char *, struct ip_addr *) {}
    
    /** If a request is finished it will be deleted with this method. Simple cleanup. */
    void deleteRequest();

    /** Call the handler to send the next chunk of data. */
    void send();
    
    /** Call the handler if we received new data. */
    void store(void *d, struct pbuf *p);
    
    /** 
     * If a request header is not complete we can colect needed header fields. 
     * This happens in here.
     */
    void getFields(struct pbuf **q, char **d);

    char *_request_url;
    char  _request_type;
    char *_request_headerfields;
    map<unsigned int, char *> _request_fields;
    int _request_length;
    char *_request_arg_key;
    char *_request_arg_value;
    char  _request_arg_state;
    
    unsigned int emptypolls; // Last time for timeout
    unsigned int _timeout_max;
};

/* Class HTTPServer
 *  An object of this class is an HTTPServer instance and will anwere requests on a given port.
 *  It will deliver HTTP pages
 */
class HTTPServer : public TCPListener {
  public:

    /* Constructor: HTTPServer
     *  Creates an HTTPServer object. You might want to initialise the network server befor.
     *  If you dont do it it will be happen by the first post or get request you make.
     *
     *  To initialize the network server on creation of the HTTPServer object it's possible to parse some arguments:
     * Variables:
     *  hostname - A host name for the device. Might take a while to appear in the network, 
     *             depends on the network infrastructure. Furthermore in most cases you have 
     *             to add your domainname after the host name to address the device.
     *             Default is NULL.
     *  ip  - The device ipaddress or ip_addr_any for dhcp. Default is ip_addr_any
     *  nm  - The device netmask or ip_addr_any for dhcp. Default is ip_addr_any.
     *  gw  - The device gateway or ip_addr_any for dhcp. Default is ip_addr_any.
     *  dns - The device first dns server ip or ip_addr_any for dhcp. Default is ip_addr_any.
     *
     * Example:
     *  > HTTPServer http;         // Simple DHCP, brings up the TCP/IP stack on bind(). Default prot is port 80.
     *  > HTTPServer http(8080);   // Port is here 8080.
     *  
     *  > HTTPServer http("worf"); // Brings up the device with DHCP and sets the host name "worf"
     *  >                          // The device will be available under worf.<your local domain> 
     *  >                          // for example worf.1-2-3-4.dynamic.sky.com
     *
     *  > HTTPServer http("wolf",              // Brings up the device with static IP address and domain name.
     *  >                 IPv4(192,168,0,44),  // IPv4 is a helper function which allows to rtype ipaddresses direct
     *  >                 IPv4(255,255,255,0), // as numbers in C++.
     *  >                 IPv4(192,168,0,1),   // the device address is set to 192.168.0.44, netmask 255.255.255.0
     *  >                 IPv4(192,168,0,1)    // default gateway is 192.168.0.1 and dns to 192.168.0.1 as well.
     *  >                 8080);               // And port is on 8080. Default port is 80.
     */

    HTTPServer(const char *hostname, struct ip_addr ip = ip_addr_any, struct ip_addr nm = ip_addr_any, struct ip_addr gw = ip_addr_any, struct ip_addr dns = ip_addr_any, unsigned short port = 80);
    HTTPServer(unsigned short port = 80);

    /* Destructor: ~HTTPServer
     *  Destroys the HTTPServer and all open connections.
     */
    virtual ~HTTPServer() {
      fields.clear();
      _handler.clear();
    }

    /* Function: addHandler
     *  Add a new content handler to handle requests.
     *  Content handler are URL prefix specific.
     *  Have a look at HTTPRPC and HTTPFileSystemHandler for examples.
     */
    virtual void addHandler(HTTPHandler *handler) {
      _handler.push_back(handler);
      handler->reg(this);
    }
    
    /* Function registerField
     *  Register needed header fields to filter from a request header.
     *  Should be called from HTTPHandler::reg()
     */
    virtual void registerField(char *name) {
      fields.insert(hash((unsigned char *)name));
    }

    /* Function isField
     *  A short lookup if the headerfield is registerd.
     */
    virtual bool isField(unsigned long h) const {
      return fields.find(h) != fields.end();
    }
    
    /* Function: poll 
     *   You have to call this method at least every 250ms to let the http server run.
     *   But I would recomend to call this function as fast as possible.
     *   This function is directly coupled to the answere time of your HTTPServer instance.
     */
    inline static void poll() {
      NetServer::poll();
    }

    /* Function: timeout
     *  Sets the timout for a HTTP request. 
     *  The timout is the time wich is allowed to spent between two incomming TCP packets. 
     *  If the time is passed the connection will be closed.
     */
    void timeout(int value) {
      _timeout_max = value;
    }

    /* Function timeout
     *  Returns the timout to use it in HTTPHandlers and HTTPConnections
     */
    int timeout() {
      return _timeout_max;
    }
  private:
    /**
     * Pick up the right handler to deliver the response.
     */
    virtual HTTPHandler *handle(HTTPConnection *con) const {
      for(list<HTTPHandler *>::const_iterator iter = _handler.begin();
          iter != _handler.end(); iter++) {
        if(strncmp((*iter)->getPrefix(), con->getURL(), strlen((*iter)->getPrefix()))==0) {
          HTTPHandler *handler = *iter;
          if(handler->action(con)==HTTP_Deliver) {
            return *iter;
          }
        }
      }
      return NULL;
    }

    /**
     * Accept an incomming connection and fork a HTTPConnection if we have enought memory.
     */
    virtual err_t accept(struct tcp_pcb *pcb, err_t err) {
      LWIP_UNUSED_ARG(err);
      HTTPConnection *con = new HTTPConnection(this, pcb);
//      printf("New Connection opend. Now are %u connections open\n", ++gconnections);
      if(con == NULL) {
        printf("http_accept: Out of memory\n");
        return ERR_MEM;
      }
      con->set_poll_interval(1);
      tcp_setprio(pcb, TCP_PRIO_MIN);
      return ERR_OK;
    }

    /** The registerd request header fields */
    set<unsigned int> fields;
    
    /** A List of all registered handler. */
    list<HTTPHandler *> _handler;

    int _timeout_max;

  friend class HTTPConnection;
};

};

#include "HTTPRPC.h"
#include "HTTPFS.h"
#include "HTTPFields.h"

#endif /* HTTP_H */
