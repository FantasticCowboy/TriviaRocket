/*

Authors: Luke Hobeika, Warren Dietz
Description: This is the server application of a chat room
Date: May 9, 2021

MIT License

Copyright (c) 2021 Luke Hobeika, Warren Dietz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
    
*/

#define BUFFERSIZE 200

enum messageType{
    ANSWER,
    LOGINACCEPTED,
    LOGINDENIED,
    LOGIN,
    QUESTION,
    ENDQUESTION,
    START,
    END,
    NA
};

struct message{
    messageType type;
    char        body[BUFFERSIZE];
};


// sends the whole message out
void sendAll(message m, int fd){
    uint16_t out = htons( uint16_t(m.type) );
    send(fd, &out ,sizeof(out), 0);
    int bytesSent= 0;
    int errorCode;


    while(bytesSent < BUFFERSIZE){
        errorCode = send(fd, &m.body + bytesSent, sizeof(m.body) - bytesSent, 0);
        if(errorCode == -1){
            std::cout << "Failed to send out bytes" << std::endl;
            exit(1);
        }
        bytesSent += errorCode;
    }// while
};

message recieveAll(int fd){

    message m;
    m.type = NA;
    uint16_t type;
    int bytesRecieved = 0;

    char buffer[BUFFERSIZE];

    int error = recv(fd, &type, sizeof(type), 0 );

    type = ntohs(type);

    memcpy(&m.type, &type, sizeof(type));

    while(bytesRecieved < BUFFERSIZE){

        int bytes = recv(fd, &buffer, sizeof(buffer), 0);

        if(bytes == 0){
            std::cout << "connection shutdown" << std::endl;
            exit(1);
        }// if


        memcpy(&m.body + bytesRecieved, buffer, bytes);

        bytesRecieved += bytes;
    }//
    return m;
}