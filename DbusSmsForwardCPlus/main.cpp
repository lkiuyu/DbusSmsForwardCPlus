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
std::string trim(std::string strinput)
{
    if (!strinput.empty())
    {
        strinput.erase(0, strinput.find_first_not_of(" "));
        strinput.erase(strinput.find_last_not_of(" ") + 1);
    }
    return strinput;
}
void replaceChar(std::string& str, char targetChar, char replacementChar) {
    for (char& c : str) {
        if (c == targetChar) {
            c = replacementChar;
        }
    }
}
void replaceString(std::string& str, std::string targetStr, std::string replacementStr) {
    size_t index = str.find(targetStr);
    if (index != std::string::npos) {
        str.replace(index, targetStr.length(), replacementStr);
    }
}
std::vector<std::string> SplitCodeKeyString(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter[0])) {
        replaceString(token, "\261", "");
        tokens.push_back(token);
    }
    return tokens;
}
std::vector<std::string> splitCNString(const std::string& input, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t startPos = 0;
    size_t endPos = input.find(delimiter);
    while (endPos != std::string::npos) {
        std::string substr = input.substr(startPos, endPos - startPos);
        result.push_back(substr);
        startPos = endPos + delimiter.length();
        endPos = input.find(delimiter, startPos);
    }
    // Add the remaining part of the input string
    std::string substr = input.substr(startPos);
    result.push_back(substr);
    return result;
}
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
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
std::string UrlEncodeUTF8(const std::string& str) {
    std::stringstream encodedStr;
    encodedStr << std::hex << std::uppercase << std::setfill('0');

    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encodedStr << c;
        }
        else {
            encodedStr << '%' << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(c));
        }
    }

    return encodedStr.str();
}
std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
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
std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
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
std::vector<std::string> extractAllContent(const std::string& input) {
    std::vector<std::string> extractedContents;
    size_t startPos = 0;
    while (true) {
        size_t openingPos = input.find("【", startPos);
        size_t closingPos = input.find("】", startPos);
        if (openingPos != std::string::npos && closingPos != std::string::npos && openingPos < closingPos) {
            openingPos += 3; // Adjust to skip the opening bracket "【" (3 characters)
            std::string content = input.substr(openingPos, closingPos - openingPos);
            extractedContents.push_back(content);
            startPos = closingPos + 3; // Move startPos to the character after the closing bracket "】" (3 characters)
        }
        else {
            break; // Break the loop if no more brackets are found or in the wrong order
        }
    }
    return extractedContents;
}
std::string configFilePath = "";
// 读取配置文件并存储到一个键值对映射中
std::map<std::string, std::string> readConfigFile() {
    std::string filename = "";
    if (configFilePath!="") {
        filename = configFilePath;
    }
    else {
        filename = "config.txt";
    }
    std::map<std::string, std::string> configMap;
    std::ifstream configFile(filename);

    if (configFile.is_open()) {
        std::string line;
        while (std::getline(configFile, line)) {
            // 解析每一行的配置项和值
            std::size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = trim(line.substr(0, delimiterPos));
                std::string value = trim(line.substr(delimiterPos + 1));
                configMap[key] = value;
            }
        }
        configFile.close();
    }
    return configMap;
}
// 将配置项写入配置文件
void writeConfigFile(const std::map<std::string, std::string>& configMap) {
    std::string filename = "";
    if (configFilePath != "") {
        filename = configFilePath;
    }
    else {
        filename = "config.txt";
    }
    std::ofstream configFileClear(filename, std::ofstream::trunc);
    configFileClear.close();
    std::ofstream configFile(filename);
    if (configFile.is_open()) 
    {
        for (const auto& pair : configMap) {
            configFile << pair.first << " = " << pair.second << std::endl;
        }
        configFile.close();
    }
    else {
        std::cout << "无法打开配置文件。" << std::endl;
    }
}
//判断短信内是否含有验证码
bool JudgeSmsContentHasCode(std::string& smscontent) {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string smsKeysStr = configMap["smsCodeKey"];
    if (smsKeysStr=="") {
        smsKeysStr = "验证码±verification±code±인증±代码±随机码";
        configMap.erase("smsCodeKey");
        configMap["smsCodeKey"] = smsKeysStr;
        writeConfigFile(configMap);
    }
    std::string delimiter = "±";
    std::vector<std::string> splitStrings = SplitCodeKeyString(smsKeysStr, delimiter);
    for (const auto& t : splitStrings) {
        size_t index = smscontent.find(t);
        if (index != std::string::npos) {
            replaceString(smscontent, t, " " + t + " ");
            return true;
        }
        //std::cout << t << std::endl;
    }
    return false;
}
int CountDigits(const std::string& str) {
    int digitCount = 0;
    for (char c : str) {
        if (std::isdigit(c)) {
            digitCount++;
        }
    }
    return digitCount;
}
//获取验证码
std::string GetCode(const std::string& smsContent) {
    std::string pattern = R"(\b[A-Za-z0-9]{4,7}\b)";
    std::regex regexPattern(pattern);

    std::sregex_iterator iter(smsContent.begin(), smsContent.end(), regexPattern);
    std::sregex_iterator end;
    std::vector<std::string> matchs;
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
        std::string maxDigitsString = "";
        for (const auto& match : matchs) {
            std::string currentString = match;
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
std::string GetCodeSmsFrom(const std::string& smsContent) {

    std::vector<std::string> extractedContents = extractAllContent(smsContent);
    for (const auto& content : extractedContents) {
        std::string delimiter = content;
        std::vector<std::string> splitStrings = splitCNString(smsContent, delimiter);
        if (splitStrings[0]=="【") {
            return "【"+ content +"】";
        }
        else if (splitStrings[splitStrings.size()-1] == "】") {
            return "【" + content + "】";
        }
        //std::cout << "Extracted content: " << content << std::endl;
    }
    return "";
}
//获取组合的验证码来源和验证码
std::string GetSmsCodeStr(std::string smscontent, std::string& smscode, std::string& CodeFrom) {
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
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string smtpHost = configMap["smtpHost"];
    std::string smtpPort = configMap["smtpPort"];
    std::string emailKey = configMap["emailKey"];
    std::string sendEmial = configMap["sendEmial"];
    std::string reciveEmial = configMap["reciveEmial"];
    if (smtpHost == ""&& smtpPort == "" && emailKey == "" && sendEmial == "" && reciveEmial == "") {
        configMap.erase("smtpHost");
        configMap.erase("smtpPort");
        configMap.erase("emailKey");
        configMap.erase("sendEmial");
        configMap.erase("reciveEmial");
        printf("首次运行请输入邮箱转发相关配置信息\n请输入smtp地址：\n");
        std::getline(std::cin, smtpHost);
        configMap["smtpHost"] = smtpHost;

        printf("请输入smtp端口：\n");
        std::getline(std::cin, smtpPort);
        configMap["smtpPort"] = smtpPort;

        printf("请输入邮箱密钥：\n");
        std::getline(std::cin, emailKey);
        configMap["emailKey"] = emailKey;

        printf("请输入发件邮箱：\n");
        std::getline(std::cin, sendEmial);
        configMap["sendEmial"] = sendEmial;

        printf("请输入收件邮箱：\n");
        std::getline(std::cin, reciveEmial);
        configMap["reciveEmial"] = reciveEmial;

        writeConfigFile(configMap);
    }
}
//使用Email转发消息
void sendByEmail(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string smtpHost = configMap["smtpHost"];
    int smtpPort = 0;
    sscanf(configMap["smtpPort"].c_str(), "%d", &smtpPort);
    std::string emailKey = configMap["emailKey"];
    std::string sendEmial = configMap["sendEmial"];
    std::string reciveEmial = configMap["reciveEmial"];
    std::string emailcontent = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    std::string SmsCode;
    std::string SmsCodeFrom;
    std::string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    std::string subject = "短信转发" + smsnumber;
    if (SmsCodeStr != "") {
        subject = SmsCodeStr + " " + subject;
    }
    std::string smtpserver = "smtps://" + smtpHost ;
    //printf(smtpserver.c_str());
    std::string from = sendEmial;
    std::string passs = emailKey;//这里替换成自己的授权码
    std::string to = reciveEmial;
    std::string strMessage = emailcontent;
    std::vector<std::string> vecTo; //发送列表
    vecTo.push_back(reciveEmial);
    std::vector<std::string> ccList;
    // ccList.push_back("xxx@xxx.com.cn");//抄送列表
    std::vector<std::string> attachment;

    SmtpBase* base;
    //SimpleSmtpEmail m_mail(smtpHost, configMap["smtpPort"]);
   // base = &m_mail;
   // base->SendEmail(from, passs, to, subject, strMessage);//普通的文本发送，明文发送

    SimpleSslSmtpEmail m_ssl_mail(smtpHost, "465");
    base = &m_ssl_mail;
    base->SendEmail(from, passs, to, subject, strMessage);
    //base->SendEmail(from, passs, vecTo, subject, strMessage, attachment, ccList);//加密的发送，支持抄送、附件等


    /*std::string smtp_server { smtpHost };
    int port{ smtpPort }; 
    std::string
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
        std::cerr << "error: " << e.displayText() << std::endl;
    }
    std::cout << "email sent successfully!" << std::endl;*/

}

//设置pushplus相关配置
void SetupPushPlusInfo() {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string pushPlusToken = configMap["pushPlusToken"];
    if (pushPlusToken=="") {
        configMap.erase("pushPlusToken");
        printf("首次运行请输入PushPlusToken\n");
        std::getline(std::cin, pushPlusToken);
        configMap["pushPlusToken"] = pushPlusToken;
        writeConfigFile(configMap);
    }
}
//使用pushplus转发消息
void sendByPushPlus(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string pushPlusToken = configMap["pushPlusToken"];
    // PushPlus的API端点URL
    std::string apiUrl = "https://www.pushplus.plus/send";
    std::string SmsCode;
    std::string SmsCodeFrom;
    std::string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);

    // 推送消息的标题和内容
    std::string title = "短信转发" + smsnumber;
    if (SmsCodeStr!="") {
        title = SmsCodeStr + " " + title;
    }
    
    std::string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;

    // 构建POST请求的数据
    std::string postData = "token=" + pushPlusToken + "&title=" + title + "&content=" + smstext;
    // 初始化cURL库
    CURL* curl = curl_easy_init();
    if (curl)
    {
        // 设置POST请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        // 设置响应数据的回调函数
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                std::cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << std::endl;
            }
            int code = doc["code"].GetInt();
            std::string errmsg = doc["msg"].GetString(); 
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
            std::cerr << "Failed to send PushPlus message: " << curl_easy_strerror(res) << std::endl;
        }
        // 释放cURL资源
        curl_easy_cleanup(curl);
    }
}

