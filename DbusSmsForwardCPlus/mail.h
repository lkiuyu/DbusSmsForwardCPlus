#pragma once
#include <string>
#include <vector>
#include <map>

#include <openssl/ossl_typ.h>

#ifndef WIN32
#include<netdb.h>
#endif

class SmtpBase
{
protected:
    struct EmailInfo
    {
        std::string smtpServer;      //the SMTP server
        std::string serverPort;      //the SMTP server port
        std::string charset;         //the email character set
        std::string sender;          //the sender's name
        std::string senderEmail;     //the sender's email
        std::string password;        //the password of sender
        std::string recipient;       //the recipient's name
        std::string recipientEmail;  //the recipient's email

        std::map<std::string, std::string> recvList; //收件人列表<email, name>

        std::string subject;         //the email message's subject  邮件主题
        std::string message;         //the email message body   邮件内容

        std::map<std::string, std::string> ccEmail;         //抄送列表
        std::vector<std::string> attachment; //附件
    };
public:

    virtual ~SmtpBase() {}
    /**
     * @brief 简单发送文本邮件
     * @param   from 发送者的帐号
     * @param   passs 发送者密码
     * @param   to 收件人
     * @param   subject 主题
     * @param   strMessage  邮件内容
     */

    virtual int SendEmail(const std::string& from, const std::string& passs, const std::string& to, const std::string& subject, const std::string& strMessage) = 0;
    /**
     * @brief 发送邮件，包括附件以及抄送人和多个收件人
     * @param   from 发送者的帐号
     * @param   passs 发送者密码
     * @param   vecTo 收件人列表
     * @param   subject 主题
     * @param   strMessage  邮件内容
     * @param   attachment  附件列表    附件可以是绝对路径，默认是可执行程序目录下
     * @param   ccList  抄送列表
     */
    virtual int SendEmail(const std::string& from, const std::string& passs, const std::vector<std::string>& vecTo,
        const std::string& subject, const std::string& strMessage, const std::vector<std::string>& attachment, const std::vector<std::string>& ccList) = 0;

    std::string GetLastError()
    {
        return m_lastErrorMsg;
    }

    virtual int Read(void* buf, int num) = 0;
    virtual int Write(const void* buf, int num) = 0;
    virtual int Connect() = 0;
    virtual int DisConnect() = 0;

protected:

    std::string m_lastErrorMsg;


};


class SmtpEmail : public SmtpBase
{

public:
    SmtpEmail(const std::string& emailHost, const std::string& port);
    ~SmtpEmail();

    int SendEmail(const std::string& from, const std::string& passs, const std::string& to, const std::string& subject, const std::string& strMessage);

    int SendEmail(const std::string& from, const std::string& passs, const std::vector<std::string>& vecTo,
        const std::string& subject, const std::string& strMessage, const std::vector<std::string>& attachment, const std::vector<std::string>& ccList);
protected:
    int Read(void* buf, int num);
    int Write(const void* buf, int num);
    int Connect();
    int DisConnect();

    virtual std::string GetEmailBody(const EmailInfo& info);
private:
    //int SMTPSSLComunicate(SSL *connection, const EmailInfo &info);
    int SMTPComunicate(const EmailInfo& info);




protected:
    addrinfo* m_addrinfo;
    int m_socketfd;

    std::string m_host;
    std::string m_port;

    bool m_isConnected;
};

class SimpleSmtpEmail : public SmtpEmail
{
public:
    using SmtpEmail::SmtpEmail;
    virtual std::string GetEmailBody(const EmailInfo& info);
};

class SslSmtpEmail : public SmtpEmail
{
public:
    using SmtpEmail::SmtpEmail;
    ~SslSmtpEmail();

    int Connect();
    int DisConnect();
protected:
    int Read(void* buf, int num);
    int Write(const void* buf, int num);
private:
    SSL_CTX* m_ctx;
    SSL* m_ssl;
};

class SimpleSslSmtpEmail : public SslSmtpEmail
{
public:
    using SslSmtpEmail::SslSmtpEmail;
    virtual std::string GetEmailBody(const EmailInfo& info);
};
#pragma once
