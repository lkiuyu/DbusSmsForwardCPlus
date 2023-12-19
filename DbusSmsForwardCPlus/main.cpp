#include <iostream>
#include <regex>
#include <thread>
#include <cstdio>
#include <locale>
#include "ProcessInitArgs/processinitargs.h"
#include "ProcessUserChoose/processuserchoose.h"
#include "ConfigFileProcess/configfileprocess.h"
#include "ProcessSetup/processsetup.h"
#include "InitWebPageAndApi/initwebpageandapi.h"

using namespace std;
string configFilePath = "";
vector<string> splitOptionStrings;

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");
    string startGuideChoiseNum = "";
    string sendMethodGuideChoiseNum = "";
    string deviceName = "";

    processInitArgs(argc, argv, startGuideChoiseNum, sendMethodGuideChoiseNum, configFilePath, splitOptionStrings);
    if (startGuideChoiseNum == "1" || startGuideChoiseNum == "3") {
        deviceName = "*Host*Name*";
    }
    string StartGuideResult = onStartGuide(startGuideChoiseNum);
    if (StartGuideResult == "1" || StartGuideResult == "3") {
        checkConfig(configFilePath);
        SetupDeviceName(deviceName);
        if (StartGuideResult == "3") {
            //创建一个线程来运行接口
            setupApiPort();
            string sendMethodGuideResult = sendMethodGuide(sendMethodGuideChoiseNum);
            thread server_thread1(monitorDbus, sendMethodGuideResult);
            thread server_thread(startSendSmsApi);
            server_thread1.join();
            server_thread.join();
        }
        else {
            string sendMethodGuideResult = sendMethodGuide(sendMethodGuideChoiseNum);
            monitorDbus(sendMethodGuideResult);
        }
    }
    else if (StartGuideResult == "2") {  //以下为调用dbus发送短信
        printf("请输入收信号码：\n");
        string telNumber = "";
        getline(cin, telNumber);
        printf("请输入短信内容\n");
        string smsText = "";
        getline(cin, smsText);
        sendSms(telNumber, smsText, "command");
    }
    else if (StartGuideResult == "4") {
        setupApiPort();
        startSendSmsApi();
    }
    return 0;
}