//设置企业微信相关配置
void SetupWeComInfo() {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string corpid = configMap["WeChatQYID"];
    std::string appsecret = configMap["WeChatQYApplicationSecret"];
    std::string appid = configMap["WeChatQYApplicationID"];

    if (corpid == ""&& appsecret == "" && appid == "") {
        configMap.erase("WeChatQYID");
        configMap.erase("WeChatQYApplicationSecret");
        configMap.erase("WeChatQYApplicationID");
        printf("首次运行请输入企业ID\n");
        std::getline(std::cin, corpid);
        configMap["WeChatQYID"] = corpid;
        printf("请输入自建应用ID\n");
        std::getline(std::cin, appid);
        configMap["WeChatQYApplicationID"] = appid;
        printf("请输入自建应用密钥\n");
        std::getline(std::cin, appsecret);
        configMap["WeChatQYApplicationSecret"] = appsecret;
        writeConfigFile(configMap);
    }
}
//使用企业微信转发消息
void sendByWeCom(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();

    std::string corpid = configMap["WeChatQYID"];
    std::string corpsecret = configMap["WeChatQYApplicationSecret"];
    int agentid = 0;
    sscanf(configMap["WeChatQYApplicationID"].c_str(), "%d", &agentid);
    std::string url = "https://qyapi.weixin.qq.com/cgi-bin/gettoken?corpid=" + corpid + "&corpsecret=" + corpsecret;
    std::string smscontent = "短信转发\n发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    std::string SmsCode;
    std::string SmsCodeFrom;
    std::string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    if (SmsCodeStr != "") {
        smscontent = SmsCodeStr + "\n" + smscontent;
    }
    CURL* curl = curl_easy_init();
    if (curl)
    {
        // 设置POST请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // 设置响应数据的回调函数
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                std::cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << std::endl;
            }
            int errcode = doc["errcode"].GetInt();
            std::string errmsg = doc["errmsg"].GetString();
            if (errcode == 0 && errmsg=="ok")
            {
                std::string access_token = doc["access_token"].GetString();
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
                std::string smsbody =smscontent;
                rapidjson::Value v(rapidjson::kStringType);
                v.SetString(smsbody.c_str(), smsbody.length(), allocator1);
                jobj1.AddMember("content", v, allocator1);
                jobj.AddMember("text", jobj1, allocator);
                jobj.AddMember("safe", 0, allocator);
                jobj.AddMember("enable_id_trans", 0, allocator);
                jobj.AddMember("enable_duplicate_check", 0, allocator);
                jobj.AddMember("duplicate_check_interval", 1800, allocator);
               
                std::string msgurl = "https://qyapi.weixin.qq.com/cgi-bin/message/send?access_token=" + access_token;
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                jobj.Accept(writer);
                std::string jsonData = buffer.GetString();

                CURL* curl2 = curl_easy_init();
                if (curl2) {
                    // 设置POST请求的URL和数据
                    curl_easy_setopt(curl2, CURLOPT_URL, msgurl.c_str());
                    curl_easy_setopt(curl2, CURLOPT_POSTFIELDS, jsonData.c_str());
                    curl_easy_setopt(curl2, CURLOPT_POSTFIELDSIZE, jsonData.length());
                    // 设置响应数据的回调函数
                    std::string response2;
                    curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl2, CURLOPT_WRITEDATA, &response2);
                    // 执行HTTP请求
                    CURLcode res2 = curl_easy_perform(curl2);
                    if (res2 == CURLE_OK) {
                        rapidjson::Document doc2;
                        doc2.Parse(response2.c_str());
                        if (doc2.HasParseError()) {
                            std::cout << "Failed to parse JSON. Error code: " << doc2.GetParseError() << ", "
                                << rapidjson::GetParseError_En(doc2.GetParseError()) << std::endl;
                        }
                        int errcode1 = doc2["errcode"].GetInt();
                        std::string errmsg1 = doc2["errmsg"].GetString();
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
                        std::cerr << "Failed to send WeCom message: " << curl_easy_strerror(res2) << std::endl;
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
            std::cerr << "Failed to send WeCom message: " << curl_easy_strerror(res) << std::endl;
        }
        // 释放cURL资源
        curl_easy_cleanup(curl);
    }
}

