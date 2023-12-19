#ifndef INITWEBPAGEANDAPI_H
#define INITWEBPAGEANDAPI_H

#include <regex>
#include "../httplib.h"
#include "../DbusSmsMethod/dbussmsmethod.h"

using namespace httplib;
using namespace std;



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

    svr.Get("/", [](const Request& req, Response& res) {
        string htmlContent = R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
  <style>
  html, body {
            height: 100%;
            display: flex;
            justify-content: center;
            align-items: center;
            background-color: #f7f7f7;
        }
        .container {
            text-align: center;
            padding: 20px;
            background-color: #fff;
            border-radius: 5px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            width: 90vw;
            max-width: 500px;
        }
        input[type='text'], textarea {
          width: 80%;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
            font-size: 20px;
        }
        button {
            padding: 10px 20px;
            background-color: #007bff;
            color: #fff;
            border: none;
            border-radius: 5px;
            font-size: 20px;
            cursor: pointer;
        }
        button:hover {
            background-color: #0056b3;
        }
</style>
    <title>短信发送</title>
    <script>
        function sendSMS() {
            var phoneNumber = encodeURIComponent(document.getElementById('phone').value);
            var message = encodeURIComponent(document.getElementById('message').value);
			var xhr = new XMLHttpRequest();
			var url=window.location.protocol+'//'+window.location.hostname+':'+window.location.port+'/api?telnum='+phoneNumber+'&smstext='+message;
		    xhr.open('GET', url, true);
		    xhr.onreadystatechange = function () 
			{
				if (xhr.readyState === 4 && xhr.status === 200) {
				  alert('已发送');
				}
		    };
		    xhr.send();
        }
    </script>
</head>
<body>
      <div class='container'>
    <h1>短信发送</h1>
    <form>
        <label for='phone'>收信号码:</label>
        <input type='text' id='phone' name='phone' required><br><br>
        <label for='message'>短信内容:</label>
        <textarea id='message' name='message' required></textarea><br><br>
        <button type='button' onclick='sendSMS()'>发送</button>
    </form>
      </div>
</body>
</html>)";
        res.set_content(htmlContent, "text/html");
        });
    // 处理 GET 请求，路径为 /api
    svr.Get("/api", [](const Request& req, Response& res) {
        handle_request(req, res);
        });
    cout << "短信发送webapi接口已运行在" + apiPort + "端口" << endl;
    svr.listen("0.0.0.0", sapiPort);
}

#endif // INITWEBPAGEANDAPI_H
