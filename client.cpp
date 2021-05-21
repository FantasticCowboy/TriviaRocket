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


#include "common.hpp"
#include "message.hpp"


int setupClient(std::string port, std::string ip){

    std::cout << "Connecting to server at PORT:" << port << ", IP: " << ip << "\n";

    int fileDescriptor;
    struct addrinfo hints;
    struct addrinfo *result;
    
    memset(&hints, 0, sizeof(hints)); // make sure hints is empty

    hints.ai_family = AF_UNSPEC;      // allows for both ipv4 and ipv6 addresses
    hints.ai_socktype = SOCK_STREAM;  // only searches for stream sockets

    // attempts to get the addresses, if it fails it stores the error code in erno and exits the program
    int errorCode = getaddrinfo(&ip[0], &port[0], &hints, &result);
    if( errorCode != 0){
        std::cerr << "ERROR: Could not get address info\n";
        gai_strerror(errorCode);
        exit(1);
    }    

    for(addrinfo* addr = result; addr != nullptr; addr = addr->ai_next){
        fileDescriptor = socket(result->ai_family, result->ai_socktype, result->ai_protocol);;
        if(fileDescriptor != -1){
            result = addr;
            break;
        }// if
        if(addr->ai_next == nullptr){
            std::cerr << "ERROR: Could not create a socket\n";
            exit(1);
        }// if
    }// for


    errorCode = connect(fileDescriptor, result->ai_addr, result->ai_addrlen);
    if(errorCode != 0){
        std::cerr << "ERROR: Could not connect\n";
        exit(1);
    }//if

    std::cout << "Succesfully connected!\n";

    return fileDescriptor;

}// setupClient

void printServerClosed(){
    std::cout << "Connection closed due to server ending";
}// printServerClosed

void listenForInput(std::string* str){
    while(true){
        std::string temp;
        std::cin >> temp;
        if(temp != "a" && temp != "b" && temp != "c" && temp != "d" && temp != "A" && temp != "B" && temp != "C" && temp != "D"){
            std::cout << "Answer must be a,b,c, or d\n";
        }
        else{
            *str = temp;
        }
    }// while
}

int main(int argc, char const *argv[]){
    
    std::string port = argv[1];
    std::string ip = argv[2];
    message in;                     // incoming messages
    message out;                    // outgoing messages
    int closed;                     // if the server has closed the connection
    std::string username;
    bool finished = false;          // if the game is over or not 
    std::string answer = "A";

    if(argc > 3 || argc < 3){
        std::cerr << "Usage: client port ip\n";
        exit(1);
    }// if

    int fileDescriptor = setupClient(port, ip);

    
    in.type = LOGINDENIED;
    out.type = LOGIN;

    while(in.type == LOGINDENIED){
        std::cin >> username;

        strcpy( (out.body), username.c_str());

        sendAll(out, fileDescriptor);

        in = recieveAll(fileDescriptor);
        if(in.type == LOGINDENIED){
            std::cout << in.body << std::endl;
        }// if
    }// while

    std::thread input(listenForInput, &answer);

    while(!finished){

        message temp = recieveAll(fileDescriptor);

        switch (temp.type){
        case START :
            std::cout << temp.body << "\n";
            break;
        case QUESTION:
            std::cout << temp.body << "\n";
            break;
        case ENDQUESTION:
            std::cout << "Answers sent!\n";
            message ans;
            ans.type = ANSWER;
            strcpy(ans.body, answer.c_str());
            sendAll(ans, fileDescriptor);
            break;
        case END :
            std::cout << "Game Over!\n";
            std::cout << temp.body << std::endl;
            finished = true;
            break;
        default:
            break;
        }
    }
    
    input.detach();
    input.~thread();

    close(fileDescriptor);

    return 0;
}
