#include <dbus/dbus.h>
#include <iostream>
#include <thread>
#include <cstdio>
#include <curl/curl.h>
#include <fstream> 
#include <map>
#include <chrono>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "mail.h"
#include <cassert>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <regex>
#include "httplib.h"
#include <thread>

using namespace httplib;
using namespace std;

string trim(string strinput)
{
    if (!strinput.empty())
    {
        strinput.erase(0, strinput.find_first_not_of(" "));
        strinput.erase(strinput.find_last_not_of(" ") + 1);
    }
    return strinput;
}
void replaceChar(string& str, char targetChar, char replacementChar) {
    for (char& c : str) {
        if (c == targetChar) {
            c = replacementChar;
        }
    }
}
void replaceString(string& str, string targetStr, string replacementStr) {
    size_t index = str.find(targetStr);
    if (index != string::npos) {
        str.replace(index, targetStr.length(), replacementStr);
    }
}
vector<string> SplitCodeKeyString(const string& str, const string& delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;

    while (getline(ss, token, delimiter[0])) {
        replaceString(token, "\261", "");
        tokens.push_back(token);
    }
    return tokens;
}
vector<string> splitCNString(const string& input, const string& delimiter) {
    vector<string> result;
    size_t startPos = 0;
    size_t endPos = input.find(delimiter);
    while (endPos != string::npos) {
        string substr = input.substr(startPos, endPos - startPos);
        result.push_back(substr);
        startPos = endPos + delimiter.length();
        endPos = input.find(delimiter, startPos);
    }
    // Add the remaining part of the input string
    string substr = input.substr(startPos);
    result.push_back(substr);
    return result;
}
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* response)
{
    size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}
unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}
string UrlEncodeUTF8(const string& str) {
    stringstream encodedStr;
    encodedStr << hex << uppercase << setfill('0');

    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encodedStr << c;
        }
        else {
            encodedStr << '%' << setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(c));
        }
    }

    return encodedStr.str();
}
string UrlEncode(const string& str)
{
    string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}
string UrlDecode(const string& str)
{
    string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}