//设置TelegramBot相关配置
void SetupTGBotInfo() {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string TGBotToken = configMap["TGBotToken"];
    std::string TGBotChatID = configMap["TGBotChatID"];
    std::string IsEnableCustomTGBotApi = configMap["IsEnableCustomTGBotApi"];
    std::string CustomTGBotApi = configMap["CustomTGBotApi"];

    if (TGBotToken == ""&& TGBotChatID == "" && IsEnableCustomTGBotApi == "") {
        configMap.erase("TGBotToken");
        configMap.erase("TGBotChatID");
        configMap.erase("IsEnableCustomTGBotApi");
        configMap.erase("CustomTGBotApi");

        printf("首次运行请输入TG机器人Token\n");
        std::getline(std::cin, TGBotToken);
        configMap["TGBotToken"] = TGBotToken;
        printf("请输入机器人要转发到的ChatId\n");
        std::getline(std::cin, TGBotChatID);
        configMap["TGBotChatID"] = TGBotChatID;
        std::string customApiEnableInput = "";
        do
        {
            printf("是否需要使用自定义api(1.使用 2.不使用)\n");
            std::getline(std::cin, customApiEnableInput);
        } while (!(customApiEnableInput == "1" || customApiEnableInput == "2"));
        if (customApiEnableInput == "1")
        {
            IsEnableCustomTGBotApi = "true";
            configMap["IsEnableCustomTGBotApi"] = IsEnableCustomTGBotApi;
            printf("请输入机器人自定义api(格式https://xxx.abc.com)\n");
            std::getline(std::cin, CustomTGBotApi);
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
void sendByTGBot(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string TGBotToken = configMap["TGBotToken"];
    std::string TGBotChatID = configMap["TGBotChatID"];
    std::string IsEnableCustomTGBotApi = configMap["IsEnableCustomTGBotApi"];
    std::string CustomTGBotApi = configMap["CustomTGBotApi"];
    std::string url = "";
    if (IsEnableCustomTGBotApi == "true")
    {
        url = CustomTGBotApi;
    }
    else
    {
        url = "https://api.telegram.org";
    }
    url += "/bot" + TGBotToken + "/sendMessage?chat_id=" + TGBotChatID + "&text=";
    std::string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    std::string SmsCode;
    std::string SmsCodeFrom;
    std::string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    if (SmsCodeStr != "") {
        url += UrlEncode(SmsCodeStr + "\n短信转发\n" + content);
    }
    else {
        url += UrlEncode("短信转发\n" + content);
    }
    //std::cout << url << std::endl;

    CURL* curl = curl_easy_init();
    if (curl) {
        // 设置Get请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // 设置响应数据的回调函数
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                std::cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << std::endl;
            }
            bool status = doc["ok"].GetBool();
            if (status)
            {
                printf("TGBot转发成功\n");
            }
            else
            {
                std::string description = doc["description"].GetString();
                printf(description.c_str());
            }
        }
        curl_easy_cleanup(curl);
    }
    
}

//设置DingTalkBot相关配置
void SetupDingtalkBotMsg() {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string DingTalkAccessToken = configMap["DingTalkAccessToken"];
    std::string DingTalkSecret = configMap["DingTalkSecret"];
    if (DingTalkAccessToken == ""&& DingTalkSecret == "") {
        configMap.erase("DingTalkAccessToken");
        configMap.erase("DingTalkSecret");
        printf("首次运行请输入钉钉机器人AccessToken\n");
        std::getline(std::cin, DingTalkAccessToken);
        configMap["DingTalkAccessToken"] = DingTalkAccessToken;
        printf("请输入钉钉机器人加签secret\n");
        std::getline(std::cin, DingTalkSecret);
        configMap["DingTalkSecret"] = DingTalkSecret;
        writeConfigFile(configMap);
    }
}
//使用DingTalkBot转发消息
void sendByDingtalkBot(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string DingTalkAccessToken = configMap["DingTalkAccessToken"];
    std::string DingTalkSecret = configMap["DingTalkSecret"];
    std::string DING_TALK_BOT_URL = "https://oapi.dingtalk.com/robot/send?access_token=";
    std::string url = DING_TALK_BOT_URL + DingTalkAccessToken;
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string timestamp1=std::to_string(timestamp);
    std::string stringToSign = std::to_string(timestamp) + "\n" + DingTalkSecret;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength;
    HMAC(EVP_sha256(), DingTalkSecret.c_str(), DingTalkSecret.length(),
        reinterpret_cast<const unsigned char*>(stringToSign.c_str()), stringToSign.length(),
        digest, &digestLength);
    std::string hmacresult(reinterpret_cast<char*>(digest), digestLength);
    BIO* bmem = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bmem);
    BIO_write(b64, hmacresult.c_str(), hmacresult.length());
    BIO_flush(b64);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bmem, &bptr);
    std::string base64result(bptr->data, bptr->length);
    BIO_free_all(bmem);
    std::string sign = UrlEncodeUTF8(base64result);
    url += "&timestamp=" + timestamp1 + "&sign="+ sign;
    std::string smscontent = "短信转发\n发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    std::string SmsCode;
    std::string SmsCodeFrom;
    std::string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    if (SmsCodeStr != "") {
        smscontent = SmsCodeStr + "\n" + smscontent;
    }
    rapidjson::Document msgContent;
    msgContent.SetObject();
    // 添加键值对
    rapidjson::Document::AllocatorType& allocator = msgContent.GetAllocator();
    std::string smsbody = smscontent;
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
    std::string jsonData = buffer.GetString();
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
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                std::cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << std::endl;
            }
            int errcode1 = doc["errcode"].GetInt();
            std::string errmsg1 = doc["errmsg"].GetString();
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
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();

    std::string BarkUrl = configMap["BarkUrl"];
    std::string BrakKey = configMap["BrakKey"];

    if (BarkUrl == ""&& BrakKey == "") {
        configMap.erase("BarkUrl");
        configMap.erase("BrakKey");
        printf("首次运行请输入Bark服务器地址\n");
        std::getline(std::cin, BarkUrl);
        configMap["BarkUrl"] = BarkUrl;
        printf("请输入Bark推送key\n");
        std::getline(std::cin, BrakKey);
        configMap["BrakKey"] = BrakKey;
        writeConfigFile(configMap);
    }
}
//使用Bark转发消息
void sendByBark(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    std::string BarkUrl = configMap["BarkUrl"];
    std::string BrakKey = configMap["BrakKey"];
    std::string SmsCode;
    std::string SmsCodeFrom;
    std::string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    std::string title = "短信转发" + smsnumber;
    std::string url = BarkUrl + "/" + BrakKey + "/";
    std::string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    url += UrlEncode(content);
    if (SmsCodeStr != "") {
        title = SmsCodeStr + " " + title;
        url += "?group=" + smsnumber + "&title=" + UrlEncode(title) + "&autoCopy=1&copy=" + SmsCode;
    }
    else
    {
        url += "?group=" + smsnumber + "&title=" + title;
    }
    std::cout << url << std::endl;

    CURL* curl = curl_easy_init();
    if (curl) {
        // 设置Get请求的URL和数据
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // 设置响应数据的回调函数
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // 执行HTTP请求
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError()) {
                std::cout << "Failed to parse JSON. Error code: " << doc.GetParseError() << ", "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << std::endl;
            }
            int status = doc["code"].GetInt();
            if (status == 200)
            {
                printf("Bark转发成功\n");
            }
            else
            {
                std::string rmsg = doc["message"].GetString();
                printf(rmsg.c_str());
               
            }
        }
        curl_easy_cleanup(curl);
    }
}


