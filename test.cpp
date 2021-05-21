
#include <json.hpp>
#include <iostream>
#include <fstream>

int main(){

    std::string path = "./trivia.json";

    std::ifstream myfile;
    
    myfile.open(path);

    nlohmann::json j;
    
    myfile >> j;

    std::string temp;


    for(auto it : j){
        temp = it["A"];
        std::cout << temp << "\n";
    }

    

    

    return 0;
}