vector<string> extractAllContent(const string& input) {
    vector<string> extractedContents;
    size_t startPos = 0;
    while (true) {
        size_t openingPos = input.find("【", startPos);
        size_t closingPos = input.find("】", startPos);
        if (openingPos != string::npos && closingPos != string::npos && openingPos < closingPos) {
            openingPos += 3; // Adjust to skip the opening bracket "【" (3 characters)
            string content = input.substr(openingPos, closingPos - openingPos);
            extractedContents.push_back(content);
            startPos = closingPos + 3; // Move startPos to the character after the closing bracket "】" (3 characters)
        }
        else {
            break; // Break the loop if no more brackets are found or in the wrong order
        }
    }
    return extractedContents;
}
string configFilePath = "";
// 读取配置文件并存储到一个键值对映射中
map<string, string> readConfigFile() {
    string filename = "";
    if (configFilePath!="") {
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
        cout << "无法打开配置文件。" << endl;
    }
}
//判断短信内是否含有验证码
bool JudgeSmsContentHasCode(string& smscontent) {
    string filename = "config.txt";
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string smsKeysStr = configMap["smsCodeKey"];
    if (smsKeysStr=="") {
        smsKeysStr = "验证码±verification±code±인증±代码±随机码";
        configMap.erase("smsCodeKey");
        configMap["smsCodeKey"] = smsKeysStr;
        writeConfigFile(configMap);
    }
    string delimiter = "±";
    vector<string> splitStrings = SplitCodeKeyString(smsKeysStr, delimiter);
    for (const auto& t : splitStrings) {
        size_t index = smscontent.find(t);
        if (index != string::npos) {
            replaceString(smscontent, t, " " + t + " ");
            return true;
        }
        //cout << t << endl;
    }
    return false;
}
int CountDigits(const string& str) {
    int digitCount = 0;
    for (char c : str) {
        if (isdigit(c)) {
            digitCount++;
        }
    }
    return digitCount;
}
//获取验证码
string GetCode(const string& smsContent) {
    string pattern = R"(\b[A-Za-z0-9]{4,7}\b)";
    regex regexPattern(pattern);

    sregex_iterator iter(smsContent.begin(), smsContent.end(), regexPattern);
    sregex_iterator end;
    vector<string> matchs;
    while (iter != end)
    {
        for (unsigned i = 0; i < iter->size(); ++i)
        {
            matchs.push_back((*iter)[i]);
        }
        ++iter;
    }
    if (matchs.size() > 1) {
        int maxDigits = 0;
        string maxDigitsString = "";
        for (const auto& match : matchs) {
            string currentString = match;
            int digitCount = CountDigits(currentString);
            if (digitCount > maxDigits) {
                maxDigits = digitCount;
                maxDigitsString = currentString;
            }
        }
        return maxDigitsString;
    }
    else if (matchs.size() == 1) {
        return matchs[0];
    }
    else {
        return "";
    }
}
//获取验证码来源
string GetCodeSmsFrom(const string& smsContent) {

    vector<string> extractedContents = extractAllContent(smsContent);
    for (const auto& content : extractedContents) {
        string delimiter = content;
        vector<string> splitStrings = splitCNString(smsContent, delimiter);
        if (splitStrings[0]=="【") {
            return "【"+ content +"】";
        }
        else if (splitStrings[splitStrings.size()-1] == "】") {
            return "【" + content + "】";
        }
        //cout << "Extracted content: " << content << endl;
    }
    return "";
}
//获取组合的验证码来源和验证码
string GetSmsCodeStr(string smscontent, string& smscode, string& CodeFrom) {
    smscontent = trim(smscontent);
    if (JudgeSmsContentHasCode(smscontent)) {
        smscode = trim(GetCode(smscontent));
        if (smscode != "")
        {
            CodeFrom = GetCodeSmsFrom(smscontent);
            return CodeFrom + smscode;
        }
    }
    return "";
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
    if (smtpHost == ""&& smtpPort == "" && emailKey == "" && sendEmial == "" && reciveEmial == "") {
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
//使用Email转发消息
void sendByEmail(string smsnumber, string smstext, string smsdate) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string smtpHost = configMap["smtpHost"];
    int smtpPort = 0;
    sscanf(configMap["smtpPort"].c_str(), "%d", &smtpPort);
    string emailKey = configMap["emailKey"];
    string sendEmial = configMap["sendEmial"];
    string reciveEmial = configMap["reciveEmial"];
    string emailcontent = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    string subject = "短信转发" + smsnumber;
    if (SmsCodeStr != "") {
        subject = SmsCodeStr + " " + subject;
    }
    string smtpserver = "smtps://" + smtpHost ;
    //printf(smtpserver.c_str());
    string from = sendEmial;
    string passs = emailKey;//这里替换成自己的授权码
    string to = reciveEmial;
    string strMessage = emailcontent;
    vector<string> vecTo; //发送列表
    vecTo.push_back(reciveEmial);
    vector<string> ccList;
    // ccList.push_back("xxx@xxx.com.cn");//抄送列表
    vector<string> attachment;

    SmtpBase* base;
    //SimpleSmtpEmail m_mail(smtpHost, configMap["smtpPort"]);
   // base = &m_mail;
   // base->SendEmail(from, passs, to, subject, strMessage);//普通的文本发送，明文发送

    SimpleSslSmtpEmail m_ssl_mail(smtpHost, "465");
    base = &m_ssl_mail;
    base->SendEmail(from, passs, to, subject, strMessage);
    //base->SendEmail(from, passs, vecTo, subject, strMessage, attachment, ccList);//加密的发送，支持抄送、附件等


    /*string smtp_server { smtpHost };
    int port{ smtpPort }; 
    string
        sender { sendEmial }, 
        recipient{ reciveEmial },
        subject{ "短信转发"+ smsnumber },
        content{ emailcontent }, 
        auth_code{ emailKey };
    try
    {
        Poco::Net::MailMessage message;
        message.setSender(sender);
        message.addRecipient(Poco::Net::MailRecipient(Poco::Net::MailRecipient::PRIMARY_RECIPIENT, recipient));
        message.setSubject(subject);
        message.setContent(content);
        Poco::Net::SecureSMTPClientSession smtp(smtp_server, port);
        smtp.login(Poco::Net::SMTPClientSession::LoginMethod::AUTH_LOGIN, sender, auth_code);
        smtp.sendMessage(message);
        smtp.close();
    }
    catch (Poco::Net::NetException& e) {
        cerr << "error: " << e.displayText() << endl;
    }
    cout << "email sent successfully!" << endl;*/

}

//设置pushplus相关配置
void SetupPushPlusInfo() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string pushPlusToken = configMap["pushPlusToken"];
    if (pushPlusToken=="") {
        configMap.erase("pushPlusToken");
        printf("首次运行请输入PushPlusToken\n");
        getline(cin, pushPlusToken);
        configMap["pushPlusToken"] = pushPlusToken;
        writeConfigFile(configMap);
    }
}
//使用pushplus转发消息
void sendByPushPlus(string smsnumber, string smstext, string smsdate) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string pushPlusToken = configMap["pushPlusToken"];
    // PushPlus的API端点URL
    string apiUrl = "https://www.pushplus.plus/send";
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);

    // 推送消息的标题和内容
    string title = "短信转发" + smsnumber;
    if (SmsCodeStr!="") {
        title = SmsCodeStr + " " + title;
    }
    
    string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;

    // 构建POST请求的数据
    string postData = "token=" + pushPlusToken + "&title=" + title + "&content=" + smstext;
    // 初始化cURL库
    CURL* curl = curl_easy_init();
    if (curl)
    {
        // 设置POST请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        // 设置响应数据的回调函数
        string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << endl;
            }
            int code = doc["code"].GetInt();
            string errmsg = doc["msg"].GetString(); 
            if (code == 200)
            {
                printf("pushplus转发成功\n");
            }
            else
            {
                printf(errmsg.c_str());
            }
        }
        else
        {
            // HTTP请求失败，输出错误信息
            cerr << "Failed to send PushPlus message: " << curl_easy_strerror(res) << endl;
        }
        // 释放cURL资源
        curl_easy_cleanup(curl);
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

    if (corpid == ""&& appsecret == "" && appid == "") {
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
//使用企业微信转发消息
void sendByWeCom(string smsnumber, string smstext, string smsdate) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();

    string corpid = configMap["WeChatQYID"];
    string corpsecret = configMap["WeChatQYApplicationSecret"];
    int agentid = 0;
    sscanf(configMap["WeChatQYApplicationID"].c_str(), "%d", &agentid);
    string url = "https://qyapi.weixin.qq.com/cgi-bin/gettoken?corpid=" + corpid + "&corpsecret=" + corpsecret;
    string smscontent = "短信转发\n发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    if (SmsCodeStr != "") {
        smscontent = SmsCodeStr + "\n" + smscontent;
    }
    CURL* curl = curl_easy_init();
    if (curl)
    {
        // 设置POST请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // 设置响应数据的回调函数
        string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << endl;
            }
            int errcode = doc["errcode"].GetInt();
            string errmsg = doc["errmsg"].GetString();
            if (errcode == 0 && errmsg=="ok")
            {
                string access_token = doc["access_token"].GetString();
                rapidjson::Document jobj;
                jobj.SetObject();
                // 添加键值对
                rapidjson::Document::AllocatorType& allocator = jobj.GetAllocator();
                jobj.AddMember("touser", "@all", allocator);
                jobj.AddMember("toparty", "", allocator);
                jobj.AddMember("totag", "", allocator);
                jobj.AddMember("msgtype", "text", allocator);
                jobj.AddMember("agentid", agentid, allocator);

                rapidjson::Document jobj1;
                jobj1.SetObject();
                // 添加键值对
                rapidjson::Document::AllocatorType& allocator1 = jobj1.GetAllocator();
                string smsbody =smscontent;
                rapidjson::Value v(rapidjson::kStringType);
                v.SetString(smsbody.c_str(), smsbody.length(), allocator1);
                jobj1.AddMember("content", v, allocator1);
                jobj.AddMember("text", jobj1, allocator);
                jobj.AddMember("safe", 0, allocator);
                jobj.AddMember("enable_id_trans", 0, allocator);
                jobj.AddMember("enable_duplicate_check", 0, allocator);
                jobj.AddMember("duplicate_check_interval", 1800, allocator);
               
                string msgurl = "https://qyapi.weixin.qq.com/cgi-bin/message/send?access_token=" + access_token;
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                jobj.Accept(writer);
                string jsonData = buffer.GetString();

                CURL* curl2 = curl_easy_init();
                if (curl2) {
                    // 设置POST请求的URL和数据
                    curl_easy_setopt(curl2, CURLOPT_URL, msgurl.c_str());
                    curl_easy_setopt(curl2, CURLOPT_POSTFIELDS, jsonData.c_str());
                    curl_easy_setopt(curl2, CURLOPT_POSTFIELDSIZE, jsonData.length());
                    // 设置响应数据的回调函数
                    string response2;
                    curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl2, CURLOPT_WRITEDATA, &response2);
                    // 执行HTTP请求
                    CURLcode res2 = curl_easy_perform(curl2);
                    if (res2 == CURLE_OK) {
                        rapidjson::Document doc2;
                        doc2.Parse(response2.c_str());
                        if (doc2.HasParseError()) {
                            cout << "Failed to parse JSON. Error code: " << doc2.GetParseError() << ", "
                                << rapidjson::GetParseError_En(doc2.GetParseError()) << endl;
                        }
                        int errcode1 = doc2["errcode"].GetInt();
                        string errmsg1 = doc2["errmsg"].GetString();
                        if (errcode1 == 0 && errmsg1 == "ok")
                        {
                            printf("企业微信转发成功\n");
                        }
                        else
                        {
                            printf(errmsg1.c_str());
                        }
                    }
                    else
                    {
                        // HTTP请求失败，输出错误信息
                        cerr << "Failed to send WeCom message: " << curl_easy_strerror(res2) << endl;
                    }
                    curl_easy_cleanup(curl2);
                }
            }
            else
            {
                printf(errmsg.c_str());
            }
        }
        else
        {
            // HTTP请求失败，输出错误信息
            cerr << "Failed to send WeCom message: " << curl_easy_strerror(res) << endl;
        }
        // 释放cURL资源
        curl_easy_cleanup(curl);
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

    if (TGBotToken == ""&& TGBotChatID == "" && IsEnableCustomTGBotApi == "") {
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
//使用TelegramBot转发消息
void sendByTGBot(string smsnumber, string smstext, string smsdate) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string TGBotToken = configMap["TGBotToken"];
    string TGBotChatID = configMap["TGBotChatID"];
    string IsEnableCustomTGBotApi = configMap["IsEnableCustomTGBotApi"];
    string CustomTGBotApi = configMap["CustomTGBotApi"];
    string url = "";
    if (IsEnableCustomTGBotApi == "true")
    {
        url = CustomTGBotApi;
    }
    else
    {
        url = "https://api.telegram.org";
    }
    url += "/bot" + TGBotToken + "/sendMessage?chat_id=" + TGBotChatID + "&text=";
    string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    if (SmsCodeStr != "") {
        url += UrlEncode(SmsCodeStr + "\n短信转发\n" + content);
    }
    else {
        url += UrlEncode("短信转发\n" + content);
    }
    //cout << url << endl;

    CURL* curl = curl_easy_init();
    if (curl) {
        // 设置Get请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // 设置响应数据的回调函数
        string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << endl;
            }
            bool status = doc["ok"].GetBool();
            if (status)
            {
                printf("TGBot转发成功\n");
            }
            else
            {
                string description = doc["description"].GetString();
                printf(description.c_str());
            }
        }
        curl_easy_cleanup(curl);
    }
    
}

