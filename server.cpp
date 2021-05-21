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

#define  BACKLOG 10

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int setupServer(std::string port, std::string ip){
    std::cout << "Initializing server at PORT:" << port << ", IP: " << ip << "\n";

    struct addrinfo hints;
    struct addrinfo *result;
    int fileDescriptor;
    
    memset(&hints, 0, sizeof(hints)); // make sure hints is empty

    hints.ai_family = AF_UNSPEC;      // allows for both ipv4 and ipv6 addresses
    hints.ai_socktype = SOCK_STREAM;  // only searches for stream sockets

    // attempts to get the addresses, if it fails it stores the error code in erno and exits the program
    int errorCode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
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

    errorCode = bind(fileDescriptor, result->ai_addr, result->ai_addrlen);
    if(errorCode != 0){
        std::cout << errno << "\n";
        std::cerr << "ERROR: Could not bind socket\n";
        exit(1);
    }// if

    errorCode = listen(fileDescriptor, BACKLOG );
    if(errorCode != 0){
        std::cerr << "ERROR: Could not listen to socket\n";
        exit(1);
    }// if


    freeaddrinfo(result);

    std::cout << "Server successfully started!\n";

    return fileDescriptor;
}// setup

int addNewConnection(int fileDescriptor, bool *doneAccepting, int maxPlayers, std::vector<int> *fileDescriptors){
    std::cout << "Awaiting new connection\n";
    struct sockaddr_storage incomingAddress;
    socklen_t sin_size = sizeof(incomingAddress);
    int newFileDescriptor = accept(fileDescriptor, (struct sockaddr*) &incomingAddress, &sin_size);

    if(*doneAccepting || fileDescriptors->size() >= maxPlayers){
        return -1;
    }// if

    if(newFileDescriptor == -1){
        std::cerr << "ERROR: Did not accept connection correctly\n";
        return -1;
    }//if
    char s[INET6_ADDRSTRLEN];
    inet_ntop(incomingAddress.ss_family, get_in_addr((struct sockaddr *) &incomingAddress), s, sizeof(s));
    std::cout << "New connection from " << s << "\n";
    return newFileDescriptor;
}// addNEwConnection



// only adds a new client if they added in
void login(int fileDescriptor, std::unordered_map<std::string, int> *usernameFileDescriptor, std::mutex *usernameLock, std::vector<int> *fileDescriptors, bool *doneAccepting){
    std::cout << "Logging in new user\n";
    bool finished = false;
    message out{LOGINDENIED, "Server: login deined, name in use"};
    out.type = LOGINDENIED;
    message in;
    while (!finished && !*doneAccepting){
        in = recieveAll(fileDescriptor); 

        std::cout << in.body << std::endl;
        
        if(*doneAccepting){
            return;
        }// if

        std::string temp = in.body;

        std::cout << "The user has selected " << temp << " as their username\n";

        // locks usernames while it is being accesed
        usernameLock->lock();
        if(usernameFileDescriptor->find(temp) == usernameFileDescriptor->end()){
            usernameFileDescriptor->insert({temp,fileDescriptor});
            finished = true;
            out.type = messageType::LOGINACCEPTED;
            fileDescriptors->push_back(fileDescriptor);
            sendAll(out, fileDescriptor);
        }// if
        else{
            std::cout << temp << " already is in use\n";
            sendAll(out, fileDescriptor);            
        }// else
        usernameLock->unlock();
    }// while
}// login

void connectPlayers(std::vector<int> *fileDescriptors, bool *doneAccepting, int fileDescirptor, int maxPlayers, std::unordered_map<std::string, int> *usernameFileDescriptor, std::mutex *usernameLock , std::vector<std::thread> *threads){
    while(!*doneAccepting && fileDescriptors->size() < maxPlayers){
        int fd = addNewConnection(fileDescirptor, doneAccepting, maxPlayers, fileDescriptors);
        if(fd == -1){
            break;
        }// if
        threads->push_back(std::thread(login, fd, usernameFileDescriptor, usernameLock, fileDescriptors, doneAccepting));
    }// while
    *doneAccepting =true;
}// connectPlayers

void quitConnecting(bool *doneAccepting){
    std::string temp;
    while(temp != "Done" && temp != "done"){
        std::cin >> temp;
    }// while
    *doneAccepting = true; 
}// quitConnecting

struct question{
    std::string question;
    std::string a;
    std::string b;
    std::string c;
    std::string d;
    std::string answer;
};

