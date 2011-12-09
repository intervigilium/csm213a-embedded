#include "HTTPClient.h"
#include "NetServer.h"
#include "iputil.h"

using namespace mbed;
using namespace std;

#define POST   0x1
#define FDATA  0x2
#define FRES   0x4
#define GET    0x0
#define CDATA  0x0
#define CRES   0x0

long fleft(FILE *fd) {
  long len, cur;
  cur = ftell(fd);
  fseek(fd, 0, SEEK_END);
  len = ftell(fd);
  fseek(fd, cur, SEEK_SET);
  return len;
}


HTTPClient::HTTPClient(const char *hostname, struct ip_addr ip, struct ip_addr nm, struct ip_addr gw, struct ip_addr dns)
  : TCPConnection(),  _auth(NULL), _timeout(0), _data(NULL), _headerfields(NULL), _timeout_max(60000) {
  NetServer *net = NULL;
  if(ip.addr != ip_addr_any.addr && nm.addr != ip_addr_any.addr && gw.addr != ip_addr_any.addr) {
    net = NetServer::create(ip, nm, gw);
    if(dns.addr != ip_addr_any.addr) {
      net->setDNS1(dns);
    }
  } else if(hostname) {
    net = NetServer::create();
  }
  if(hostname) {
    net->setHostname(hostname);
  }
}

void HTTPClient::timeout(int value) {
  _timeout_max = value;
}

void HTTPClient::err(err_t err) {
  release_callbacks();
}

err_t HTTPClient::poll() {
  if(NetServer::time() - _timeout > _timeout_max) {
    release_callbacks();
    close();
    printf("TIMEOUT\n");
    _ready = true;
    _state = 99;
  }
  return ERR_OK;
}

void HTTPClient::dnsreply(const char *hostname, struct ip_addr *ipaddr) {
  _ready = true;
  _ipaddr = *ipaddr;
  _state = (ipaddr==NULL)? 99 : 0;
  _timeout = NetServer::time();
}