//设置DingTalkBot相关配置
void SetupDingtalkBotMsg() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string DingTalkAccessToken = configMap["DingTalkAccessToken"];
    string DingTalkSecret = configMap["DingTalkSecret"];
    if (DingTalkAccessToken == ""&& DingTalkSecret == "") {
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
//使用DingTalkBot转发消息
void sendByDingtalkBot(string smsnumber, string smstext, string smsdate) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string DingTalkAccessToken = configMap["DingTalkAccessToken"];
    string DingTalkSecret = configMap["DingTalkSecret"];
    string DING_TALK_BOT_URL = "https://oapi.dingtalk.com/robot/send?access_token=";
    string url = DING_TALK_BOT_URL + DingTalkAccessToken;
    auto now = chrono::system_clock::now();
    auto timestamp = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
    string timestamp1=to_string(timestamp);
    string stringToSign = to_string(timestamp) + "\n" + DingTalkSecret;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength;
    HMAC(EVP_sha256(), DingTalkSecret.c_str(), DingTalkSecret.length(),
        reinterpret_cast<const unsigned char*>(stringToSign.c_str()), stringToSign.length(),
        digest, &digestLength);
    string hmacresult(reinterpret_cast<char*>(digest), digestLength);
    BIO* bmem = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bmem);
    BIO_write(b64, hmacresult.c_str(), hmacresult.length());
    BIO_flush(b64);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bmem, &bptr);
    string base64result(bptr->data, bptr->length);
    BIO_free_all(bmem);
    string sign = UrlEncodeUTF8(base64result);
    url += "&timestamp=" + timestamp1 + "&sign="+ sign;
    string smscontent = "短信转发\n发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    if (SmsCodeStr != "") {
        smscontent = SmsCodeStr + "\n" + smscontent;
    }
    rapidjson::Document msgContent;
    msgContent.SetObject();
    // 添加键值对
    rapidjson::Document::AllocatorType& allocator = msgContent.GetAllocator();
    string smsbody = smscontent;
    rapidjson::Value v(rapidjson::kStringType);
    v.SetString(smsbody.c_str(), smsbody.length(), allocator);
    msgContent.AddMember("content", v, allocator);
    rapidjson::Document msgObj;
    msgObj.SetObject();
    // 添加键值对
    rapidjson::Document::AllocatorType& allocator1 = msgObj.GetAllocator();
    msgObj.AddMember("msgtype", "text", allocator1);
    msgObj.AddMember("text", msgContent, allocator1);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msgObj.Accept(writer);
    string jsonData = buffer.GetString();
    CURL* curl = curl_easy_init();
    if (curl) {
        // 设置POST请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // 设置请求头的 Content-Type
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        // 设置响应数据的回调函数
        string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << endl;
            }
            int errcode1 = doc["errcode"].GetInt();
            string errmsg1 = doc["errmsg"].GetString();
            if (errcode1 == 0 && errmsg1 == "ok")
            {
                printf("钉钉转发成功\n");
            }
            else
            {
                printf(errmsg1.c_str());
            }
        }
        curl_easy_cleanup(curl);
    }
}