//处理用户选择的转发渠道
std::string sendMethodGuide(std::string chooseOption)
{
    if (chooseOption == "")
    {
        printf("请选择转发渠道：1.邮箱转发，2.pushplus转发，3.企业微信转发，4.TG机器人转发，5.钉钉转发，6.Bark转发\n");
        std::getline(std::cin, chooseOption);
    }
    if (chooseOption == "1" || chooseOption == "2" || chooseOption == "3" || chooseOption == "4" || chooseOption == "5" || chooseOption == "6")
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
        else
        {
            return "";
        }
    }
    else
    {
        printf("请输入1或2或3或4或5或6\n");
        return sendMethodGuide("");
    }
}
void sendSms(std::string sendMethodGuideResult, std::string telnum, std::string smscontent, std::string smsdate)
{
    char target = 'T';
    char replacement = ' ';
    replaceChar(smsdate, target, replacement);
    std::string body = "发信电话:" + telnum + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smscontent + "\n";
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
}
std::string onStartGuide(std::string chooseOption)
{
    if (chooseOption=="")
    {
        printf("请选择运行模式：1为短信转发模式，2为发短信模式\n");
        std::getline(std::cin, chooseOption);
    }
    if (chooseOption == "1" || chooseOption == "2")
    {
        return chooseOption;
    }
    else
    {
        printf("请输入1或2\n");
        return onStartGuide("");
    }

}

