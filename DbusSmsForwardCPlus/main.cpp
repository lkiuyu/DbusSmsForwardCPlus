﻿#include <dbus/dbus.h>
#include <iostream>
#include <thread>
#include <cstdio>
#include <curl/curl.h>
#include <fstream> 
#include <map>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/SecureSMTPClientSession.h>
#include <Poco/Net/NetException.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Query.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/URI.h>
#include <chrono>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <string>
#include <sstream>
#include <iomanip>

std::string trim(std::string strinput)
{
    if (!strinput.empty())
    {
        strinput.erase(0, strinput.find_first_not_of(" "));
        strinput.erase(strinput.find_last_not_of(" ") + 1);
    }
    return strinput;
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

// 读取配置文件并存储到一个键值对映射中
std::map<std::string, std::string> readConfigFile(const std::string& filename) {
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
void writeConfigFile(const std::string& filename, const std::map<std::string, std::string>& configMap) {
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


//设置Email相关配置
void SetupEmailInfo() {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
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

        writeConfigFile(filename, configMap);
    }
}
//使用Email转发消息
void sendByEmail(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
    std::string smtpHost = configMap["smtpHost"];
    int smtpPort = 0;
    sscanf(configMap["smtpPort"].c_str(), "%d", &smtpPort);
    std::string emailKey = configMap["emailKey"];
    std::string sendEmial = configMap["sendEmial"];
    std::string reciveEmial = configMap["reciveEmial"];
    std::string emailcontent = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;

    std::string smtp_server { smtpHost };
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
    std::cout << "email sent successfully!" << std::endl;

}

//设置pushplus相关配置
void SetupPushPlusInfo() {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
    std::string pushPlusToken = configMap["pushPlusToken"];
    if (pushPlusToken=="") {
        configMap.erase("pushPlusToken");
        printf("首次运行请输入PushPlusToken\n");
        std::getline(std::cin, pushPlusToken);
        configMap["pushPlusToken"] = pushPlusToken;
        writeConfigFile(filename, configMap);
    }
}
//使用pushplus转发消息
void sendByPushPlus(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
    std::string pushPlusToken = configMap["pushPlusToken"];
    // PushPlus的API端点URL
    std::string apiUrl = "https://www.pushplus.plus/send";
    // 推送消息的标题和内容
    std::string title = "短信转发"+smsnumber;
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
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse(response);
            Poco::JSON::Object::Ptr responseObject = result.extract<Poco::JSON::Object::Ptr>();
            std::string code = responseObject->getValue<std::string>("code");
            std::string errmsg = responseObject->getValue<std::string>("msg");
            if (code == "200")
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
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
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
        writeConfigFile(filename, configMap);
    }
}
//使用企业微信转发消息
void sendByWeCom(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);

    std::string corpid = configMap["WeChatQYID"];
    std::string corpsecret = configMap["WeChatQYApplicationSecret"];
    int agentid = 0;
    sscanf(configMap["WeChatQYApplicationID"].c_str(), "%d", &agentid);
    std::string url = "https://qyapi.weixin.qq.com/cgi-bin/gettoken?corpid=" + corpid + "&corpsecret=" + corpsecret;
    std::string smscontent = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;


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
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse(response);
            Poco::JSON::Object::Ptr responseObject = result.extract<Poco::JSON::Object::Ptr>();
            std::string errcode = responseObject->getValue<std::string>("errcode");
            std::string errmsg = responseObject->getValue<std::string>("errmsg");
            if (errcode == "0"&& errmsg=="ok")
            {
                std::string access_token = responseObject->getValue<std::string>("access_token");
                Poco::JSON::Object jobj;
                Poco::JSON::Object jobj1;
                jobj.set("touser", "@all");
                jobj.set("toparty", "");
                jobj.set("totag", "");
                jobj.set("msgtype", "text");
                jobj.set("agentid", agentid);
                jobj1.set("content", "短信转发\n" + smscontent);
                jobj.set("text", jobj1);
                jobj.set("safe", 0);
                jobj.set("enable_id_trans",0);
                jobj.set("enable_duplicate_check", 0);
                jobj.set("duplicate_check_interval", 1800);
                std::string msgurl = "https://qyapi.weixin.qq.com/cgi-bin/message/send?access_token=" + access_token;
                std::ostringstream jsonStream;
                Poco::JSON::Stringifier::stringify(jobj, jsonStream);
                std::string jsonData = jsonStream.str();
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
                        Poco::JSON::Parser parser2;
                        Poco::Dynamic::Var result2 = parser2.parse(response2);
                        Poco::JSON::Object::Ptr responseObject2 = result2.extract<Poco::JSON::Object::Ptr>();
                        std::string errcode1 = responseObject2->getValue<std::string>("errcode");
                        std::string errmsg1 = responseObject2->getValue<std::string>("errmsg");
                        if (errcode1 == "0" && errmsg1 == "ok")
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
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
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
        writeConfigFile(filename, configMap);
    }
}
//使用TelegramBot转发消息
void sendByTGBot(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
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
    url += UrlEncode("短信转发\n" + content);
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
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse(response);
            Poco::JSON::Object::Ptr responseObject = result.extract<Poco::JSON::Object::Ptr>();
            std::string status = responseObject->getValue<std::string>("ok");
            if (status == "true"|| status == "True")
            {
                printf("TGBot转发成功\n");
            }
            else
            {
                printf(responseObject->getValue<std::string>("error_code").c_str());
                printf(responseObject->getValue<std::string>("description").c_str());
            }
        }
        curl_easy_cleanup(curl);
    }
    
}