//设置Bark相关配置
void SetupBarkInfo() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();

    string BarkUrl = configMap["BarkUrl"];
    string BrakKey = configMap["BrakKey"];

    if (BarkUrl == ""&& BrakKey == "") {
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
//使用Bark转发消息
void sendByBark(string smsnumber, string smstext, string smsdate) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string BarkUrl = configMap["BarkUrl"];
    string BrakKey = configMap["BrakKey"];
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    string title = "短信转发" + smsnumber;
    string url = BarkUrl + "/" + BrakKey + "/";
    string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    url += UrlEncode(content);
    if (SmsCodeStr != "") {
        title = SmsCodeStr + " " + title;
        url += "?group=" + smsnumber + "&title=" + UrlEncode(title) + "&autoCopy=1&copy=" + SmsCode;
    }
    else
    {
        url += "?group=" + smsnumber + "&title=" + title;
    }
    cout << url << endl;

    CURL* curl = curl_easy_init();
    if (curl) {
        // 设置Get请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // 设置响应数据的回调函数
        string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << endl;
            }
            int status = doc["code"].GetInt();
            if (status == 200)
            {
                printf("Bark转发成功\n");
            }
            else
            {
                string rmsg = doc["message"].GetString();
                printf(rmsg.c_str());
               
            }
        }
        curl_easy_cleanup(curl);
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
//使用shell转发消息
void sendByShell(string smsnumber, string smstext, string smsdate) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string ShellPath = configMap["ShellPath"];
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);

    ShellPath += " \"" + smsnumber + "\" \"" + smsdate +"\" \""+ smstext+"\" \""+ SmsCode+"\" \""+ SmsCodeFrom+"\"";

    int result = system(ShellPath.c_str());
    if (result == 0) {
        cout << "Shell script executed successfully." << endl;
    }
    else {
        cout << "Failed to execute shell script." << endl;
    }
}


