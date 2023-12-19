#ifndef PROCESSUSERCHOOSE_H
#define PROCESSUSERCHOOSE_H
#include <regex>
#include <iostream>

using namespace std;

string onStartGuide(string chooseOption);

string onStartGuide(string chooseOption)
{
    if (chooseOption == "")
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


#endif // PROCESSUSERCHOOSE_H
