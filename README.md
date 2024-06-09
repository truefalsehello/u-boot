# u-boot
All-in-One Android Touch Screen(Smart6818 CPU Boards，SoC S5P6818)开发板的u-boot，增加ping,tftp命令(tftpboot 下载文件命令，tftpput 上传文件命令)

tftpboot - boot image via network using TFTP protocol
Usage:
tftpboot [loadAddress] [[hostIPaddr:]bootfilename]

tftpput - TFTP put command, for uploading files to a server
Usage:
tftpput Address Size [[hostIPaddr:]filename]

ping - send ICMP ECHO_REQUEST to network host
Usage:
ping pingAddress