std::string formatQuestion(question q){
    return "Question: " + q.question + "\nA: " + q.a + "\nB: " + q.b + "\nC: " + q.c + "\nD: " + q.d;
}


std::vector<question> readJson(std::string path){

    std::vector<question> questions;

    nlohmann::json j;

    std::ifstream file;

    file.open(path);

    file >> j;

    for( auto array : j){
        question temp;
        temp.a = array["A"];
        temp.b = array["B"];
        temp.c = array["C"];
        temp.d = array["D"];
        temp.question = array["question"];
        temp.answer = array["answer"];
        questions.push_back(temp);
    }

    return questions;


}//

int main(int argc, char const *argv[]){

    if(argc > 7 || argc < 7){
        std::cerr << "Usage: server port ip round_time max_points max_rounds max_players\n";
        exit(1);
    }// if

    std::string port = argv[1];
    std::string ip   = argv[2];
    int roundTime;                                                  // round time in seconds
    int maxPoints;                                                  //
    int maxRounds;                                                  //
    int maxPlayers;                                                 //
    int acceptFd;                                                   // file descriptor we listen for new conenctions on
    std::vector<int> fileDescriptors;                               // all connected fille descritors
    bool doneAccepting = false;                                     // if the server is done accepting new clients
    std::vector<std::thread> threads;                               // contains all threads listening to the different end users
    std::string path = "./trivia.json";
    std::vector<question> questions = readJson(path);     

    int rounds = 0;



    std::unordered_map<std::string, int> usernameToScore;           // holds the score of each player;


                                                                    // below datastructures deal with logging users in
    std::unordered_map<int, std::string> fileDescriptorToUsername;    //  
    std::unordered_map<std::string, int> usernameToFileDescriptor;    //
    std::set<std::string> usernames;                                // used to determine if an inputed username is unique
    std::mutex usernamesLock;                                       //


    for(int i = 3; i < 7; ++i) {
        if(isdigit(*argv[i])) {
            if(i == 3) {
                roundTime = atoi(argv[i]);
            }// if
            if(i == 4) {
                maxPoints = atoi(argv[i]);
            }// if
            if(i == 5) {
                maxRounds = atoi(argv[i]);
            }// if
            if(i == 6) {
                maxPlayers = atoi(argv[i]);
            }// if
        }// if
        else {
            std::cerr << "Usage: server port ip round_time max_points max_rounds max_players\n";
            exit(1);
        }// else
    }

    acceptFd = setupServer(port, ip);

    std::thread connect(connectPlayers, &fileDescriptors, &doneAccepting, acceptFd, maxPlayers, &usernameToFileDescriptor, &usernamesLock, &threads);
    std::thread serverBreak(quitConnecting, &doneAccepting);

    while(!doneAccepting){}// while

    while(!threads.empty()){
        threads.back().detach();
        threads.back().~thread();
        threads.pop_back();

    }//
    connect.detach();
    serverBreak.detach();
    connect.~thread();
    serverBreak.~thread();


    for(auto item : usernameToFileDescriptor){
        usernameToScore[item.first] = 0;
    }// for
    for(auto item : usernameToFileDescriptor){
        fileDescriptorToUsername[item.second] = item.first;
    }

    std::cout << "Starting Game\n";
    for(auto fd : fileDescriptors){
        sendAll({messageType::START, "Server: Starting Game..."}, fd);
    }// for




    for(; rounds < maxRounds; rounds ++){
        for(auto fd: fileDescriptors){
            message q;
            strcpy( (q.body), formatQuestion(questions[rounds]).c_str());
            q.type = QUESTION;
            sendAll(q, fd);
        }// for

    
        std::this_thread::sleep_for(std::chrono::seconds(roundTime));

        for(auto fd: fileDescriptors){
            message t;
            t.type = ENDQUESTION;
            sendAll(t, fd); 
        }// for

        for(auto fd: fileDescriptors){
            message t;
            t = recieveAll(fd);
            std::string a = (t.body);
            if(a == questions[rounds].answer ){
                std::string temp = fileDescriptorToUsername[fd];
                usernameToScore[temp] += 1;
            }//if 
        }// for
    }// for

    
    std::string s;
    for(auto item : usernameToScore){
        s = s + item.first + " : " + std::to_string(item.second) + "\n";
    }// for


    message end;

    end.type = END;

    strcpy(end.body, s.c_str());


    for(auto fd : fileDescriptors){
        sendAll(end, fd);
    }// for


    
    
    close(acceptFd);





    return 0;
}// server
