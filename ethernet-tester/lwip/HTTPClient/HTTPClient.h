#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include "TCPConnection.h"
#include "NetServer.h"
#include "iputil.h"

/* Class: HTTPClient
 *  A simple Class to fetch HTTP Pages.
 */
class HTTPClient : public TCPConnection {
  public:
    /* Constructor: HTTPClient
     *  Creates an HTTPClient object. You might want to initialise the network server befor.
     *  If you dont do it it will be happen by the first post or get request you make.
     *
     *  To initialize the network server on creation of the HTTPClient object it's possible to parse some arguments.
     *
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
     *  > HTTPClient http;         // Simple DHCP, brings up the TCP/IP stack on first get/post reguest.
     *  
     *  > HTTPClient http("worf"); // Brings up the device with DHCP and sets the host name "worf"
     *  >                          // The device will be available under worf.<your local domain> 
     *  >                          // for example worf.1-2-3-4.dynamic.sky.com
     *
     *  > HTTPClient http("wolf",              // Brings up the device with static IP address and domain name.
     *  >                 IPv4(192,168,0,44),  // IPv4 is a helper function which allows to rtype ipaddresses direct
     *  >                 IPv4(255,255,255,0), // as numbers in C++.
     *  >                 IPv4(192,168,0,1),   // the device address is set to 192.168.0.44, netmask 255.255.255.0
     *  >                 IPv4(192,168,0,1));  // default gateway is 192.168.0.1 and dns to 192.168.0.1 as well.
     *  >
     */
    HTTPClient(const char *hostname = NULL, struct ip_addr ip = ip_addr_any, struct ip_addr nm = ip_addr_any, struct ip_addr gw = ip_addr_any, struct ip_addr dns = ip_addr_any);
   
    /* Destructor: ~HTTPClient
     * Destroys the HTTPClient class.
     */
    virtual ~HTTPClient() {
      if(_auth) {
        delete _auth;
        _auth = NULL;
      }
    }
   
    /* Function: headers
     *  Add header additional Information to the next post or get requests.
     *  Somtimes it is useful to add further header information. For example an auth field.
     *  Each time you call this function it will be replace the header fields given by an 
     *  prior call.
     *
     *  It will not free your data.
     * Variables:
     *  fields - A string containing all fields you want to add. Seperated by "\\r\\n".
     *
     */
    void headers(const char *fields);

    /* Function: auth
     *  Enables basic authentication. Just enter username and password 
     *  and they will be used for all requests.
     *  If you want to lean your username and passwort just insert NULL, NULL.
     *
     * Variables:
     *  user - Username for ure auth or NULL.
     *  password - Password for auth or NULL.
     */
    void auth(const char *user, const char *password);

    /* Function: get
     *  A simple get-request just insert the url.
     *  But if you want you can get the result back as a string.
     *  Sometimes you want get a large result, more than 64 Bytes
     *  than define your size.
     *
     * Variables:
     * url     - The requested URL.
     * result  - The answere to your request, by default you have not to take it. But if you want it, it has a default length from 64 Bytes.
     * rsize   - The maximum size of your result. By default 64 Bytes.
     * returns - The length of your demanted result or 1 if the request is succssesful or 0 if it failed. But it might be 0 too wether your result has 0 in length.
     */
    unsigned int get(const char *url, char *result = NULL, int rsize = 64);

    /* Function: get
     *  A simple get-request just insert the url and a FILE Pointer.
     *  This get request will save yor result to an file. Very helpful if you demat a big bunch of data.
     *
     * Variables:
     *  url     - The requested URL.
     *  result  - The FILE Pointer in which you want to store the result.
     *  returns - The length of your demanted result or 1 if the request is succssesful or 0 if it failed. But it might be 0 too wether your result has 0 in length.
     */
    unsigned int get(const char *url, FILE *result);

    /* Function: post
     *  A simple post-request just insert the url.
     *  You can send data if you want but they should be NULL-Terminated.
     *  If you want you can get the result back as a string.
     *  Sometimes you want get a large result, more than 64 Bytes
     *  than define your size.
     *
     * Variables:
     *  url     - The requested URL.
     *  data    - A char array of the data you might want to send.
     *  result  - The answere to your request, by default you have not to take it. But if you want it, it has a default length from 64 Bytes.
     *  rsize   - The maximum size of your result. By default 64 Bytes.
     *  returns - The length of your demanted result or 1 if the request is succssesful or 0 if it failed. But it might be 0 too wether your result has 0 in length.
     */
    unsigned int post(const char *url, const char *data = NULL, char *result = NULL, int rsize = 64);

    /* Function: post
     *  A simple get-request just insert the url and a FILE Pointer.
     *  You can send data if you want but they should be NULL-Terminated.
     *  This get request will save yor result to an file. Very helpful if you demat a big bunch of data.
     *
     * Variables:
     *  url     - The requested URL.
     *  data    - A char array of the data you might want to send.
     *  result  - The FILE Pointer in which you want to store the result.
     *  returns - The length of your demanted result or 1 if the request is succssesful or 0 if it failed. But it might be 0 too wether your result has 0 in length.
     */
    unsigned int post(const char *url, const char *data, FILE *result);

    /* Function: post
     *  A simple get-request just insert the url and a two FILE Pointers to send the content of the file out and store you results.
     *  Your data to sent can come from a file.
     *  This get request will save yor result to an file. Very helpful if you demat a big bunch of data.
     *
     * Variables:
     *  url     - The requested URL.
     *  data    - A FILE Pointer of a file you might want to send.
     *  result  - The FILE Pointer in which you want to store the result.
     *  returns - The length of your demanted result or 1 if the request is succssesful or 0 if it failed. But it might be 0 too wether your result has 0 in length.
     */
    unsigned int post(const char *url, FILE *data, FILE *result);

    /* Function: post
     *  A simple get-request just insert the url and a two FILE Pointers to send the content of the file out and store you results.
     *  Your data to sent can come from a file.
     *  If you want you can get the result back as a string.
     *  Sometimes you want get a large result, more than 64 Bytes
     *  than define your size.
     *
     *  url     - The requested URL.
     *  data    - A FILE Pointer of a file you might want to send.
     *  result  - The answere to your request, by default you have not to take it. But if you want it, it has a default length from 64 Bytes.
     *  length  - The maximum size of your result. By default 64 Bytes.
     *  returns - The length of your demanted result or 1 if the request is succssesful or 0 if it failed. But it might be 0 too wether your result has 0 in length.
     */
    unsigned int post(const char *url, FILE *data = NULL, char *result = NULL, int length = 64);

    /* Function: timeout
     *  Sets the timout for a HTTP request. 
     *  The timout is the time wich is allowed to spent between two incomming TCP packets. 
     *  If the time is passed the connection will be closed.
     */
    void timeout(int value);

  private:
    virtual void err(err_t err);
    virtual err_t poll();
    virtual err_t sent(u16_t len)                 {return ERR_OK;};
    virtual err_t connected(err_t err);
    virtual err_t recv(struct pbuf *q, err_t err);
    virtual void dnsreply(const char *hostname, struct ip_addr *ipaddr);
    unsigned int make(const char *);
    
    char *_auth;
    bool _ready;
    char _mode;
    char _state;
    int  _timeout;
    const char *_host;
    const char *_path;
    void *_result;
    void *_data;
    const char *_request;
    const char *_headerfields;
    unsigned int _hostlen;
    unsigned int _resultoff;
    unsigned int _resultleft;
    int _timeout_max;
};

#endif
