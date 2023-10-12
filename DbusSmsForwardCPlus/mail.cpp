#ifdef WIN32
#include <WinSock2.h>
#endif
#include "mail.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#define INVALID_SOCKET -1
#endif

template<typename T>
std::string join(T& vecData, const std::string& delim)
{
    if (vecData.size() <= 0)
    {
        return std::string();
    }
    std::stringstream ss;
    for (auto& item : vecData)
    {
        ss << delim << item;
    }

    return ss.str().substr(delim.length());
}

const char MimeTypes[][2][128] =
{
    { "***",    "application/octet-stream" },
    { "csv",    "text/csv" },
    { "tsv",    "text/tab-separated-values" },
    { "tab",    "text/tab-separated-values" },
    { "html",    "text/html" },
    { "htm",    "text/html" },
    { "doc",    "application/msword" },
    { "docx",    "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
    { "ods",    "application/x-vnd.oasis.opendocument.spreadsheet" },
    { "odt",    "application/vnd.oasis.opendocument.text" },
    { "rtf",    "application/rtf" },
    { "sxw",    "application/vnd.sun.xml.writer" },
    { "txt",    "text/plain" },
    { "xls",    "application/vnd.ms-excel" },
    { "xlsx",    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
    { "pdf",    "application/pdf" },
    { "ppt",    "application/vnd.ms-powerpoint" },
    { "pps",    "application/vnd.ms-powerpoint" },
    { "pptx",    "application/vnd.openxmlformats-officedocument.presentationml.presentation" },
    { "wmf",    "image/x-wmf" },
    { "atom",    "application/atom+xml" },
    { "xml",    "application/xml" },
    { "json",    "application/json" },
    { "js",    "application/javascript" },
    { "ogg",    "application/ogg" },
    { "ps",    "application/postscript" },
    { "woff",    "application/x-woff" },
    { "xhtml","application/xhtml+xml" },
    { "xht",    "application/xhtml+xml" },
    { "zip",    "application/zip" },
    { "gz",    "application/x-gzip" },
    { "rar",    "application/rar" },
    { "rm",    "application/vnd.rn-realmedia" },
    { "rmvb",    "application/vnd.rn-realmedia-vbr" },
    { "swf",    "application/x-shockwave-flash" },
    { "au",        "audio/basic" },
    { "snd",    "audio/basic" },
    { "mid",    "audio/mid" },
    { "rmi",        "audio/mid" },
    { "mp3",    "audio/mpeg" },
    { "aif",    "audio/x-aiff" },
    { "aifc",    "audio/x-aiff" },
    { "aiff",    "audio/x-aiff" },
    { "m3u",    "audio/x-mpegurl" },
    { "ra",    "audio/vnd.rn-realaudio" },
    { "ram",    "audio/vnd.rn-realaudio" },
    { "wav",    "audio/x-wave" },
    { "wma",    "audio/x-ms-wma" },
    { "m4a",    "audio/x-m4a" },
    { "bmp",    "image/bmp" },
    { "gif",    "image/gif" },
    { "jpe",    "image/jpeg" },
    { "jpeg",    "image/jpeg" },
    { "jpg",    "image/jpeg" },
    { "jfif",    "image/jpeg" },
    { "png",    "image/png" },
    { "svg",    "image/svg+xml" },
    { "tif",    "image/tiff" },
    { "tiff",    "image/tiff" },
    { "ico",    "image/vnd.microsoft.icon" },
    { "css",    "text/css" },
    { "bas",    "text/plain" },
    { "c",        "text/plain" },
    { "h",        "text/plain" },
    { "rtx",    "text/richtext" },
    { "mp2",    "video/mpeg" },
    { "mpa",    "video/mpeg" },
    { "mpe",    "video/mpeg" },
    { "mpeg",    "video/mpeg" },
    { "mpg",    "video/mpeg" },
    { "mpv2",    "video/mpeg" },
    { "mov",    "video/quicktime" },
    { "qt",    "video/quicktime" },
    { "lsf",    "video/x-la-asf" },
    { "lsx",    "video/x-la-asf" },
    { "asf",    "video/x-ms-asf" },
    { "asr",    "video/x-ms-asf" },
    { "asx",    "video/x-ms-asf" },
    { "avi",    "video/x-msvideo" },
    { "3gp",    "video/3gpp" },
    { "3gpp",    "video/3gpp" },
    { "3g2",    "video/3gpp2" },
    { "movie","video/x-sgi-movie" },
    { "mp4",    "video/mp4" },
    { "wmv",    "video/x-ms-wmv" },
    { "webm","video/webm" },
    { "m4v",    "video/x-m4v" },
    { "flv",    "video/x-flv" }
};


std::string fileBasename(const std::string path)
{
    std::string filename = path.substr(path.find_last_of("/\\") + 1);//解析出文件名字
    return filename;
}

std::string getFileContents(const char* filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return(contents);
    }
    else {
        printf("文件读取失败:%s\n", filename);
        return "";
    }
}
//获取文件的后缀名 如xxx.jpg 获取的是jpg
std::string GetFileExtension(const std::string& FileName)
{
    if (FileName.find_last_of(".") != std::string::npos)        //find_last_of逆向查找在原字符串中最后一个与指定字符串（或字符）中的某个字符匹配的字符，返回它的位置。若查找失败，则返回npos。
        return FileName.substr(FileName.find_last_of(".") + 1);
    return "";
}

const char* GetMimeTypeFromFileName(char* szFileExt)
{
    for (unsigned int i = 0; i < sizeof(MimeTypes) / sizeof(MimeTypes[0]); i++)
    {
        if (strcmp(MimeTypes[i][0], szFileExt) == 0)
        {
            return MimeTypes[i][1];
        }
    }
    return MimeTypes[0][1];   //if does not match any,  "application/octet-stream" is returned
}

char* base64Encode(char const* origSigned, unsigned origLength)
{
    static const char base64Char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
    if (orig == NULL) return NULL;

    unsigned const numOrig24BitValues = origLength / 3;
    bool havePadding = origLength > numOrig24BitValues * 3;
    bool havePadding2 = origLength == numOrig24BitValues * 3 + 2;
    unsigned const numResultBytes = 4 * (numOrig24BitValues + havePadding);
    char* result = new char[numResultBytes + 3]; // allow for trailing '/0'

    // Map each full group of 3 input bytes into 4 output base-64 characters:
    unsigned i;
    for (i = 0; i < numOrig24BitValues; ++i)
    {
        result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
        result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
        result[4 * i + 2] = base64Char[((orig[3 * i + 1] << 2) | (orig[3 * i + 2] >> 6)) & 0x3F];
        result[4 * i + 3] = base64Char[orig[3 * i + 2] & 0x3F];
    }

    // Now, take padding into account.  (Note: i == numOrig24BitValues)
    if (havePadding)
    {
        result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
        if (havePadding2)
        {
            result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
            result[4 * i + 2] = base64Char[(orig[3 * i + 1] << 2) & 0x3F];
        }
        else
        {
            result[4 * i + 1] = base64Char[((orig[3 * i] & 0x3) << 4) & 0x3F];
            result[4 * i + 2] = '=';
        }
        result[4 * i + 3] = '=';
    }

    result[numResultBytes] = '\0';
    return result;
}

int SmtpEmail::SMTPComunicate(const EmailInfo& info)
{

    if (Connect() != 0)
    {
        return -1;
    }
    char* buffer = new char[1000];
    memset(buffer, 0, 1000);

    Read(buffer, 999);
    if (strncmp(buffer, "220", 3) != 0) // not equal to 220
    {
        m_lastErrorMsg = buffer;
        return 220;
    }

    //向服务器发送ehlo
    std::string command = "ehlo EmailService\r\n";
    Write(command.c_str(), command.length());

    memset(buffer, 0, 1000);
    Read(buffer, 999);
    if (strncmp(buffer, "250", 3) != 0) // ehlo failed
    {
        m_lastErrorMsg = buffer;
        return 250;
    }

    //进行登录验证
    command = "AUTH PLAIN ";
    std::string auth = '\0' + info.senderEmail + '\0' + info.password;
    command += base64Encode(auth.data(), auth.size());
    command += "\r\n";
    Write(command.c_str(), command.length());

    memset(buffer, 0, 1000);
    Read(buffer, 999);
    if (strncmp(buffer, "235", 3) != 0) // login failed
    {
        m_lastErrorMsg = buffer;
        return 250;
    }

    //设置邮件发送者的邮箱地址
    command = "mail FROM:<" + info.senderEmail + ">\r\n";
    Write(command.c_str(), command.length());

    memset(buffer, 0, 1000);
    Read(buffer, 999);
    if (strncmp(buffer, "250", 3) != 0) // not ok
    {
        m_lastErrorMsg = buffer;
        return 250;
    }

    //设置邮件接收者的邮箱地址
    command = "RCPT TO:<" + info.recipientEmail + ">\r\n";
    Write(command.c_str(), command.length());

    memset(buffer, 0, 1000);
    Read(buffer, 999);
    if (strncmp(buffer, "250", 3) != 0) // not ok
    {
        m_lastErrorMsg = buffer;
        return 250;
    }



    //准备发送邮件
    command = "data\r\n";
    Write(command.c_str(), command.length());

    memset(buffer, 0, 1000);
    Read(buffer, 999);
    if (strncmp(buffer, "354", 3) != 0) // not ready to receive message
    {
        m_lastErrorMsg = buffer;
        return 354;
    }

    command = std::move(GetEmailBody(info));
    Write(command.c_str(), command.length());

    memset(buffer, 0, 1000);
    Read(buffer, 999);
    if (strncmp(buffer, "250", 3) != 0) // not ok
    {
        m_lastErrorMsg = buffer;
        return 250;
    }

    //结束发送过程
    delete buffer;
    Write("quit\r\n", 6);

    DisConnect();
    return 0;
}

std::string SmtpEmail::GetEmailBody(const EmailInfo& info)
{
    //设定邮件的发送者名称、接收者名称、邮件主题，邮件内容。
    std::ostringstream message;
    message << "From: =?" << info.charset << "?b?" << base64Encode(info.sender.c_str(), info.sender.length()) << "?= <" << info.senderEmail << ">\r\n";

    std::vector<std::string> vecToList;
    for (auto item : info.recvList)
    {
        std::string to = "=?" + info.charset + "?b?" + base64Encode(item.second.c_str(), item.second.length()) + "?= <" + item.first + ">";
        vecToList.push_back(to);
    }

    message << "To: " << join(vecToList, ",") << "\r\n";
    message << "Subject: =?" << info.charset << "?b?" << base64Encode(info.subject.c_str(), info.subject.length()) << "?=\r\n";
    message << "MIME-Version: 1.0\r\n";

    if (info.ccEmail.size() > 0)
    {
        std::vector<std::string> vecCcList;
        for (auto item : info.ccEmail)
        {
            std::string cc = "=?" + info.charset + "?b?" + base64Encode(item.first.c_str(), item.first.length()) + "?= <" + item.second + ">";
            vecCcList.push_back(cc);
        }
        message << "Cc:" << join(vecCcList, ",") << "\r\n";
    }

    message << "Content-Type:multipart/mixed; boundary=\"Separator_ztq_000\"\r\n\r\n";
    message << "--Separator_ztq_000\r\n";
    message << "Content-Type: multipart/alternative; boundary=\"Separator_ztq_111\"\r\n\r\n";
    message << "--Separator_ztq_111\r\n";
    message << "Content-Type: " << "text/plain" << "; charset=\"" << info.charset << "\"\r\n";
    message << "Content-Transfer-Encoding: base64\r\n";
    message << "\r\n";                  //此处要加，不然邮件正文无内容
    message << base64Encode(info.message.c_str(), info.message.length());
    message << "\r\n\r\n";
    message << "--Separator_ztq_111--\r\n";
    //---------------------文件部分处理--------------------------------------

    for (auto item : info.attachment)
    {
        std::string filename = fileBasename(item);
        std::string strContext = getFileContents(item.c_str());
        if (strContext.empty())
        {
            std::cerr << "请检查传入的文件路径是否正确,此路径文件添加到附件失败，不发送此文件:" << std::endl;
            std::cerr << item << std::endl;
        }
        else
        {
            std::string fileContext = base64Encode(strContext.c_str(), strContext.length());
            std::string extension = GetFileExtension(filename);
            std::string mimetype = GetMimeTypeFromFileName((char*)extension.c_str());
            message << "--Separator_ztq_000\r\n";
            message << "Content-Type: " << mimetype << "; name=\"" << filename << "\"\r\n";
            message << "Content-Transfer-Encoding: base64\r\n";
            message << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n\r\n";
            message << fileContext + "\r\n\r\n";        //把读取到的文件内容以二进制形式发送
        }
    }
    //-----------------------------------------------------------
    message << "\r\n.\r\n";
    return message.str();
}

SmtpEmail::SmtpEmail(const std::string& emailHost, const std::string& port) :m_host(emailHost), m_port(port)
{

}

SmtpEmail::~SmtpEmail()
{

}

int SmtpEmail::Read(void* buf, int num)
{
    return recv(m_socketfd, (char*)buf, num, 0);
}
int SmtpEmail::Write(const void* buf, int num)
{
    return send(m_socketfd, (char*)buf, num, 0);
}

int SmtpEmail::Connect()
{
#ifdef WIN32
    //start socket connection
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

    m_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socketfd == INVALID_SOCKET)
    {

        m_lastErrorMsg = "Error on creating socket fd.";
        return -1;
    }

    addrinfo inAddrInfo = { 0 };
    inAddrInfo.ai_family = AF_INET;
    inAddrInfo.ai_socktype = SOCK_STREAM;

    //printf("host:%s port:%s \n", m_host.c_str(), m_port.c_str());
    if (getaddrinfo(m_host.c_str(), m_port.c_str(), &inAddrInfo, &m_addrinfo) != 0) // error occurs
    {

        m_lastErrorMsg = "Error on calling getadrrinfo().";
        return -2;
    }


    if (connect(m_socketfd, m_addrinfo->ai_addr, m_addrinfo->ai_addrlen))
    {

        m_lastErrorMsg = "Error on calling connect().";
        return -3;
    }
    return 0;
}

