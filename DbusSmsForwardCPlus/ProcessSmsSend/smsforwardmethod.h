#ifndef SMSFORWARDMETHOD_H
#define SMSFORWARDMETHOD_H

#include <regex>
#include <curl/curl.h>
#include <iostream>
#include <chrono>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include "../rapidjson/document.h"
#include "../rapidjson/error/en.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"
#include "../ConfigFileProcess/configfileprocess.h"
#include "../SmsCodeProcess/smscodeprocess.h"
#include "../mail.h"

using namespace std;

void sendByEmail(string smsnumber, string smstext, string smsdate, string devicename);
void sendByPushPlus(string smsnumber, string smstext, string smsdate, string devicename);
void sendByWeCom(string smsnumber, string smstext, string smsdate, string devicename);
void sendByTGBot(string smsnumber, string smstext, string smsdate, string devicename);
void sendByDingtalkBot(string smsnumber, string smstext, string smsdate, string devicename);
void sendByBark(string smsnumber, string smstext, string smsdate, string devicename);
void sendByShell(string smsnumber, string smstext, string smsdate, string devicename);

//使用Email转发消息
void sendByEmail(string smsnumber, string smstext, string smsdate, string devicename) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string smtpHost = configMap["smtpHost"];
    int smtpPort = 0;
    sscanf(configMap["smtpPort"].c_str(), "%d", &smtpPort);
    string emailKey = configMap["emailKey"];
    string sendEmial = configMap["sendEmial"];
    string reciveEmial = configMap["reciveEmial"];
    string emailcontent = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "转发设备:" + devicename + "\n" + "短信内容:" + smstext;
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);
    string subject = "短信转发" + smsnumber;
    if (SmsCodeStr != "") {
        subject = SmsCodeStr + " " + subject;
    }
    string smtpserver = "smtps://" + smtpHost;
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
    printf("Email转发成功\n");
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
//使用pushplus转发消息
void sendByPushPlus(string smsnumber, string smstext, string smsdate, string devicename) {
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
    if (SmsCodeStr != "") {
        title = SmsCodeStr + " " + title;
    }

    string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "转发设备:" + devicename + "\n" + "短信内容:" + smstext;

    // 构建POST请求的数据
    string postData = "token=" + pushPlusToken + "&title=" + title + "&content=" + content;
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
//使用企业微信转发消息
void sendByWeCom(string smsnumber, string smstext, string smsdate, string devicename) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();

    string corpid = configMap["WeChatQYID"];
    string corpsecret = configMap["WeChatQYApplicationSecret"];
    int agentid = 0;
    sscanf(configMap["WeChatQYApplicationID"].c_str(), "%d", &agentid);
    string url = "https://qyapi.weixin.qq.com/cgi-bin/gettoken?corpid=" + corpid + "&corpsecret=" + corpsecret;
    string smscontent = "短信转发\n发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "转发设备:" + devicename + "\n" + "短信内容:" + smstext;
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
            if (errcode == 0 && errmsg == "ok")
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
                string smsbody = smscontent;
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
//使用TelegramBot转发消息
void sendByTGBot(string smsnumber, string smstext, string smsdate, string devicename) {
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
    string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "转发设备:" + devicename + "\n" + "短信内容:" + smstext;
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
//使用DingTalkBot转发消息
void sendByDingtalkBot(string smsnumber, string smstext, string smsdate, string devicename) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string DingTalkAccessToken = configMap["DingTalkAccessToken"];
    string DingTalkSecret = configMap["DingTalkSecret"];
    string DING_TALK_BOT_URL = "https://oapi.dingtalk.com/robot/send?access_token=";
    string url = DING_TALK_BOT_URL + DingTalkAccessToken;
    auto now = chrono::system_clock::now();
    auto timestamp = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
    string timestamp1 = to_string(timestamp);
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
    url += "&timestamp=" + timestamp1 + "&sign=" + sign;
    string smscontent = "短信转发\n发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "转发设备:" + devicename + "\n" + "短信内容:" + smstext;
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
//使用Bark转发消息
void sendByBark(string smsnumber, string smstext, string smsdate, string devicename) {
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
    string content = "发信电话:" + smsnumber + "\n" + "时间:" + smsdate + "\n" + "转发设备:" + devicename + "\n" + "短信内容:" + smstext;
    url += UrlEncode(content);
    if (SmsCodeStr != "") {
        title = SmsCodeStr + " " + title;
        url += "?group=" + smsnumber + "&title=" + UrlEncode(title) + "&autoCopy=1&copy=" + SmsCode;
    }
    else
    {
        url += "?group=" + smsnumber + "&title=" + title;
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
//使用shell转发消息
void sendByShell(string smsnumber, string smstext, string smsdate, string devicename) {
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string ShellPath = configMap["ShellPath"];
    string SmsCode;
    string SmsCodeFrom;
    string SmsCodeStr = GetSmsCodeStr(smstext, SmsCode, SmsCodeFrom);

    ShellPath += " \"" + smsnumber + "\" \"" + smsdate + "\" \"" + smstext + "\" \"" + SmsCode + "\" \"" + SmsCodeFrom + "\" \"" + devicename + "\"";

    int result = system(ShellPath.c_str());
    if (result == 0) {
        cout << "\nShell调用成功" << endl;
    }
    else {
        cout << "\nShell调用失败" << endl;
    }
}

#endif // SMSFORWARDMETHOD_H