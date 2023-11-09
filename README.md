# DbusSmsFowardCPlus
用于部分随身wifi刷了Debian或openwrt后的短信的Email、PushPlus、企业微信自建应用、TG机器人、钉钉机器人消息转发、Bark推送转发、自定义shell脚本转发以及短信发送，通过监听dbus实时获取新接收的短信并转发以及调用dbus发送短信
# 使用教程

**Debian系统下注意事项:**

1.解压程序至home(你也可以换别的文件夹)文件夹下

2.输入 
sudo chmod -R 777 DbusSmsForwardCPlus
配置程序可执行权限

3.输入
sudo ./DbusSmsForwardCPlus
运行程序

4.根据提示配置相关转发信息

5.带参数运行跳过程序初始的运行模式选择以达到快速运行程序

输入
sudo ./DbusSmsForwardCPlus -fE
跳过运行模式选择直接进入邮箱转发模式

输入
sudo ./DbusSmsForwardCPlus -fP
跳过运行模式选择直接进入PushPlus转发模式

输入
sudo ./DbusSmsForwardCPlus -fW
跳过运行模式选择直接进入企业微信转发模式

输入
sudo ./DbusSmsForwardCPlus -fT
跳过运行模式选择直接进入TGBot转发模式

输入
sudo ./DbusSmsForwardCPlus -fD
跳过运行模式选择直接进入钉钉转发模式

输入
sudo ./DbusSmsForwardCPlus -fB
跳过运行模式选择直接进入Bark转发模式

输入
sudo ./DbusSmsForwardCPlus -fS
跳过运行模式选择直接进入Shell转发模式

输入
sudo ./DbusSmsForwardCPlus -sS
跳过运行模式选择直接进入短信发送界面

输入
sudo ./DbusSmsForwardCPlus --configfile=/root/config.txt
可加载自定义路径的配置文件

输入
sudo ./DbusSmsForwardCPlus --sendsmsapi=enable
可开启短信发送webapi接口

**举例**

sudo ./DbusSmsForwardCPlus -fB --configfile=/root/config.txt
启动到bark转发模式，并使用root路径下的config.txt配置文件

sudo ./DbusSmsForwardCPlus -fB --configfile=/root/config.txt --sendsmsapi=enable
启动到bark转发模式，使用root路径下的config.txt配置文件，并开启发送短信webapi接口

sudo ./DbusSmsForwardCPlus --configfile=/root/config.txt --sendsmsapi=enable
使用root路径下的config.txt配置文件，并开启发送短信webapi接口

sudo ./DbusSmsForwardCPlus --sendsmsapi=enable
开启发送短信webapi接口


**shell转发模式注意事项：**

shell转发模式下，程序会调用你指定路径的shell脚本文件，程序会传入五个参数，分别为如下

telnum 发信电话号码

smsdate 短信收信日期

smscontent 短信内容

smscode 短信验证码（如果存在的话）

smscodefrom 验证码来源（如果存在的话）

在shell脚本内可对这5个参数进行自定义组合，发送到你自定义的渠道，目前在ShellExample文件夹内存放了一份发送到pushplus的shell脚本示例以供参考

**openwrt系统下注意事项：**

直接安装releases页提供的软件包，终端内输入DbusSmsForwardCPlus即可运行，无需sudo ./
程序运行生成的配置文件存储路径会在你运行命令时所处的路径，比如你当前在/root/的路径下下输入命令运行了程序，那配置文件就会在root目录下
在rc.local文件设置自启动时可加载自定义路径的配置文件
举例在rc.loacl中设置自启动的命令
( DbusSmsForwardCPlus -fE --configfile=/root/config.txt > /dev/null ) &


# 参考
1. [ModemManager API document](https://www.freedesktop.org/software/ModemManager/api/latest/)
2. ChatGPT
