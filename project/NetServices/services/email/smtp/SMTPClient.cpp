
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

#include "SMTPClient.h"

/*
  Provided as reference only, this code has not been tested.
*/
#if 0

#include <stdio.h>

#define __DEBUG
#include "dbg.h"

#define BUF_SIZE 128
#define CHUNK_SIZE 512

SMTPClient::SMTPClient() : m_pMessage(NULL), m_nextState(SMTP_HELLO), 
m_pCbItem(NULL), m_pCbMeth(NULL), m_watchdog(), m_timeout(0), m_posInMsg(0), m_closed(true), m_host()
{
  setTimeout(SMTP_REQUEST_TIMEOUT);
}

SMTPClient::~SMTPClient()
{
  close();
}

void SMTPClient::setHost(const Host& host)
{
  m_host = host;
}
  
void SMTPClient::send(EmailMessage* pMessage)
{
  init();
  m_posInMsg = 0;
  m_nextState = SMTP_HELLO;
  if( !m_pTCPSocket->connect(m_host) )
  {
    close();
    onResult(SMTP_DISC);
  }
}

void SMTPClient::init() //Create and setup socket if needed
{
  close(); //Remove previous elements
  if(!m_closed) //Already opened
    return;
  m_nextState = SMTP_HELLO;
  m_pTCPSocket = new TCPSocket;
  m_pTCPSocket->setOnEvent(this, &SMTPClient::onTCPSocketEvent);
  m_closed = false;
}

void SMTPClient::close()
{
  if(m_closed)
    return;
  m_closed = true; //Prevent recursive calling or calling on an object being destructed by someone else
  m_watchdog.detach();
  m_pTCPSocket->resetOnEvent();
  m_pTCPSocket->close();
  delete m_pTCPSocket;
  m_pTCPSocket = NULL;
}

int SMTPClient::rc(char* buf) //Parse return code
{
  int rc;
  int len = sscanf(buf, "%d %*[^\r\n]\r\n", &rc);
  if(len != 1)
    return -1;
  return rc;
}

#define MIN(a,b) ((a)<(b))?(a):(b)
void SMTPClient::process(bool moreData) //Main state-machine
{
  char buf[BUF_SIZE] = {0};
  if(moreData)
  {
    if( m_nextState != SMTP_BODYMORE )
    {
      return;
    }
  }
  if(!moreData) //Receive next frame
  {
    m_pTCPSocket->recv(buf, BUF_SIZE - 1);
  }
  
  IpAddr myIp(0,0,0,0);
  string to;
  int sendLen;
  
  DBG("In state %d", m_nextState);

  switch(m_nextState)
  {
  case SMTP_HELLO:
    if( rc(buf) != 220 )
      { close(); onResult(SMTP_PRTCL); return; }
    myIp = Net::getDefaultIf()->getIp();
    sprintf(buf, "HELO %d.%d.%d.%d\r\n", myIp[0], myIp[1], myIp[2], myIp[3]);
    m_nextState = SMTP_FROM;
    break;
  case SMTP_FROM:
    if( rc(buf) != 250 )
      { close(); onResult(SMTP_PRTCL); return; }
    sprintf(buf, "MAIL FROM:<%s>\r\n", m_pMessage->m_from.c_str());
    break;
  case SMTP_TO:
    if( rc(buf) != 250 )
      { close(); onResult(SMTP_PRTCL); return; }
    to = m_pMessage->m_lTo.front();
    sprintf(buf, "RCPT TO:<%s>\r\n", to.c_str());
    m_pMessage->m_lTo.pop();
    if(m_pMessage->m_lTo.empty())
    {
      m_nextState = SMTP_DATA;
    }
    break;  
  case SMTP_DATA:
    if( rc(buf) != 250 )
      { close(); onResult(SMTP_PRTCL); return; }
    sprintf(buf, "DATA\r\n");
    break;
  case SMTP_BODY:
    if( rc(buf) != 354 )
      { close(); onResult(SMTP_PRTCL); return; }  
    m_nextState = SMTP_BODYMORE;
  case SMTP_BODYMORE:
    sendLen = 0;
    if( m_posInMsg < m_pMessage->m_content.length() )
    {
      sendLen = MIN( (m_pMessage->m_content.length() - m_posInMsg), CHUNK_SIZE );
      m_pTCPSocket->send( m_pMessage->m_content.c_str() + m_posInMsg, sendLen );
      m_posInMsg += sendLen;
    }
    if( m_posInMsg == m_pMessage->m_content.length() )
    {
      sprintf(buf, "\r\n.\r\n"); //EOF
      m_nextState = SMTP_EOF;
    }
    break;
  case SMTP_EOF:
    if( rc(buf) != 250 )
      { close(); onResult(SMTP_PRTCL); return; }
    sprintf(buf, "QUIT\r\n");
    m_nextState = SMTP_BYE;
    break;
  case SMTP_BYE:
    if( rc(buf) != 221 )
      { close(); onResult(SMTP_PRTCL); return; }
    close();
    onResult(SMTP_OK);
    break;
  }
  
 if( m_nextState != SMTP_BODYMORE )
 {
   m_pTCPSocket->send( buf, strlen(buf) );
 }
}

void SMTPClient::setTimeout(int ms)
{
  m_timeout = 1000*ms;
  resetTimeout();
}

void SMTPClient::resetTimeout()
{
  m_watchdog.detach();
  m_watchdog.attach_us<SMTPClient>(this, &SMTPClient::onTimeout, m_timeout);
}

void SMTPClient::onTimeout() //Connection has timed out
{
  close();
  onResult(SMTP_TIMEOUT);
}
  
void SMTPClient::onTCPSocketEvent(TCPSocketEvent e)
{
  switch(e)
  {
  case TCPSOCKET_READABLE:
    resetTimeout();
    process(false);
    break;
  case TCPSOCKET_WRITEABLE:
    resetTimeout();
    process(true);
    break;
  case TCPSOCKET_CONTIMEOUT:
  case TCPSOCKET_CONRST:
  case TCPSOCKET_CONABRT:
  case TCPSOCKET_ERROR:
    onResult(SMTP_DISC);
    DBG("\r\nConnection error in SMTP Client.\r\n");
    close();
    break;
  case TCPSOCKET_DISCONNECTED:
    if(m_nextState != SMTP_BYE)
    {
      onResult(SMTP_DISC);
      DBG("\r\nConnection error in SMTP Client.\r\n");
      close();
    }
    break;
  }
}

void SMTPClient::onResult(SMTPResult r) //Must be called by impl when the request completes
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)(r);
}

#endif
