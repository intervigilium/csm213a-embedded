#include "HTTPServer.h"
#include "NetServer.h"

using namespace std;
using namespace mbed;

//unsigned int gconnections = 0;

unsigned int mbed::hash(unsigned char *str) {
  unsigned int hash = 5381;
  int c;
  while((c = *(str++))!=(unsigned char)'\0') {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

HTTPConnection::HTTPConnection(HTTPServer *pparent, struct tcp_pcb *pcb)
: TCPConnection(pparent, pcb), request_incomplete(true), data(NULL), 
request_handler(NULL), request_status(HTTP_NotFound), parent(pparent), 
_request_url(NULL), _request_type(0), _request_headerfields(NULL), 
_request_length(0), _request_arg_key(NULL), _request_arg_value(NULL), 
_request_arg_state(0), emptypolls(0) {
  _timeout_max = pparent->timeout();
}

HTTPConnection::~HTTPConnection() {
  deleteRequest();
  emptypolls = NetServer::time();
}

const char *HTTPConnection::getField(char *key) const {
  unsigned int h = hash((unsigned char *)key);
  return _request_fields.find(h)->second;
}

void HTTPConnection::addField(char *key, char *value) {
  unsigned int h = hash((unsigned char *)key);
  if(parent->isField(h)) {
    _request_fields.insert( make_pair(h, value));
  }
}

void HTTPConnection::send() {
  int i = sndbuf();
  if(!request_incomplete&&i) {
    switch(request_handler->send(this, i)) {
      case HTTP_SuccessEnded:
      case HTTP_Failed: {
        deleteRequest();
        release_callbacks();
        NetServer::get()->free(this);
        close();
      } break;
      default:
        emptypolls = NetServer::time();
        break;
    }
  } else {
    if(NetServer::time() - emptypolls > _timeout_max) {
      release_callbacks();
      NetServer::get()->free(this);
      close();
    }
  }
}

void HTTPConnection::store(void *d, struct pbuf *p) {
  int len = p->len-(((int)d)-((int)p->payload));
  do {
    switch(request_handler->data(this, d, len)) {
      case HTTP_SuccessEnded:
      case HTTP_Failed: {
        deleteRequest();
        release_callbacks();
        NetServer::get()->free(this);
        close();
      } break;
      default:
        break;
    }
    p = p->next;
    if(p) {
      len = p->len;
      d = static_cast<char *>(p->payload);
    }
  } while(_request_type&&p);
}

void HTTPConnection::err(err_t err) {
  printf("Error\n");
  release_callbacks();
  NetServer::get()->free(this);
}

err_t HTTPConnection::poll() {
  send();
  return ERR_OK; 
}

err_t HTTPConnection::sent(u16_t len) {
  return poll();
}

err_t HTTPConnection::recv(struct pbuf *q, err_t err) {
  struct pbuf *p = q;
  int i;
  char *data;
  if(err == ERR_OK && p != NULL) {
    /* Inform TCP that we have taken the data. */
    recved(p->tot_len);
    data = static_cast<char *>(p->payload);

    // :1
    // Looking if it's GET, POST, 
    // Followup from an incomplete request Header,
    // POST data or just crap (DEL, HEAD ...).
    if(!_request_type&&(strncmp(data, "GET ", 4) == 0)) {
      _request_type = GET;                   // Need :2
    } else if(!_request_type&&(strncmp(data, "POST ", 5) == 0)) {
      _request_type = POST;                  // Need :2
    } else if(_request_type&&request_incomplete) {
      getFields(&p, &data); // Need :3
    } else if(_request_type == POST) {
      // Followup (Data)            // Exits
      data = static_cast<char *>(p->payload);
      store(data, p);
      emptypolls = NetServer::time();
      pbuf_free(q);
      data = NULL;
      return ERR_OK;
    } else {
      pbuf_free(q);                 // Exits
      data = NULL;
      return ERR_OK;
    }

    // :2
    // Processing first POST or GET Packet
    // need :3   v--- If its 0 we have followup header data.
    if(_request_type&&!_request_url) {
      char *pagename = (char *)(data + _request_type);
      for(i = _request_type; i < p->len; i++) {
        if((data[i] == ' ') || (data[i] == '\r') || (data[i] == '\n')) {
          data[i] = 0;
  	      data = &data[i+1];
          break;
        }
      }
      emptypolls = NetServer::time();

      if((pagename[0] == '/') && (pagename[1] == 0)) {
        pagename = "/index.htm";
      }

      i = strlen(pagename);
      _request_url = new char[i+1];
      memcpy(_request_url, pagename, i);
      _request_url[i] = '\0';
      getFields(&p, &data);
    }
    // :3
    // Send or store the first amoungh of data.
    // Only when the message is complete.
    if(!request_incomplete) {
      emptypolls = NetServer::time();
      // Find the right handler
      if(!request_handler) {
        request_handler = parent->handle(this);
        request_status = request_handler->init(this);
      }
      i = strlen(_request_headerfields) + 120;
      char *buf = new char[i];
      sprintf(buf, "HTTP/1.1 %d OK\r\nServer:mbed embedded%s%d%s\r\nConnection: Close\r\n\r\n", request_status, 
                   (_request_length?"\r\nContent-Length: ":""),_request_length, 
                    getHeaderFields());
      i = strlen(buf);
      if(sndbuf()>i) {
        if(request_status==HTTP_NotFound) {
          const char *msg = {
            "HTTP/1.1 404 Not Found\r\nServer:mbed embedded\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 163\r\n"
            "\r\n"
            "<html>\r\n"
            "<header>\r\n"
            "<title>File not found<title>\r\n"
            "</header>\r\n"
            "<body>\r\n"
            "<h1>HTTP 404</h1>\r\n"
            "<p>The file you requested was not found on this mbed. </p>\r\n"
            "</body>\r\n"
            "</html>\r\n"
          };

          write((void *)msg, strlen(msg), 0);
          deleteRequest();
        } else {
          write(buf, i, (TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE));
          if(_request_type == POST) {
            store(data, p);
          } else {
            send();
          }
        }
      }
      delete buf;
    }

    // Exits
    pbuf_free(q);
    data = NULL;
  } else {
    release_callbacks();
    NetServer::get()->free(this);
    close();
  }
  return ERR_OK;
}
    
void HTTPConnection::getFields(struct pbuf **q, char **d) {
  if(parent->fields.empty()) {
    while((*q)&&request_incomplete) {
      unsigned int end = ((unsigned int)((*q)->payload)+(unsigned int)((*q)->len));
      for(; request_incomplete && ((unsigned int)(*d) < end); (*d)++) {
        if((*((char *)((*d)-0))=='\n')&&(*((char *)((*d)-1))=='\r')&&
           (*((char *)((*d)-2))=='\n')&&(*((char *)((*d)-3))=='\r')) {
          request_incomplete = false;
          (*d) += 1;
          break;
        }
      }
      if(request_incomplete) {
        (*q) = (*q)->next;
	    if((*q)) {
          (*d) = static_cast<char *>((*q)->payload);
        }
	  }
    }
  } else {
    char *kb = *d, *ke = NULL, *vb = *d, *ve = NULL;
    while((*q)&&request_incomplete) {
      unsigned int end = ((unsigned int)((*q)->payload)+(unsigned int)((*q)->len));
      for(; request_incomplete && ((unsigned int)(*d) < end); (*d)++) {
        switch(**d) {
          case ' ': switch(_request_arg_state) {
            case 1: case 2: _request_arg_state = 2; break;
            case 3:         _request_arg_state = 3; break;
            default:        _request_arg_state = 0; break;
          } break;
          case ':': switch(_request_arg_state) {
            default:        _request_arg_state = 2; break;
            case 3:         _request_arg_state = 3; break;
          } break;
          case '\r': switch(_request_arg_state) {
            default:        _request_arg_state = 4; break;
            case 5: case 6: _request_arg_state = 6; break;
          } break;
          case '\n': switch(_request_arg_state) {
            default:        _request_arg_state = 4; break;
            case 4: case 5: _request_arg_state = 5; break;
            case 6:         _request_arg_state = 7; break;
          } break;
          default: switch(_request_arg_state) {
            default:        _request_arg_state = 1; break;
            case 2: case 3: _request_arg_state = 3; break;
          } break;
        }
        switch(_request_arg_state) {
          case 0: kb = (*d)+1; break; //PreKey
          case 1: ke = (*d);   break; //Key
          case 2: vb = (*d)+1; break; //PreValue
          case 3: ve = (*d);   break; //Value
          default:             break;
          case 7: request_incomplete = false; break;
          case 5: {
            int oldkey = (_request_arg_key)?strlen(_request_arg_key):0;
            int oldval = (_request_arg_value)?strlen(_request_arg_value):0;
            int keylen =(ke&&kb)?ke-kb+1:0;
            int vallen = (ve&&vb)?ve-vb+1:0;
            char *key = new char[oldkey+keylen];
            char *val = new char[oldval+vallen];
            if(_request_arg_key&&oldkey) {
              strncpy(key, _request_arg_key, oldkey);
            }
            if(_request_arg_value&&oldval) {
              strncpy(val, _request_arg_value, oldval);
            }
            if(kb&&keylen) {
              strncpy(&key[oldkey], kb, keylen);
            }
            if(vb&&vallen) {
              strncpy(&val[oldval], vb, vallen);
            }
            key[oldkey+keylen] = 0;
            val[oldval+vallen] = 0;
//            printf("'%s':='%s'\n",  key, val);
            addField(key, val);
            kb = vb = (*d)+1;
            ke = ve = NULL;
            if(_request_arg_key) {
              delete _request_arg_key;
              _request_arg_key = NULL;
            }
            if(_request_arg_value) {
              delete _request_arg_value;
              _request_arg_value = NULL;
            }
            delete key;
          } break;
        }
      }
    }
    switch(_request_arg_state) {
      case 0: break; // PreKey
      case 5: break; // n-rec
      case 6: break; // 2r-rec
      default: break;
      case 1: case 2: { // Key // PreValue
        int keylen =(kb)?(*d)-kb+1:0;
        _request_arg_key = new char[keylen];
        strncpy(_request_arg_key, kb, keylen+1);
        _request_arg_key[keylen] = 0;
      } break; 
      case 3: case 4: { // Value // r-rec
        int keylen =(ke&&kb)?ke-kb+1:0;
        int vallen = (vb)?(*d)-vb+1:0;
        _request_arg_key = new char[keylen];
        _request_arg_value = new char[vallen];
        strncpy(_request_arg_key, kb, keylen+1);
        strncpy(_request_arg_value, vb, vallen+1);
        _request_arg_key[keylen] = 0;
        _request_arg_value[vallen] = 0;
      } break; 
    }
    if(request_incomplete) {
      (*q) = (*q)->next;
	  if((*q)) {
        (*d) = static_cast<char *>((*q)->payload);
	  }
	}
  }
}

HTTPServer::HTTPServer(unsigned short port)
  : TCPListener(port), _timeout_max(60000) {
}

HTTPServer::HTTPServer(const char *hostname, struct ip_addr ip, struct ip_addr nm, struct ip_addr gw, struct ip_addr dns, unsigned short port)
  : TCPListener(port), _timeout_max(60000) {
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
    
void HTTPConnection::deleteRequest() {
  for(map<unsigned int, char *>::iterator iter = _request_fields.begin();
    iter!=_request_fields.end();iter++) {
    delete iter->second;
  }
  _request_fields.clear();
  if(data) {delete data; data = NULL; };
  
  if(_request_type) {
    delete _request_headerfields;
    delete _request_arg_key;
    delete _request_arg_value;
    if(_request_url) {
      delete _request_url;
      _request_url = NULL;
    }
  }
  _request_type = 0;
  request_incomplete = true;
}