//处理用户选择的转发渠道
string sendMethodGuide(string chooseOption)
{
    if (chooseOption == "")
    {
        printf("请选择转发渠道：1.邮箱转发，2.pushplus转发，3.企业微信转发，4.TG机器人转发，5.钉钉转发，6.Bark转发，7.Shell脚本转发\n");
        getline(cin, chooseOption);
    }
    if (chooseOption == "1" || chooseOption == "2" || chooseOption == "3" || chooseOption == "4" || chooseOption == "5" || chooseOption == "6" || chooseOption == "7")
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
        else
        {
            return "";
        }
    }
    else
    {
        printf("请输入1或2或3或4或5或6或7\n");
        return sendMethodGuide("");
    }
}
void fordardSendSms(string sendMethodGuideResult, string telnum, string smscontent, string smsdate)
{
    char target = 'T';
    char replacement = ' ';
    replaceChar(smsdate, target, replacement);
    string body = "发信电话:" + telnum + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smscontent + "\n";
    printf(body.c_str());
    if (sendMethodGuideResult == "1")
    {
        sendByEmail(telnum, smscontent, smsdate);
    }
    if (sendMethodGuideResult == "2")
    {
        sendByPushPlus(telnum, smscontent, smsdate);
    }
    if (sendMethodGuideResult == "3")
    {
        sendByWeCom(telnum, smscontent, smsdate);
    }
    if (sendMethodGuideResult == "4")
    {
        sendByTGBot(telnum, smscontent, smsdate);
    }
    if (sendMethodGuideResult == "5")
    {
        sendByDingtalkBot(telnum, smscontent, smsdate);
    }
    if (sendMethodGuideResult == "6")
    {
        sendByBark(telnum, smscontent, smsdate);
    }
    if (sendMethodGuideResult == "7")
    {
        sendByShell(telnum, smscontent, smsdate);
    }
}
string onStartGuide(string chooseOption)
{
    if (chooseOption=="")
    {
        printf("请选择运行模式：1为短信转发模式，2为发短信模式，3为短信转发模式并开启发短信webapi接口，4为只运行发短信webapi接口\n");
        getline(cin, chooseOption);
    }
    if (chooseOption == "1" || chooseOption == "2" || chooseOption == "3" || chooseOption == "4")
    {
        return chooseOption;
    }
    else
    {
        printf("请输入1或2或3或4\n");
        return onStartGuide("");
    }

}

