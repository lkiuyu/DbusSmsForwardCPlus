# DbusSmsFowardCPlus
用于部分随身wifi刷了Debian或openwrt后的短信的Email、PushPlus、企业微信自建应用、TG机器人、钉钉机器人消息转发、Bark推送转发以及短信发送，通过监听dbus实时获取新接收的短信并转发以及调用dbus发送短信
# 使用教程

Debian系统下

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
sudo ./DbusSmsForwardCPlus -sS
跳过运行模式选择直接进入短信发送界面

openwrt系统下

直接安装releases页提供的软件包，终端内输入DbusSmsForwardCPlus即可运行，无需sudo ./
程序运行生成的配置文件存储路径会在你运行命令时所处的路径，比如你当前在/root/的路径下下输入命令运行了程序，那配置文件就会在root目录下

# 参考
1. [ModemManager API document](https://www.freedesktop.org/software/ModemManager/api/latest/)
2. ChatGPT