int SmtpEmail::DisConnect()
{
    freeaddrinfo(m_addrinfo);
#ifdef WIN32
    closesocket(m_socketfd);
#else
    close(m_socketfd);
#endif
    return 0;
}

/*********************************************************************************/


std::string SimpleSmtpEmail::GetEmailBody(const EmailInfo& info)
{
    //设定邮件的发送者名称、接收者名称、邮件主题，邮件内容。
    std::ostringstream message;
    message << "From: =?" << info.charset << "?b?" << base64Encode(info.sender.c_str(), info.sender.length()) << "?= <" << info.senderEmail << ">\r\n";

    std::vector<std::string> vecToList;
    for (auto item : info.recvList)
    {
        std::string to = "=?" + info.charset + "?b?" + base64Encode(item.second.c_str(), item.second.length()) + "?= <" + item.first + ">";
        vecToList.push_back(to);
    }

    message << "To: " << join(vecToList, ",") << "\r\n";
    message << "Subject: =?" << info.charset << "?b?" << base64Encode(info.subject.c_str(), info.subject.length()) << "?=\r\n";
    message << "MIME-Version: 1.0\r\n";

    if (info.ccEmail.size() > 0)
    {
        std::vector<std::string> vecCcList;
        for (auto item : info.ccEmail)
        {
            std::string cc = "=?" + info.charset + "?b?" + base64Encode(item.first.c_str(), item.first.length()) + "?= <" + item.second + ">";
            vecCcList.push_back(cc);
        }
        message << "Cc:" << join(vecCcList, ",") << "\r\n";
    }

    message << "Content-Type:multipart/mixed; boundary=\"Separator_ztq_000\"\r\n\r\n";
    message << "--Separator_ztq_000\r\n";
    message << "Content-Type: multipart/alternative; boundary=\"Separator_ztq_111\"\r\n\r\n";
    message << "--Separator_ztq_111\r\n";
    message << "Content-Type: " << "text/plain" << "; charset=\"" << info.charset << "\"\r\n";
    message << "Content-Transfer-Encoding: base64\r\n";
    message << "\r\n";                  //此处要加，不然邮件正文无内容
    message << base64Encode(info.message.c_str(), info.message.length());
    message << "\r\n\r\n";
    message << "--Separator_ztq_111--\r\n";
    //---------------------文件部分处理--------------------------------------

    for (auto item : info.attachment)
    {
        std::string filename = fileBasename(item);
        std::string strContext = getFileContents(item.c_str());
        if (strContext.empty())
        {
            std::cerr << "请检查传入的文件路径是否正确,此路径文件添加到附件失败，不发送此文件:" << std::endl;
            std::cerr << item << std::endl;
        }
        else
        {
            std::string fileContext = base64Encode(strContext.c_str(), strContext.length());
            std::string extension = GetFileExtension(filename);
            std::string mimetype = GetMimeTypeFromFileName((char*)extension.c_str());
            message << "--Separator_ztq_000\r\n";
            message << "Content-Type: " << mimetype << "; name=\"" << filename << "\"\r\n";
            message << "Content-Transfer-Encoding: base64\r\n";
            message << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n\r\n";
            message << fileContext + "\r\n\r\n";        //把读取到的文件内容以二进制形式发送
        }
    }
    //-----------------------------------------------------------
    message << "\r\n.\r\n";
    return message.str();
}

