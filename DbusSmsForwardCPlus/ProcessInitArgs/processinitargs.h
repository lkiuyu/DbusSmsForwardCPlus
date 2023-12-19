#ifndef PROCESSINITARGS_H
#define PROCESSINITARGS_H

#include <regex>
#include <fstream> 
#include "../MyHelper/myhelper.h"

using namespace std;

void processInitArgs(int argc, char* argv[], string& startGuideChoiseNum, string& sendMethodGuideChoiseNum, string& configFilePath, vector<string>& splitOptionStrings) {
    for (int i = 1; i < argc; ++i)
    {
        if (string(argv[i]) == "-fE")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "1";
            splitOptionStrings.push_back(sendMethodGuideChoiseNum);
        }
        else if (string(argv[i]) == "-fP")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "2";
            splitOptionStrings.push_back(sendMethodGuideChoiseNum);
        }
        else if (string(argv[i]) == "-fW")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "3";
            splitOptionStrings.push_back(sendMethodGuideChoiseNum);
        }
        else if (string(argv[i]) == "-fT")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "4";
            splitOptionStrings.push_back(sendMethodGuideChoiseNum);
        }
        else if (string(argv[i]) == "-fD")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "5";
            splitOptionStrings.push_back(sendMethodGuideChoiseNum);
        }
        else if (string(argv[i]) == "-fB")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "6";
            splitOptionStrings.push_back(sendMethodGuideChoiseNum);
        }
        else if (string(argv[i]) == "-fS")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "7";
            splitOptionStrings.push_back(sendMethodGuideChoiseNum);
        }
        else if (string(argv[i]) == "-sS")
        {
            startGuideChoiseNum = "2";
        }
        else if (string(argv[i]).find("--configfile=") != string::npos)
        {
            string inputpath = string(argv[i]);
            replaceString(inputpath, "--configfile=", "");
            ifstream file(inputpath);
            if (file) {
                configFilePath = inputpath;
            }
        }
        else if (string(argv[i]).find("--sendsmsapi=") != string::npos)
        {
            string apiswitch = string(argv[i]);
            replaceString(apiswitch, "--sendsmsapi=", "");
            if (apiswitch == "enable") {
                if (startGuideChoiseNum == "1") {
                    startGuideChoiseNum = "3";
                }
                else {
                    startGuideChoiseNum = "4";
                }
            }
        }
    }
    if (splitOptionStrings.size()>1) {
        sendMethodGuideChoiseNum = "8";
    }
}

#endif // PROCESSINITARGS_H
