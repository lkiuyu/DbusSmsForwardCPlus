#ifndef PROCESSSETUP_H
#define PROCESSSETUP_H

#include <regex>
#include <iostream>
#include "../ConfigFileProcess/configfileprocess.h"

using namespace std;

void SetupDeviceName(string deviceName);
void SetupEmailInfo();
void SetupPushPlusInfo();
void SetupWeComInfo();
void SetupTGBotInfo();
void SetupDingtalkBotMsg();
void SetupBarkInfo();
void SetupShellInfo();

//设置转发设备名称
void SetupDeviceName(string deviceName) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string ForwardDeviceName = configMap["ForwardDeviceName"];
    if (ForwardDeviceName == "") {
        configMap.erase("ForwardDeviceName");
        if (deviceName != "") {
            ForwardDeviceName = deviceName;
        }
        else {
            printf("初次运行是否需要设置转发设备名称?(留空回车则默认动态读取设备主机名)：\n");
            getline(cin, ForwardDeviceName);
            if (ForwardDeviceName == "") {
                ForwardDeviceName = "*Host*Name*";
            }
        }
        configMap["ForwardDeviceName"] = ForwardDeviceName;
        writeConfigFile(configMap);
    }
}
//设置Email相关配置
void SetupEmailInfo() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string smtpHost = configMap["smtpHost"];
    string smtpPort = configMap["smtpPort"];
    string emailKey = configMap["emailKey"];
    string sendEmial = configMap["sendEmial"];
    string reciveEmial = configMap["reciveEmial"];
    if (smtpHost == "" && smtpPort == "" && emailKey == "" && sendEmial == "" && reciveEmial == "") {
        configMap.erase("smtpHost");
        configMap.erase("smtpPort");
        configMap.erase("emailKey");
        configMap.erase("sendEmial");
        configMap.erase("reciveEmial");
        printf("首次运行请输入邮箱转发相关配置信息\n请输入smtp地址：\n");
        getline(cin, smtpHost);
        configMap["smtpHost"] = smtpHost;

        printf("请输入smtp端口：\n");
        getline(cin, smtpPort);
        configMap["smtpPort"] = smtpPort;

        printf("请输入邮箱密钥：\n");
        getline(cin, emailKey);
        configMap["emailKey"] = emailKey;

        printf("请输入发件邮箱：\n");
        getline(cin, sendEmial);
        configMap["sendEmial"] = sendEmial;

        printf("请输入收件邮箱：\n");
        getline(cin, reciveEmial);
        configMap["reciveEmial"] = reciveEmial;

        writeConfigFile(configMap);
    }
}
//设置pushplus相关配置
void SetupPushPlusInfo() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string pushPlusToken = configMap["pushPlusToken"];
    if (pushPlusToken == "") {
        configMap.erase("pushPlusToken");
        printf("首次运行请输入PushPlusToken\n");
        getline(cin, pushPlusToken);
        configMap["pushPlusToken"] = pushPlusToken;
        writeConfigFile(configMap);
    }
}
//设置企业微信相关配置
void SetupWeComInfo() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string corpid = configMap["WeChatQYID"];
    string appsecret = configMap["WeChatQYApplicationSecret"];
    string appid = configMap["WeChatQYApplicationID"];

    if (corpid == "" && appsecret == "" && appid == "") {
        configMap.erase("WeChatQYID");
        configMap.erase("WeChatQYApplicationSecret");
        configMap.erase("WeChatQYApplicationID");
        printf("首次运行请输入企业ID\n");
        getline(cin, corpid);
        configMap["WeChatQYID"] = corpid;
        printf("请输入自建应用ID\n");
        getline(cin, appid);
        configMap["WeChatQYApplicationID"] = appid;
        printf("请输入自建应用密钥\n");
        getline(cin, appsecret);
        configMap["WeChatQYApplicationSecret"] = appsecret;
        writeConfigFile(configMap);
    }
}
//设置TelegramBot相关配置
void SetupTGBotInfo() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string TGBotToken = configMap["TGBotToken"];
    string TGBotChatID = configMap["TGBotChatID"];
    string IsEnableCustomTGBotApi = configMap["IsEnableCustomTGBotApi"];
    string CustomTGBotApi = configMap["CustomTGBotApi"];

    if (TGBotToken == "" && TGBotChatID == "" && IsEnableCustomTGBotApi == "") {
        configMap.erase("TGBotToken");
        configMap.erase("TGBotChatID");
        configMap.erase("IsEnableCustomTGBotApi");
        configMap.erase("CustomTGBotApi");

        printf("首次运行请输入TG机器人Token\n");
        getline(cin, TGBotToken);
        configMap["TGBotToken"] = TGBotToken;
        printf("请输入机器人要转发到的ChatId\n");
        getline(cin, TGBotChatID);
        configMap["TGBotChatID"] = TGBotChatID;
        string customApiEnableInput = "";
        do
        {
            printf("是否需要使用自定义api(1.使用 2.不使用)\n");
            getline(cin, customApiEnableInput);
        } while (!(customApiEnableInput == "1" || customApiEnableInput == "2"));
        if (customApiEnableInput == "1")
        {
            IsEnableCustomTGBotApi = "true";
            configMap["IsEnableCustomTGBotApi"] = IsEnableCustomTGBotApi;
            printf("请输入机器人自定义api(格式https://xxx.abc.com)\n");
            getline(cin, CustomTGBotApi);
            configMap["CustomTGBotApi"] = CustomTGBotApi;
        }
        else
        {
            IsEnableCustomTGBotApi = "false";
            configMap["IsEnableCustomTGBotApi"] = IsEnableCustomTGBotApi;
        }
        writeConfigFile(configMap);
    }
}
//设置DingTalkBot相关配置
void SetupDingtalkBotMsg() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string DingTalkAccessToken = configMap["DingTalkAccessToken"];
    string DingTalkSecret = configMap["DingTalkSecret"];
    if (DingTalkAccessToken == "" && DingTalkSecret == "") {
        configMap.erase("DingTalkAccessToken");
        configMap.erase("DingTalkSecret");
        printf("首次运行请输入钉钉机器人AccessToken\n");
        getline(cin, DingTalkAccessToken);
        configMap["DingTalkAccessToken"] = DingTalkAccessToken;
        printf("请输入钉钉机器人加签secret\n");
        getline(cin, DingTalkSecret);
        configMap["DingTalkSecret"] = DingTalkSecret;
        writeConfigFile(configMap);
    }
}
//设置Bark相关配置
void SetupBarkInfo() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();

    string BarkUrl = configMap["BarkUrl"];
    string BrakKey = configMap["BrakKey"];

    if (BarkUrl == "" && BrakKey == "") {
        configMap.erase("BarkUrl");
        configMap.erase("BrakKey");
        printf("首次运行请输入Bark服务器地址\n");
        getline(cin, BarkUrl);
        configMap["BarkUrl"] = BarkUrl;
        printf("请输入Bark推送key\n");
        getline(cin, BrakKey);
        configMap["BrakKey"] = BrakKey;
        writeConfigFile(configMap);
    }
}
//设置shell相关配置
void SetupShellInfo() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string ShellPath = configMap["ShellPath"];

    if (ShellPath == "") {
        configMap.erase("ShellPath");
        printf("首次运行请输入shell脚本路径\n");
        getline(cin, ShellPath);
        configMap["ShellPath"] = ShellPath;
        writeConfigFile(configMap);
    }
}

#endif // PROCESSSETUP_H
