#ifndef HTTPFILESYSTEM_H
#define HTTPFILESYSTEM_H

#include "mbed.h"

#include "HTTPServer.h"

#define HTTP_BUFFER_SIZE 700
#define FILENAMELANGTH 100

/**
 * This class will store the data which are required for an request.
 * We are not in every case able to return all data at once, that means we have to store
 * the actual level of transmission.
 */
class HTTPFileSystemData : public HTTPData {
  public:
    int fleft;
    int bleft;
    int offset;
    FILE *file;
    char buffer[HTTP_BUFFER_SIZE];
    
    virtual ~HTTPFileSystemData() {
      if(file) {
        fclose(file);
        file = 0;
      }
    }
};

/**
 * This class will deliver files form the virtual file system.
 * Furthermore it is a simple example how to implement an HTTPHandler for big data requests.
 */
class HTTPFileSystemHandler : public HTTPHandler {
  public:
    /**
     * Create a new HTTPFileSzstemHandler.
     * @param prefix The Prefix is the URL Proefix in witch the Handler will work.
     * @param dir The Prefix will be directly mappt on the dir.
     */
    HTTPFileSystemHandler(const char *path, const char *dir) : HTTPHandler(path), _dir(dir) {}
    HTTPFileSystemHandler(HTTPServer *server, const char *path, const char *dir) : HTTPHandler(path), _dir(dir) { server->addHandler(this); }

  private:
    /**
     * Check if a requested file exists.
     * If it exists open it and store the data back in the HTTPConnection.
     * We would not store connection specific data into the Handler.
     * If the file exists and we cann serve a page return HTTP_OK else HTTP_NotFound.
     * @param con The Connection which will be handled.
     */
    virtual HTTPStatus init(HTTPConnection *con) const {
      char filename[FILENAMELANGTH];
      HTTPFileSystemData *data = new HTTPFileSystemData();
      snprintf(filename, FILENAMELANGTH, "%s%s\0", _dir, con->getURL() + strlen(_prefix));
      data->file = fopen(filename, "r");
      if(!data->file) {
        delete data;
        return HTTP_NotFound;
      }
      data->fleft  = fleft(data->file);
      data->bleft  = 0;
      data->offset = 0;
      
      con->data = data;
      con->setLength(data->fleft);
      loadFromFile(con);
      return HTTP_OK;
    }

    /**
     * Send the maximum available data chunk to the Client.
     * If it is the last chunk close connection by returning HTTP_SuccessEnded
     * @param con The connection to handle
     * @param maximum The maximal available sendbuffer size.
     * @return HTTP_Success when mor data is available or HTTP_SuccessEnded when the file is complete.
     */
    virtual HTTPHandle send(HTTPConnection *con, int maximum) const {
      HTTPFileSystemData *data = static_cast<HTTPFileSystemData *>(con->data);
      err_t err;
      u16_t len = min(data->bleft, maximum);
//      printf("Send File\n");
      if(len) {
        do {
          err = con->write(
            &data->buffer[data->offset], len, (((!data->fleft)&&(data->bleft==len))? 
            (TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE) : (TCP_WRITE_FLAG_COPY)));
          if(err == ERR_MEM) {
            len /= 2;
          }
        } while (err == ERR_MEM && len > 1);  
  
        if(err == ERR_OK) {
          data->offset += len;
          data->bleft  -= len;
        }
      }
      return loadFromFile(con);
    }

    /**
     * Returns the left size of a file.
     * @param fd The filehandler of which we want to know the filesize.
     * @return The filesize of fd.
     */
    long fleft(FILE *fd) const {
      long len, cur;
      cur = ftell(fd);
      fseek(fd, 0, SEEK_END);
      len = ftell(fd);
      fseek(fd, cur, SEEK_SET);
      return len;
    }
    
    /**
     * Fill the buffer if the buffer is empty.
     * If the file is complete close the filehandler and return HTTP_SuccessEnded.
     */
    HTTPHandle loadFromFile(HTTPConnection *con) const {
      HTTPFileSystemData *data = static_cast<HTTPFileSystemData *>(con->data);
      if(!data->bleft) {
        if(data->fleft) {
          int len = fread(&data->buffer[0], sizeof(char), HTTP_BUFFER_SIZE, data->file);
          data->fleft -= len;
          data->bleft  = len;
          data->offset = 0;
        } else {
          if(data->file) {
            fclose(data->file);
            data->file = 0;
          }
          return HTTP_SuccessEnded;
        }
      }
      return HTTP_Success;
    }
    
    /** The Directory which will replace the prefix of the URL */
    const char *_dir;
};

#endif
