#ifndef MY_HELPER_H
#define MY_HELPER_H

#include <iomanip>
#include <cassert>
#include <regex>
#include <unordered_set>
#include <unistd.h>

using namespace std;

string trim(string strinput);
void replaceChar(string& str, char targetChar, char replacementChar);
void replaceString(string& str, string targetStr, string replacementStr);
vector<string> SplitCodeKeyString(const string& str, const string& delimiter);
vector<string> splitCNString(const string& input, const string& delimiter);
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* response);
unsigned char ToHex(unsigned char x);
unsigned char FromHex(unsigned char x);
string UrlEncodeUTF8(const string& str);
string UrlEncode(const string& str);
string UrlDecode(const string& str);
void removeDuplicates(vector<string>& vec);
string GetDeviceHostName();

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
void removeDuplicates(vector<string>& vec) {
    unordered_set<string> uniqueSet;
    vec.erase(remove_if(vec.begin(), vec.end(), [&uniqueSet](const string& str) {
        return !uniqueSet.insert(str).second;
        }), vec.end());
}
string GetDeviceHostName() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return hostname;
    }
    else {
        return "";
    }
}
#endif // MY_HELPER_H