void getAndSendSmsContent(string sendMethodGuideResult, const char *smsPath) {
    DBusError GetSmsContentError;
    dbus_error_init(&GetSmsContentError);
    DBusConnection* GetSmsContentConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &GetSmsContentError);
    if (dbus_error_is_set(&GetSmsContentError))
    {
        printf("DBus connection error::\n%s\n", GetSmsContentError.message);
        dbus_error_free(&GetSmsContentError);
    }
    // 创建方法调用sms消息
    DBusMessage* smsContentMessage = dbus_message_new_method_call(
        "org.freedesktop.ModemManager1",  // 目标接口
        smsPath, // 目标路径
        "org.freedesktop.DBus.Properties", // 接口名称
        "GetAll" // 方法名称
    );
    const char* interfaceName = "org.freedesktop.ModemManager1.Sms";
    dbus_message_append_args(smsContentMessage, DBUS_TYPE_STRING, &interfaceName, DBUS_TYPE_INVALID);

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(GetSmsContentConnection, smsContentMessage, -1, &GetSmsContentError);

    DBusMessageIter CallOutIter;
    dbus_message_iter_init(reply, &CallOutIter);
    int returnType = dbus_message_iter_get_arg_type(&CallOutIter);
    const char* telnum;
    const char* smsdate;
    const char* smscontent;
    if (returnType == DBUS_TYPE_ARRAY)
    {
        // 迭代处理数组中的字典项
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&CallOutIter, &arrayIter);
        while (dbus_message_iter_get_arg_type(&arrayIter) != DBUS_TYPE_INVALID)
        {
            // 处理字典项
            if (dbus_message_iter_get_arg_type(&arrayIter) == DBUS_TYPE_DICT_ENTRY)
            {
                DBusMessageIter dictIter;
                dbus_message_iter_recurse(&arrayIter, &dictIter);
                const char* key;
                // 处理键
                if (dbus_message_iter_get_arg_type(&dictIter) == DBUS_TYPE_STRING)
                {
                    dbus_message_iter_get_basic(&dictIter, &key);
                }
                string keyName(key);
                if (keyName == "Number") {
                    // 移动到字典项的值
                    dbus_message_iter_next(&dictIter);
                    DBusMessageIter variantIter;
                    dbus_message_iter_recurse(&dictIter, &variantIter);
                    dbus_message_iter_get_basic(&variantIter, &telnum);
                }
                else if (keyName == "Text") {
                    // 移动到字典项的值
                    dbus_message_iter_next(&dictIter);
                    DBusMessageIter variantIter;
                    dbus_message_iter_recurse(&dictIter, &variantIter);
                    dbus_message_iter_get_basic(&variantIter, &smscontent);
                }
                else if (keyName == "Timestamp") {
                    // 移动到字典项的值
                    dbus_message_iter_next(&dictIter);
                    DBusMessageIter variantIter;
                    dbus_message_iter_recurse(&dictIter, &variantIter);
                    dbus_message_iter_get_basic(&variantIter, &smsdate);
                }
            }
            // 移动到下一个字典项
            dbus_message_iter_next(&arrayIter);
        }
    }
    if (string(smscontent)!="") {

        fordardSendSms(sendMethodGuideResult, string(telnum), string(smscontent), string(smsdate));
        // 释放消息资源
        dbus_message_unref(reply);
        dbus_message_unref(smsContentMessage);
        // 关闭DBus连接
        dbus_connection_unref(GetSmsContentConnection);
    }
    else
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        getAndSendSmsContent(sendMethodGuideResult,smsPath);
    }
}
//处理dbus获取到的消息并转发
void parseDBusMessageAndSend(DBusMessage* message,string sendMethodGuideResult)
{
    // 获取接口名称
    const char* interface = dbus_message_get_interface(message);
    string interfaceName(interface);

    if (interfaceName == "org.freedesktop.ModemManager1.Modem.Messaging") {
        // 获取信号名称
        const char* signal = dbus_message_get_member(message);
        string signalName(signal);
        if (signalName == "Added")
        {
            dbus_bool_t isReceived;
            const char* smsPath;

            // 提取对象参数
            DBusMessageIter iter;
            if (dbus_message_iter_init(message, &iter))
            {
                int argType = dbus_message_iter_get_arg_type(&iter);
                if (argType == DBUS_TYPE_OBJECT_PATH)
                {
                    dbus_message_iter_get_basic(&iter, &smsPath);
                }
            }

            // 提取布尔参数
            if (dbus_message_iter_next(&iter))
            {
                int argType = dbus_message_iter_get_arg_type(&iter);
                if (argType == DBUS_TYPE_BOOLEAN)
                {
                    dbus_message_iter_get_basic(&iter, &isReceived);
                }
            }

            if (isReceived) {
                printf("SmsPath:\n%s\n", smsPath);
                getAndSendSmsContent(sendMethodGuideResult, smsPath);
            }
        }
    }
}
//监视dbus
void monitorDbus(string sendMethodGuideResult) {
    printf("正在运行. 按下 Ctrl-C 停止.\n");
    DBusError error;
    DBusConnection* connection;
    dbus_error_init(&error);
    // 连接到DBus会话总线
    connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error))
    {
        cerr << "DBus connection error: " << error.message << endl;
        dbus_error_free(&error);
    }
    // 监听DBus消息
    dbus_bus_add_match(connection, "type='signal',path='/org/freedesktop/ModemManager1/Modem/0',interface='org.freedesktop.ModemManager1.Modem.Messaging'", &error);
    dbus_connection_flush(connection);
    // 循环接收DBus消息
    while (true)
    {
        dbus_connection_read_write(connection, 0);
        DBusMessage* message = dbus_connection_pop_message(connection);
        if (message != NULL)
        {
            parseDBusMessageAndSend(message, sendMethodGuideResult);
            // 处理接收到的DBus消息
            // 在这里进行你的处理逻辑
            dbus_message_unref(message);
        }
        else
        {
            // 休眠一段时间，避免CPU占用过高
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
    // 断开DBus连接
    dbus_connection_unref(connection);
}
//短信发送方法
void sendSms(string telNumber, string smsText,string target) {
    string smsSavePath = "";
    DBusError CreateSmsContentError;
    dbus_error_init(&CreateSmsContentError);
    DBusConnection* CreateSmsContentConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &CreateSmsContentError);
    if (dbus_error_is_set(&CreateSmsContentError))
    {
        printf("DBus connection error::\n%s\n", CreateSmsContentError.message);
        dbus_error_free(&CreateSmsContentError);
    }
    // 创建方法调用sms消息
    DBusMessage* CreateSmsContentMessage = dbus_message_new_method_call(
        "org.freedesktop.ModemManager1",  // 目标接口
        "/org/freedesktop/ModemManager1/Modem/0", // 目标路径
        "org.freedesktop.ModemManager1.Modem.Messaging", // 接口名称
        "Create" // 方法名称
    );
    DBusMessageIter iter;
    dbus_message_iter_init_append(CreateSmsContentMessage, &iter);
    DBusMessageIter arrayIter;
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &arrayIter);
    // 创建短信内容dict entry
    DBusMessageIter dictEntry1Iter;
    dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_DICT_ENTRY, nullptr, &dictEntry1Iter);
    // 添加 dict entry 的键
    const char* key1 = "text";
    dbus_message_iter_append_basic(&dictEntry1Iter, DBUS_TYPE_STRING, &key1);
    // 添加 dict entry 的值（Variant）
    DBusMessageIter variantIter;
    dbus_message_iter_open_container(&dictEntry1Iter, DBUS_TYPE_VARIANT, "s", &variantIter);
    // 添加 Variant 的值（短信内容）
    const char* value1 = smsText.c_str();
    dbus_message_iter_append_basic(&variantIter, DBUS_TYPE_STRING, &value1);
    dbus_message_iter_close_container(&dictEntry1Iter, &variantIter);
    dbus_message_iter_close_container(&arrayIter, &dictEntry1Iter);

    // 创建电话号码dict entry
    DBusMessageIter dictEntry2Iter;
    dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_DICT_ENTRY, nullptr, &dictEntry2Iter);
    // 添加 dict entry 的键
    const char* key2 = "number";
    dbus_message_iter_append_basic(&dictEntry2Iter, DBUS_TYPE_STRING, &key2);
    // 添加 dict entry 的值（Variant）
    DBusMessageIter variantIter2;
    dbus_message_iter_open_container(&dictEntry2Iter, DBUS_TYPE_VARIANT, "s", &variantIter2);
    // 添加 Variant 的值（电话号码）
    const char* value2 = telNumber.c_str();
    dbus_message_iter_append_basic(&variantIter2, DBUS_TYPE_STRING, &value2);
    dbus_message_iter_close_container(&dictEntry2Iter, &variantIter2);
    dbus_message_iter_close_container(&arrayIter, &dictEntry2Iter);

    dbus_message_iter_close_container(&iter, &arrayIter);
    DBusPendingCall* pendingCall;
    dbus_connection_send_with_reply(CreateSmsContentConnection, CreateSmsContentMessage, &pendingCall, -1);
    dbus_connection_flush(CreateSmsContentConnection);
    dbus_message_unref(CreateSmsContentMessage);
    // 等待回复
    dbus_pending_call_block(pendingCall);
    // 获取回复消息
    DBusMessage* reply = dbus_pending_call_steal_reply(pendingCall);
    if (reply) {
        if (dbus_message_is_error(reply, DBUS_ERROR_UNKNOWN_METHOD)) {
            cerr << "Method call failed: Unknown method" << endl;
        }
        else {
            // 提取对象路径
            const char* objectPath;
            dbus_message_get_args(reply, &CreateSmsContentError, DBUS_TYPE_OBJECT_PATH, &objectPath, DBUS_TYPE_INVALID);
            if (dbus_error_is_set(&CreateSmsContentError)) {
                cerr << "Failed to extract object path from reply: " << CreateSmsContentError.message << endl;
                dbus_error_free(&CreateSmsContentError);
            }
            else {
                smsSavePath = objectPath;
            }
        }
        dbus_message_unref(reply);
    }
    else {
        cerr << "Failed to get a reply." << endl;
    }
    dbus_pending_call_unref(pendingCall);
    // 关闭连接
    dbus_connection_unref(CreateSmsContentConnection);
    if (smsSavePath != "") {
        string sendChoise = "";
        if (target=="command") {
            printf("短信创建成功，是否发送？(1.发送短信,其他按键退出程序)\n");
            getline(cin, sendChoise);
        }
        
        if (sendChoise == "1"|| target == "api")
        {
            DBusError SendSmsContentError;
            dbus_error_init(&SendSmsContentError);
            DBusConnection* SendSmsContentConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &SendSmsContentError);
            if (dbus_error_is_set(&SendSmsContentError))
            {
                printf("DBus connection error::\n%s\n", SendSmsContentError.message);
                dbus_error_free(&SendSmsContentError);
            }
            // 创建方法调用sms消息
            DBusMessage* SendSmsContentMessage = dbus_message_new_method_call(
                "org.freedesktop.ModemManager1",  // 目标接口
                smsSavePath.c_str(), // 目标路径
                "org.freedesktop.ModemManager1.Sms", // 接口名称
                "Send" // 方法名称
            );
            // 发送方法调用消息并等待回复
            DBusPendingCall* pendingCallSend;
            dbus_connection_send_with_reply(SendSmsContentConnection, SendSmsContentMessage, &pendingCallSend, -1);
            dbus_connection_flush(SendSmsContentConnection);
            dbus_message_unref(SendSmsContentMessage);
            // 等待回复
            dbus_pending_call_block(pendingCallSend);
            // 获取回复消息
            DBusMessage* sendreply = dbus_pending_call_steal_reply(pendingCallSend);
            if (sendreply) {
                if (dbus_message_is_error(sendreply, DBUS_ERROR_UNKNOWN_METHOD)) {
                    cerr << "Method call failed: Unknown method" << endl;
                }
                else {
                    printf("短信已发送\n");
                }
                dbus_message_unref(sendreply);
            }
            else {
                cerr << "Failed to get a sendreply." << endl;
            }
            dbus_pending_call_unref(pendingCallSend);
            // 关闭连接
            dbus_connection_unref(SendSmsContentConnection);
        }
        else
        {
            DBusError DeleteSmsContentError;
            dbus_error_init(&DeleteSmsContentError);
            DBusConnection* DeleteSmsContentConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &DeleteSmsContentError);
            if (dbus_error_is_set(&DeleteSmsContentError))
            {
                printf("DBus connection error::\n%s\n", DeleteSmsContentError.message);
                dbus_error_free(&DeleteSmsContentError);
            }
            // 创建方法调用sms消息
            DBusMessage* DeleteSmsContentMessage = dbus_message_new_method_call(
                "org.freedesktop.ModemManager1",  // 目标接口
                "/org/freedesktop/ModemManager1/Modem/0", // 目标路径
                "org.freedesktop.ModemManager1.Modem.Messaging", // 接口名称
                "Delete" // 方法名称
            );
            const char* deleteSmsPath = smsSavePath.c_str();
            dbus_message_append_args(DeleteSmsContentMessage, DBUS_TYPE_OBJECT_PATH, &deleteSmsPath, DBUS_TYPE_INVALID);

            // 发送方法调用消息并等待回复
            DBusPendingCall* pendingCallDelete;
            dbus_connection_send_with_reply(DeleteSmsContentConnection, DeleteSmsContentMessage, &pendingCallDelete, -1);
            dbus_connection_flush(DeleteSmsContentConnection);
            dbus_message_unref(DeleteSmsContentMessage);
            // 等待回复
            dbus_pending_call_block(pendingCallDelete);
            // 获取回复消息
            DBusMessage* deletereply = dbus_pending_call_steal_reply(pendingCallDelete);
            if (deletereply) {
                if (dbus_message_is_error(deletereply, DBUS_ERROR_UNKNOWN_METHOD)) {
                    cerr << "Method call failed: Unknown method" << endl;
                }
                else {
                    printf("短信缓存已清理，按任意键退出程序\n");
                    string temp;
                    getline(cin, temp);
                }
                dbus_message_unref(deletereply);
            }
            else {
                cerr << "Failed to get a deletereply." << endl;
            }
            dbus_pending_call_unref(pendingCallDelete);
            // 关闭连接
            dbus_connection_unref(DeleteSmsContentConnection);
        }
    }
}