/***************************************************************************************************/

SslSmtpEmail::~SslSmtpEmail()
{

}

int SslSmtpEmail::Connect()
{
    if (SmtpEmail::Connect() == 0)
    {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        m_ctx = SSL_CTX_new(SSLv23_client_method());

        m_ssl = SSL_new(m_ctx);
        SSL_set_fd(m_ssl, m_socketfd);
        SSL_connect(m_ssl);
    }
    return 0;
}

int SslSmtpEmail::DisConnect()
{
    SSL_shutdown(m_ssl);
    SSL_free(m_ssl);
    SSL_CTX_free(m_ctx);

    SmtpEmail::DisConnect();
    return 0;
}



int SmtpEmail::SendEmail(const std::string& from, const std::string& passs, const std::string& to, const std::string& subject, const std::string& strMessage)
{

    EmailInfo info;
    info.charset = "UTF-8";
    info.sender = from;
    info.password = passs;
    info.senderEmail = from;
    info.recipientEmail = to;

    info.recvList[to] = "";

    info.subject = subject;
    info.message = strMessage;

    return SMTPComunicate(info);
}



int SmtpEmail::SendEmail(const std::string& from, const std::string& passs, const std::vector<std::string>& vecTo,
    const std::string& subject, const std::string& strMessage, const std::vector<std::string>& attachment, const std::vector<std::string>& ccList)
{
    std::vector<std::string> recvList;
    recvList.insert(recvList.end(), vecTo.begin(), vecTo.end());
    recvList.insert(recvList.end(), ccList.begin(), ccList.end());

    for (auto& item : recvList)
    {
        EmailInfo info;
        info.charset = "UTF-8";
        info.sender = from;
        info.password = passs;
        info.senderEmail = from;;
        info.recipientEmail = item;

        for (auto item : vecTo)
        {
            info.recvList[item] = "";
        }

        info.subject = subject;
        info.message = strMessage;

        for (auto& item : ccList)
        {
            info.ccEmail[item] = item;
        }

        info.attachment = attachment;
        if (SMTPComunicate(info) != 0)
        {
            return -1;
        }
    }
    return 0;
}