//设置DingTalkBot相关配置
void SetupDingtalkBotMsg() {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
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
        writeConfigFile(filename, configMap);
    }
}
//使用DingTalkBot转发消息
void sendByDingtalkBot(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
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
    std::string smscontent = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    Poco::JSON::Object msgContent;
    msgContent.set("content", "短信转发\n" + smscontent);
    Poco::JSON::Object msgObj;
    msgObj.set("msgtype", "text");
    msgObj.set("text", msgContent);
    std::ostringstream jsonStream;
    Poco::JSON::Stringifier::stringify(msgObj, jsonStream);
    std::string jsonData = jsonStream.str();
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
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse(response);
            Poco::JSON::Object::Ptr responseObject = result.extract<Poco::JSON::Object::Ptr>();
            std::string errcode1 = responseObject->getValue<std::string>("errcode");
            std::string errmsg1 = responseObject->getValue<std::string>("errmsg");
            if (errcode1 == "0" && errmsg1 == "ok")
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
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);

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
        writeConfigFile(filename, configMap);
    }
}
//使用Bark转发消息
void sendByBark(std::string smsnumber, std::string smstext, std::string smsdate) {
    std::string filename = "config.txt";
    std::map<std::string, std::string> configMap;
    // 读取配置文件
    configMap = readConfigFile(filename);
    std::string BarkUrl = configMap["BarkUrl"];
    std::string BrakKey = configMap["BrakKey"];
    std::string url = BarkUrl + "/" + BrakKey + "/";
    std::string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "短信内容:" + smstext;
    url += UrlEncode(content);
    url += "?group=" + smsnumber + "&title=" + "短信转发" + smsnumber;
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
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse(response);
            Poco::JSON::Object::Ptr responseObject = result.extract<Poco::JSON::Object::Ptr>();
            std::string status = responseObject->getValue<std::string>("code");
            if (status == "200")
            {
                printf("Bark转发成功\n");
            }
            else
            {
                printf(responseObject->getValue<std::string>("code").c_str());
                printf(responseObject->getValue<std::string>("message").c_str());
               
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
    std::replace(smsdate.begin(), smsdate.end(), target, replacement);
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
void checkConfig() {
    std::string fileName = "config.txt";
    std::ifstream file(fileName);
    if (!file)
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
        break;
    }
    std::string StartGuideResult = onStartGuide(startGuideChoiseNum);
    if (StartGuideResult == "1") {
        checkConfig();
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
    else if (StartGuideResult == "2") {
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