void handle_request(const Request& req, Response& res) {
    // 获取参数值
    string telnum = req.get_param_value("telnum");
    string smstext = req.get_param_value("smstext");
    sendSms(telnum, smstext, "api");
    // 构造响应
    string response_text = "ok";
    res.set_content(response_text, "text/plain");
}
void setupApiPort() {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string apiPort = configMap["apiPort"];
    if (apiPort == "") {
        configMap.erase("apiPort");
        printf("首次运行请输入要使用的api端口：\n");
        getline(cin, apiPort);
        configMap["apiPort"] = apiPort;
        writeConfigFile(configMap);
    }
}
void startSendSmsApi()
{
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string apiPort = configMap["apiPort"];
    int sapiPort = 0;
    sscanf(apiPort.c_str(), "%d", &sapiPort);
    Server svr;
    // 处理 GET 请求，路径为 /api
    svr.Get("/api", [](const Request& req, Response& res) {
        handle_request(req, res);
        });
    cout << "短信发送webapi接口已运行在"+apiPort+"端口" << endl;
    svr.listen("0.0.0.0", sapiPort);
}

//检查配置文件是否存在并初始化
void checkConfig(string configFilePath) {
    //string fileName = "config.txt";
    //ifstream file(configFilePath);
    if (configFilePath=="")
    {
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
            configFile << "smsCodeKey = 验证码±verification±code±인증±代码±随机码" << endl;
            configFile.close();
        }
        else {
            cout << "无法打开配置文件。" << endl;
        }

    }
}