err_t HTTPClient::connected(err_t err) {
  TCPConnection::connected(err);
  _ready = false;
  _state = 0;
  _resultoff = 0;
  if(_mode&POST) {
    write((void *)"POST ", 5,  TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
  } else {
    write((void *)"GET ", 4, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
  }
  if(strlen(_path) == 0) {
    write((void *)"/", 1, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
  } else {
    write((void *)_path, strlen(_path), TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
  }
  write((void *)" HTTP/1.1\r\nHost: ", 17, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
  write((void *)_host, _hostlen, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
  if(_auth&&(*_auth!='\0')) {
    write((void *)"\r\nAuthorization: Basic ", 23, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE); 
    write((void *)_auth, strlen(_auth), TCP_WRITE_FLAG_COPY |TCP_WRITE_FLAG_MORE); 
  }
  if(_headerfields&&(*_headerfields!='\0')) {
    write((void *)"\r\n", 2, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE); 
    write((void *)_headerfields, strlen(_headerfields), TCP_WRITE_FLAG_COPY |TCP_WRITE_FLAG_MORE); 
  }
  write((void *)"\r\nConnection: Close\r\n", 21, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_COPY); 
  if(_data) {
    char clen[256];
    int len, blen;
    if(_mode&FDATA) {
      //printf("Send file\n");
      len = fleft((FILE *)_data);
      sprintf(clen, "Content-Length: %d\r\n\r\n\0", len);
      write((void *)clen, strlen(clen), TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_COPY); 
      if(len) {
        do {
          int sb = sndbuf();
          blen = fread(clen, sizeof(char), (int)min(sb, 100), (FILE *)_data);
          write(clen, blen, (TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE));
          len -= blen;
        } while(len > 1 && blen);  
      }
    } else {
      len = strlen((const char *)_data);
      sprintf(clen, "Content-Length: %d\r\n\r\n\0", len);
      write((void *)clen, strlen(clen), TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_COPY); 
      write((void *)_data, len, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE); 
    }
  }
  write((void *)"\r\n", 2, TCP_WRITE_FLAG_COPY);
  _timeout = NetServer::time(); 
  return ERR_OK; 
}

#define _min(x, y) (((x)<(y))?(x):(y))
err_t HTTPClient::recv(struct pbuf *p, err_t err) {
  if(err == ERR_OK && p != NULL&&_state<10) {
    _timeout = NetServer::time();
    struct pbuf *q = p;
    char *d = (char *)q->payload;
    recved(p->tot_len);

    while(q&&_state<10) {
      unsigned int end = ((unsigned int)(q->payload)+(unsigned int)(q->len));
      switch(_state) {
        case 0: {
          for(; _state==0 && ((unsigned int)d < end); d++) {
            if((*((char *)(d-0))=='\n')&&(*((char *)(d-1))=='\r')&&
               (*((char *)(d-2))=='\n')&&(*((char *)(d-3))=='\r')) {
              _state = 1;
              d += 1;
              break;
            }
          }
        } break;
        case 1: {
          if(_result) {
            if(_mode&FRES) {
              fwrite(d, sizeof(char), end - (unsigned int)d, (FILE *)_result);
              _resultoff  += (end - (unsigned int)d);
              d = (char *)end;
            } else {
              unsigned int len = _min(_resultleft, (end-(unsigned int)d));
              memcpy((char *)_result + _resultoff, d, len);
              _resultleft -= len;
              _resultoff  += len;
              d += len;
  
              if(!_resultleft) {
                _state = 10;
              }
            }
          } else {
             _state = 10;
          }
        } break;
        default: {
          break;
        }
      }
      if(_state<10&&(unsigned int)d==end) {
        q = q->next;
        if(q) {
          d = static_cast<char *>(q->payload);
        }
      }
    }
  }
  
  if(p!=NULL) {
    pbuf_free(p);
  }
  
  if(_state>10||p==NULL||err!=ERR_OK) {
    release_callbacks();
    close();
    _ready = true;
  }
  return ERR_OK;
}

unsigned int HTTPClient::make(const char *request) {
  _request    = request;
  _resultoff  = 0;
  _ready      = false;
  _state      = 0;
  _hostlen    = 0;
  _port       = 0;
  _timeout    = NetServer::time();
  NetServer::ready();

  int hostlen = 0;
  if(strlen(_request)<10||strncmp("http://", _request, 7)!=0) {
    printf("Only http requests are allowed\n");
    return 0;
  }
  _path = _host = _request + 6;
  _host++;
  while(!_state == 1) {
    switch(*(++_path)) {
      case ':':
        _port = atoi(_path+1);
        break;
      case '/':
      case '\0':
        _port = (_port)?_port:80;
        _state  = 1;
        break;
      default:
        break;
    }
    if(!_port) {
      hostlen++;
    }
    if(!_state == 1) {
      _hostlen++;
    }
  }
  _state = 0;
  
  if(hostlen>256) {
    printf("Hostname longer than allowed\n");
    return 0;
  }
  
  char host[257];
  memcpy(host, _host, hostlen);
  host[hostlen] = 0;
  _timeout = NetServer::time();
  struct in_addr in;
  if(!inet_aton(host, &in)) {
    _ready = false;
    if(dnsrequest(host, &_ipaddr)==ERR_INPROGRESS) {
      while(!_ready) {
        NetServer::poll();
        wait_ms(10);
      }
      if(_state==99) {
        printf("Server not found\n");
        return 0;
      }
    }
  } else {
    _ipaddr.addr = in.s_addr;
  }

  _ready = false;
  _timeout = NetServer::time();
  connect();
  set_poll_interval(10);
  tcp_setprio(_pcb, TCP_PRIO_MIN);
  while(!_ready) {
    NetServer::poll();
    wait_ms(10);
  }
  //release_callbacks();
  close();
  if(_state==99) {
    printf("Connection error\n");
    return 0;
  }  
  
  if(!_mode&FRES&&_result) {
    ((char *)_result)[_resultoff+1]  = '\0';
  }
  return ((!_mode&FRES)&&!_result)?1:_resultoff;
  
}

void HTTPClient::auth(const char *user, const char *password) {
  if(user) {
    char up[256];
    sprintf(up, "%s:%s", user, password);
    _auth = new char[base64enc_len(up)+1];
    base64enc(up, strlen(up), _auth);
  } else if(_auth) {
    delete _auth;
    _auth = NULL;
  }
}

void HTTPClient::headers(const char *fields) {
  _headerfields = fields;
}

unsigned int HTTPClient::get(const char *url, char *result, int rsize) {
  _mode = GET | CDATA | CRES;
  _data = (void *)NULL;
  _result = (void *)result;
  _resultleft = rsize -1;

  return make(url);
}

unsigned int HTTPClient::get(const char *url, FILE *result) {
  _mode = GET | CDATA | FRES;
  _data = (void *)NULL;
  _result = (void *)result;
  _resultleft = 0;

  return make(url);
}

unsigned int HTTPClient::post(const char *url, const char *data, char *result, int rsize) {
  _mode = POST | CDATA | CRES;
  _data = (void *)data;
  _result = (void *)result;
  _resultleft = rsize -1;
  
  return make(url);
}

unsigned int HTTPClient::post(const char *url, const char *data, FILE *result) {
  _mode = POST | CDATA | FRES;
  _data = (void *)data;
  _result = (void *)result;
  _resultleft = 0;
  
  return make(url);
}

unsigned int HTTPClient::post(const char *url, FILE *data, FILE *result) {
  _mode = POST | FDATA | FRES;
  _data = (void *)data;
  _result = (void *)result;
  _resultleft = 0;
  
  return make(url);
}

unsigned int HTTPClient::post(const char *url, FILE *data, char *result, int rsize) {
  _mode = POST | FDATA | CRES;
  _data = (void *)data;
  _result = (void *)result;
  _resultleft = rsize -1;
  
  return make(url);
}

