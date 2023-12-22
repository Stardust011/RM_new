
# 创建虚拟串口
socat -d -d pty,raw,echo=0 pty,raw,echo=0
# 监控虚拟串口
# cat < /dev/pts/7 | hexdump -C