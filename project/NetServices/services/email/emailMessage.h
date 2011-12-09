
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

#ifndef EMAIL_MESSAGE_H
#define EMAIL_MESSAGE_H

class SMTPClient;

#include "smtp/SMTPClient.h"

#include <queue>
using std::queue;

#include <string>
using std::string;

class EmailMessage
{
public:
  EmailMessage(SMTPClient* pClient);
  ~EmailMessage();
  
  void setFrom(const char* from);
  void addTo(const char* to);
  int printf(const char* format, ... ); //Can be called multiple times to write the message
  
  void send();
  
  //For now, only message sending is implemented
  //int scanf(const char* format, ... ); 
  
private:
  friend class SMTPClient;
  queue<string> m_lTo;
  string m_from;
  
  string m_content;
  
  SMTPClient* m_pClient;

};



#endif
