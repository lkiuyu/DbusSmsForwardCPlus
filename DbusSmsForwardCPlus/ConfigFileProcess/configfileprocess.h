#ifndef CONFIGFILEPROCESS_H
#define CONFIGFILEPROCESS_H

#include <regex>
#include <iostream>
#include <fstream> 
#include "../MyHelper/myhelper.h"

using namespace std;
extern string configFilePath;

map<string, string> readConfigFile();
void writeConfigFile(const map<string, string>& configMap);

// 读取配置文件并存储到一个键值对映射中
map<string, string> readConfigFile() {
    string filename = "";
    if (configFilePath != "") {
        filename = configFilePath;
    }
    else {
        filename = "config.txt";
    }
    map<string, string> configMap;
    ifstream configFile(filename);

    if (configFile.is_open()) {
        string line;
        while (getline(configFile, line)) {
            // 解析每一行的配置项和值
            size_t delimiterPos = line.find('=');
            if (delimiterPos != string::npos) {
                string key = trim(line.substr(0, delimiterPos));
                string value = trim(line.substr(delimiterPos + 1));
                configMap[key] = value;
            }
        }
        configFile.close();
    }
    return configMap;
}
// 将配置项写入配置文件
void writeConfigFile(const map<string, string>& configMap) {
    string filename = "";
    if (configFilePath != "") {
        filename = configFilePath;
    }
    else {
        filename = "config.txt";
    }
    ofstream configFileClear(filename, ofstream::trunc);
    configFileClear.close();
    ofstream configFile(filename);
    if (configFile.is_open())
    {
        for (const auto& pair : configMap) {
            configFile << pair.first << " = " << pair.second << endl;
        }
        configFile.close();
    }
    else {
        printf("无法打开配置文件。\n");
    }
}

//检查配置文件是否存在并初始化
void checkConfig(string configFilePath) {
    //string fileName = "config.txt";
    //ifstream file(configFilePath);
    if (configFilePath == "")
    {
        string fileName = "config.txt";
        ifstream file(fileName);
        if (!file) {
            // 配置文件不存在，创建它
            ofstream configFile("config.txt"); // 创建配置文件
            if (configFile.is_open()) {
                // 写入配置项
                configFile << "smtpHost = " << endl;
                configFile << "smtpPort = " << endl;
                configFile << "emailKey = " << endl;
                configFile << "sendEmial = " << endl;
                configFile << "reciveEmial = " << endl;
                configFile << "pushPlusToken = " << endl;
                configFile << "WeChatQYID = " << endl;
                configFile << "WeChatQYApplicationSecret = " << endl;
                configFile << "WeChatQYApplicationID = " << endl;
                configFile << "TGBotToken = " << endl;
                configFile << "TGBotChatID = " << endl;
                configFile << "IsEnableCustomTGBotApi = " << endl;
                configFile << "CustomTGBotApi = " << endl;
                configFile << "DingTalkAccessToken = " << endl;
                configFile << "DingTalkSecret = " << endl;
                configFile << "BarkUrl = " << endl;
                configFile << "BrakKey = " << endl;
                configFile << "ShellPath = " << endl;
                configFile << "apiPort = " << endl;
                configFile << "ForwardDeviceName = " << endl;
                configFile << "smsCodeKey = 验证码±verification±code±인증±代码±随机码" << endl;
                configFile << "forwardIgnoreStorageType = sm" << endl;
                configFile.close();
            }
            else {
                cout << "无法打开配置文件。" << endl;
            }
        }
    }
}
#endif // CONFIGFILEPROCESS_H