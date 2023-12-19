#ifndef SENDOPTIONPROCESS_H
#define SENDOPTIONPROCESS_H

#include <iostream>
#include "../MyHelper/myhelper.h"
#include "../ProcessSetup/processsetup.h"
#include "smsforwardmethod.h"

using namespace std;

extern vector<string> splitOptionStrings;
void SetupMulitForward();
string sendMethodGuide(string chooseOption);
void processFordardSendSmsOption(string sendMethodGuideResult, string telnum, string smscontent, string smsdate, string ForwardDeviceName);
void fordardSendSms(string sendMethodGuideResult, string telnum, string smscontent, string smsdate);

//初始化设置多渠道转发
void SetupMulitForward() {
    if (splitOptionStrings.size()>0) {
        for (const auto& t : splitOptionStrings) {
            sendMethodGuide(t);
        }
    }
    else {
        printf("请正确输入需要使用的转发渠道编号，以空格分隔（举例：1 2 3 5）\n");
        string mulitForwardChooseOption = "";
        getline(cin, mulitForwardChooseOption);
        string delimiter = " ";
        splitOptionStrings = SplitCodeKeyString(mulitForwardChooseOption, delimiter);
        removeDuplicates(splitOptionStrings);
        bool smfFlag = true;
        for (const auto& t : splitOptionStrings) {
            if (t == "1" || t == "2" || t == "3" || t == "4" || t == "5" || t == "6" || t == "7") {
                continue;
            }
            else {
                smfFlag = false;
                break;
            }
        }
        if (smfFlag) {
            for (const auto& t : splitOptionStrings) {
                sendMethodGuide(t);
            }
        }
        else
        {
            SetupMulitForward();
        }
    }
    
}
//处理用户选择的转发渠道
string sendMethodGuide(string chooseOption)
{
    if (chooseOption == "")
    {
        printf("请选择转发渠道：1.邮箱转发，2.pushplus转发，3.企业微信转发，4.TG机器人转发，5.钉钉转发，6.Bark转发，7.Shell脚本转发，8.自选多渠道转发\n");
        getline(cin, chooseOption);
    }
    if (chooseOption == "1" || chooseOption == "2" || chooseOption == "3" || chooseOption == "4" || chooseOption == "5" || chooseOption == "6" || chooseOption == "7" || chooseOption == "8")
    {
        if (chooseOption == "1")
        {
            SetupEmailInfo();
            return "1";
        }
        else if (chooseOption == "2")
        {
            SetupPushPlusInfo();
            return "2";
        }
        else if (chooseOption == "3")
        {
            SetupWeComInfo();
            return "3";
        }
        else if (chooseOption == "4")
        {
            SetupTGBotInfo();
            return "4";
        }
        else if (chooseOption == "5")
        {
            SetupDingtalkBotMsg();
            return "5";
        }
        else if (chooseOption == "6")
        {
            SetupBarkInfo();
            return "6";
        }
        else if (chooseOption == "7")
        {
            SetupShellInfo();
            return "7";
        }
        else if (chooseOption == "8")
        {
            SetupMulitForward();
            return "8";
        }
        else
        {
            return "";
        }
    }
    else
    {
        printf("请输入1或2或3或4或5或6或7或8\n");
        return sendMethodGuide("");
    }
}
void processFordardSendSmsOption(string sendMethodGuideResult, string telnum, string smscontent, string smsdate, string ForwardDeviceName)
{
    if (sendMethodGuideResult == "1")
    {
        sendByEmail(telnum, smscontent, smsdate, ForwardDeviceName);
    }
    if (sendMethodGuideResult == "2")
    {
        sendByPushPlus(telnum, smscontent, smsdate, ForwardDeviceName);
    }
    if (sendMethodGuideResult == "3")
    {
        sendByWeCom(telnum, smscontent, smsdate, ForwardDeviceName);
    }
    if (sendMethodGuideResult == "4")
    {
        sendByTGBot(telnum, smscontent, smsdate, ForwardDeviceName);
    }
    if (sendMethodGuideResult == "5")
    {
        sendByDingtalkBot(telnum, smscontent, smsdate, ForwardDeviceName);
    }
    if (sendMethodGuideResult == "6")
    {
        sendByBark(telnum, smscontent, smsdate, ForwardDeviceName);
    }
    if (sendMethodGuideResult == "7")
    {
        sendByShell(telnum, smscontent, smsdate, ForwardDeviceName);
    }
    if (sendMethodGuideResult == "8")
    {
        for (const auto& t : splitOptionStrings) {
            processFordardSendSmsOption(t, telnum, smscontent, smsdate, ForwardDeviceName);
        }
    }
}
void fordardSendSms(string sendMethodGuideResult, string telnum, string smscontent, string smsdate)
{
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string ForwardDeviceName = configMap["ForwardDeviceName"];
    if (ForwardDeviceName == "*Host*Name*") {
        ForwardDeviceName = GetDeviceHostName();
    }
    char target = 'T';
    char replacement = ' ';
    replaceChar(smsdate, target, replacement);
    string body = "发信电话:" + telnum + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smscontent + "\n";
    printf(body.c_str());
    processFordardSendSmsOption(sendMethodGuideResult, telnum, smscontent, smsdate, ForwardDeviceName);
}

#endif // SENDOPTIONPROCESS_H