int main(int argc, char* argv[])
{
    string startGuideChoiseNum = "";
    string sendMethodGuideChoiseNum = "";
    for (int i = 1; i < argc; ++i)
    {
        if (string(argv[i]) == "-fE")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "1";
        }
        else if (string(argv[i]) == "-fP")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "2";
        }
        else if (string(argv[i]) == "-fW")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "3";
        }
        else if (string(argv[i]) == "-fT")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "4";
        }
        else if (string(argv[i]) == "-fD")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "5";
        }
        else if (string(argv[i]) == "-fB")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "6";
        }
        else if (string(argv[i]) == "-fS")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "7";
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
            if (apiswitch=="enable") {
                if (startGuideChoiseNum == "1") {
                    startGuideChoiseNum = "3";
                }
                else {
                    startGuideChoiseNum = "4";
                }
                
            }
        }
    }
    string StartGuideResult = onStartGuide(startGuideChoiseNum);
    if (StartGuideResult == "1"|| StartGuideResult == "3" || StartGuideResult == "4") {
        checkConfig(configFilePath);
        if (StartGuideResult == "3") {
            //创建一个线程来运行接口
            setupApiPort();
            string sendMethodGuideResult = sendMethodGuide(sendMethodGuideChoiseNum);
            thread server_thread1(monitorDbus, sendMethodGuideResult);
            thread server_thread(startSendSmsApi);
            server_thread1.join();
            server_thread.join();
        }
        else if (StartGuideResult == "4") {
            setupApiPort();
            startSendSmsApi();
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
        string smsText ="";
        getline(cin, smsText);
        sendSms(telNumber, smsText,"command");
    }
    return 0;
}