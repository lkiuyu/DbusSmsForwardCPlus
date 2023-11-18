#!/bin/bash
# 接受命令行参数
telnum=$1
smsdate=$2
smscontent=$3
smscode=$4
smscodefrom=$5
devicename=$6

pushtitle='短信转发 '$telnum
pushcontent='发信电话:'$telnum'\n时间:'$smsdate'\n转发设备:'$devicename'\n短信内容:'$smscontent

if [ -n "$smscode" ]; then
  pushtitle=$smscodefrom$smscode' '$pushtitle
fi

curl --location 'http://www.pushplus.plus/send/' \
--header 'Content-Type: application/json' \
--data '{"token":"你的pushplustoken","title":"'"$pushtitle"'","content":"'"$pushcontent"'"}'