int SslSmtpEmail::Read(void* buf, int num)
{

    int ret = SSL_read(m_ssl, buf, num);

    return ret;
}

int SslSmtpEmail::Write(const void* buf, int num)
{
    return SSL_write(m_ssl, buf, num);
}


std::string SimpleSslSmtpEmail::GetEmailBody(const EmailInfo& info)
{
    //设定邮件的发送者名称、接收者名称、邮件主题，邮件内容。
    std::ostringstream message;
    message << "From: =?" << info.charset << "?b?" << base64Encode(info.sender.c_str(), info.sender.length()) << "?= <" << info.senderEmail << ">\r\n";

    std::vector<std::string> vecToList;
    for (auto item : info.recvList)
    {
        std::string to = "=?" + info.charset + "?b?" + base64Encode(item.second.c_str(), item.second.length()) + "?= <" + item.first + ">";
        vecToList.push_back(to);
    }

    message << "To: " << join(vecToList, ",") << "\r\n";
    message << "Subject: =?" << info.charset << "?b?" << base64Encode(info.subject.c_str(), info.subject.length()) << "?=\r\n";
    message << "MIME-Version: 1.0\r\n";

    if (info.ccEmail.size() > 0)
    {
        std::vector<std::string> vecCcList;
        for (auto item : info.ccEmail)
        {
            std::string cc = "=?" + info.charset + "?b?" + base64Encode(item.first.c_str(), item.first.length()) + "?= <" + item.second + ">";
            vecCcList.push_back(cc);
        }
        message << "Cc:" << join(vecCcList, ",") << "\r\n";
    }
    message << "Content-Type:multipart/mixed; boundary=\"Separator_ztq_000\"\r\n\r\n";
    message << "--Separator_ztq_000\r\n";
    message << "Content-Type: multipart/alternative; boundary=\"Separator_ztq_111\"\r\n\r\n";
    message << "--Separator_ztq_111\r\n";
    message << "Content-Type: " << "text/plain" << "; charset=\"" << info.charset << "\"\r\n";
    message << "Content-Transfer-Encoding: base64\r\n";
    message << "\r\n";                  //此处要加，不然邮件正文无内容
    message << base64Encode(info.message.c_str(), info.message.length());
    message << "\r\n\r\n";
    message << "--Separator_ztq_111--\r\n";
    //---------------------文件部分处理--------------------------------------

    for (auto item : info.attachment)
    {
        std::string filename = fileBasename(item);
        std::string strContext = getFileContents(item.c_str());
        if (strContext.empty())
        {
            std::cerr << "请检查传入的文件路径是否正确,此路径文件添加到附件失败，不发送此文件:" << std::endl;
            std::cerr << item << std::endl;
        }
        else
        {
            std::string fileContext = base64Encode(strContext.c_str(), strContext.length());
            std::string extension = GetFileExtension(filename);
            std::string mimetype = GetMimeTypeFromFileName((char*)extension.c_str());
            message << "--Separator_ztq_000\r\n";
            message << "Content-Type: " << mimetype << "; name=\"" << filename << "\"\r\n";
            message << "Content-Transfer-Encoding: base64\r\n";
            message << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n\r\n";
            message << fileContext + "\r\n\r\n";        //把读取到的文件内容以二进制形式发送
        }
    }
    //-----------------------------------------------------------
    message << "\r\n.\r\n";
    return message.str();
}
