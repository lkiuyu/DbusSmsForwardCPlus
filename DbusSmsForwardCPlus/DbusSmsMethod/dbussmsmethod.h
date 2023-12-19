#ifndef DBUSSMSMETHOD_H
#define DBUSSMSMETHOD_H

#include <dbus/dbus.h>
#include <regex>
#include <thread>
#include "../ProcessSmsSend/sendoptionprocess.h"

using namespace std;

void getAndSendSmsContent(string sendMethodGuideResult, const char* smsPath, uint32_t storageTypeNum);
void parseDBusMessageAndSend(DBusMessage* message, string sendMethodGuideResult);
void monitorDbus(string sendMethodGuideResult);
void sendSms(string telNumber, string smsText, string target);

void getAndSendSmsContent(string sendMethodGuideResult, const char* smsPath, uint32_t storageTypeNum) {
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
    bool isIgnore = false;
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
                else if (keyName == "Storage") {

                    uint32_t storage;
                    // 移动到字典项的值
                    dbus_message_iter_next(&dictIter);
                    DBusMessageIter variantIter;
                    dbus_message_iter_recurse(&dictIter, &variantIter);
                    dbus_message_iter_get_basic(&variantIter, &storage);
                    if (storageTypeNum == 100) {
                        isIgnore = false;
                    }
                    else if (storage == storageTypeNum) {
                        isIgnore = true;
                    }
                    else
                    {
                        isIgnore = false;
                    }
                }
            }
            // 移动到下一个字典项
            dbus_message_iter_next(&arrayIter);
        }
    }
    if (!isIgnore) {
        if (string(smscontent) != "")
        {
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
            getAndSendSmsContent(sendMethodGuideResult, smsPath, storageTypeNum);
        }
    }
    else {
        cout << "已过滤不转发" << endl;
    }
}
//处理dbus获取到的消息并转发
void parseDBusMessageAndSend(DBusMessage* message, string sendMethodGuideResult)
{
    map<string, string> configMap;
    // 读取配置文件
    configMap = readConfigFile();
    string forwardStorageType = configMap["forwardIgnoreStorageType"];
    uint32_t storageTypeNum = 100;
    if (forwardStorageType == "unknown") {
        storageTypeNum = 0;
    }
    else if (forwardStorageType == "sm") {
        storageTypeNum = 1;
    }
    else  if (forwardStorageType == "me") {
        storageTypeNum = 2;
    }
    else  if (forwardStorageType == "mt") {
        storageTypeNum = 3;
    }
    else  if (forwardStorageType == "sr") {
        storageTypeNum = 4;
    }
    else  if (forwardStorageType == "bm") {
        storageTypeNum = 5;
    }
    else  if (forwardStorageType == "ta") {
        storageTypeNum = 6;
    }

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
                getAndSendSmsContent(sendMethodGuideResult, smsPath, storageTypeNum);
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
void sendSms(string telNumber, string smsText, string target) {
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
        if (target == "command") {
            printf("短信创建成功，是否发送？(1.发送短信,其他按键退出程序)\n");
            getline(cin, sendChoise);
        }

        if (sendChoise == "1" || target == "api")
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

#endif // DBUSSMSMETHOD_H