void getAndSendSmsContent(std::string sendMethodGuideResult, const char *smsPath) {
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
                std::string keyName(key);
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
    if (std::string(smscontent)!="") {

        sendSms(sendMethodGuideResult, std::string(telnum), std::string(smscontent), std::string(smsdate));
        // 释放消息资源
        dbus_message_unref(reply);
        dbus_message_unref(smsContentMessage);
        // 关闭DBus连接
        dbus_connection_unref(GetSmsContentConnection);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        getAndSendSmsContent(sendMethodGuideResult,smsPath);
    }
}
//处理dbus获取到的消息并转发
void parseDBusMessageAndSend(DBusMessage* message,std::string sendMethodGuideResult)
{
    // 获取接口名称
    const char* interface = dbus_message_get_interface(message);
    std::string interfaceName(interface);

    if (interfaceName == "org.freedesktop.ModemManager1.Modem.Messaging") {
        // 获取信号名称
        const char* signal = dbus_message_get_member(message);
        std::string signalName(signal);
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

//检查配置文件是否存在并初始化
void checkConfig(std::string configFilePath) {
    //std::string fileName = "config.txt";
    //std::ifstream file(configFilePath);
    if (configFilePath=="")
    {
        // 配置文件不存在，创建它
        std::ofstream configFile("config.txt"); // 创建配置文件
        if (configFile.is_open()) {
            // 写入配置项
            configFile << "smtpHost = " << std::endl;
            configFile << "smtpPort = " << std::endl;
            configFile << "emailKey = " << std::endl;
            configFile << "sendEmial = " << std::endl;
            configFile << "reciveEmial = " << std::endl;
            configFile << "pushPlusToken = " << std::endl;
            configFile << "WeChatQYID = " << std::endl;
            configFile << "WeChatQYApplicationSecret = " << std::endl;
            configFile << "WeChatQYApplicationID = " << std::endl;
            configFile << "TGBotToken = " << std::endl;
            configFile << "TGBotChatID = " << std::endl;
            configFile << "IsEnableCustomTGBotApi = " << std::endl;
            configFile << "CustomTGBotApi = " << std::endl;
            configFile << "DingTalkAccessToken = " << std::endl;
            configFile << "DingTalkSecret = " << std::endl;
            configFile << "BarkUrl = " << std::endl;
            configFile << "BrakKey = " << std::endl;
            configFile << "smsCodeKey = 验证码±verification±code±인증±代码±随机码" << std::endl;
            configFile.close();
        }
        else {
            std::cout << "无法打开配置文件。" << std::endl;
        }

    }
}

int main(int argc, char* argv[])
{
    std::string startGuideChoiseNum = "";
    std::string sendMethodGuideChoiseNum = "";
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "-fE")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "1";
        }
        else if (std::string(argv[i]) == "-fP")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "2";
        }
        else if (std::string(argv[i]) == "-fW")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "3";
        }
        else if (std::string(argv[i]) == "-fT")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "4";
        }
        else if (std::string(argv[i]) == "-fD")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "5";
        }
        else if (std::string(argv[i]) == "-fB")
        {
            startGuideChoiseNum = "1";
            sendMethodGuideChoiseNum = "6";
        }
        else if (std::string(argv[i]) == "-sS")
        {
            startGuideChoiseNum = "2";
        }
        else if (std::string(argv[i]).find("--configfile=") != std::string::npos)
        {
            std::string inputpath = std::string(argv[i]);
            replaceString(inputpath, "--configfile=", "");
            std::ifstream file(inputpath);
            if (file) {
                configFilePath = inputpath;
            }
        }
    }
    std::string StartGuideResult = onStartGuide(startGuideChoiseNum);
    if (StartGuideResult == "1") {
        checkConfig(configFilePath);
        std::string sendMethodGuideResult = sendMethodGuide(sendMethodGuideChoiseNum);
        printf("正在运行. 按下 Ctrl-C 停止.\n");
        DBusError error;
        DBusConnection* connection;
        dbus_error_init(&error);
        // 连接到DBus会话总线
        connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
        if (dbus_error_is_set(&error))
        {
            std::cerr << "DBus connection error: " << error.message << std::endl;
            dbus_error_free(&error);
            return 1;
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
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        // 断开DBus连接
        dbus_connection_unref(connection);
    }
    else if (StartGuideResult == "2") {  //以下为调用dbus发送短信
        printf("请输入收信号码：\n");
        std::string telNumber = "";
        std::getline(std::cin, telNumber);
        printf("请输入短信内容\n");
        std::string smsText ="";
        std::getline(std::cin, smsText);

        std::string smsSavePath="";


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
                std::cerr << "Method call failed: Unknown method" << std::endl;
            }
            else {
                // 提取对象路径
                const char* objectPath;
                dbus_message_get_args(reply, &CreateSmsContentError, DBUS_TYPE_OBJECT_PATH, &objectPath, DBUS_TYPE_INVALID);
                if (dbus_error_is_set(&CreateSmsContentError)) {
                    std::cerr << "Failed to extract object path from reply: " << CreateSmsContentError.message << std::endl;
                    dbus_error_free(&CreateSmsContentError);
                }
                else {
                    smsSavePath = objectPath;
                }
            }
            dbus_message_unref(reply);
        }
        else {
            std::cerr << "Failed to get a reply." << std::endl;
        }
        dbus_pending_call_unref(pendingCall);
        // 关闭连接
        dbus_connection_unref(CreateSmsContentConnection);
        if (smsSavePath!="") {
            printf("短信创建成功，是否发送？(1.发送短信,其他按键退出程序)\n");
            std::string sendChoise = "";
            std::getline(std::cin, sendChoise);
            if (sendChoise == "1")
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
                        std::cerr << "Method call failed: Unknown method" << std::endl;
                    }
                    else {
                        printf("短信已发送\n");
                    }
                    dbus_message_unref(sendreply);
                }
                else {
                    std::cerr << "Failed to get a sendreply." << std::endl;
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
                        std::cerr << "Method call failed: Unknown method" << std::endl;
                    }
                    else {
                        printf("短信缓存已清理，按任意键退出程序\n");
                        std::string temp;
                        std::getline(std::cin, temp);
                    }
                    dbus_message_unref(deletereply);
                }
                else {
                    std::cerr << "Failed to get a deletereply." << std::endl;
                }
                dbus_pending_call_unref(pendingCallDelete);
                // 关闭连接
                dbus_connection_unref(DeleteSmsContentConnection);
            }
        }
    }
    return 0;
}