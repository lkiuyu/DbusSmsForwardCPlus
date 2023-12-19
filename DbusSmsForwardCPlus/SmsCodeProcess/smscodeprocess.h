#ifndef SMSCODEPROCESS_H
#define SMSCODEPROCESS_H

#include <iomanip>
#include <regex>
#include "../MyHelper/myhelper.h"
#include "../ConfigFileProcess/configfileprocess.h"

using namespace std;

vector<string> extractAllContent(const string& input);
bool JudgeSmsContentHasCode(string& smscontent);
int CountDigits(const string& str);
string GetCode(const string& smsContent);
string GetCodeSmsFrom(const string& smsContent);
string GetSmsCodeStr(string smscontent, string& smscode, string& CodeFrom);


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
//判断短信内是否含有验证码
bool JudgeSmsContentHasCode(string& smscontent) {
    string filename = "config.txt";
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string smsKeysStr = configMap["smsCodeKey"];
    if (smsKeysStr == "") {
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
        if (splitStrings[0] == "【") {
            return "【" + content + "】";
        }
        else if (splitStrings[splitStrings.size() - 1] == "】") {
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

#endif // SMSCODEPROCESS_H
