
/*
Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

class EmailMessage;

#include "core/net.h"
#include "api/TCPSocket.h"
#include "../emailMessage.h"

#include "mbed.h"

#define SMTP_REQUEST_TIMEOUT 5000

enum SMTPResult
{
  SMTP_OK,
  SMTP_PRTCL, //Protocol error
  SMTP_TIMEOUT, //Connection timeout
  SMTP_DISC //Disconnected 
};

class SMTPClient /*: public NetService*/
{
public:
  SMTPClient();
  virtual ~SMTPClient();
  
  void setHost(const Host& host);
  void send(EmailMessage* pMessage);
  
  class CDummy;
  template<class T> 
  //Linker bug : Must be defined here :(
  void setOnResult( T* pItem, void (T::*pMethod)(SMTPResult) )
  {
    m_pCbItem = (CDummy*) pItem;
    m_pCbMeth = (void (CDummy::*)(SMTPResult)) pMethod;
  }
  
  void init(); //Create and setup socket if needed
  void close();
  
private:
  int rc(char* buf); //Return code
  void process(bool moreData); //Main state-machine

  void setTimeout(int ms);
  void resetTimeout();
  
  void onTimeout(); //Connection has timed out
  void onTCPSocketEvent(TCPSocketEvent e);
  void onResult(SMTPResult r); //Called when exchange completed or on failure
  
  EmailMessage* m_pMessage;

  TCPSocket* m_pTCPSocket;

  enum SMTPStep
  {
    SMTP_HELLO,
    SMTP_FROM,
    SMTP_TO,
    SMTP_DATA,
    SMTP_BODY,
    SMTP_BODYMORE,
    SMTP_EOF,
    SMTP_BYE
  };
  
  SMTPStep m_nextState;
  
  CDummy* m_pCbItem;
  void (CDummy::*m_pCbMeth)(SMTPResult);
  
  Timeout m_watchdog;
  int m_timeout;
  
  int m_posInMsg;
  
  bool m_closed;
  
  Host m_host;

};

#endif
