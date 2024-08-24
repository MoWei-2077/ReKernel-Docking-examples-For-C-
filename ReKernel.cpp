#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <netinet/tcp.h>
#include <unistd.h>

#define PACKET_SIZE      128
#define NETLINK_UNIT     22
#define USER_PORT        100
#define MAX_PLOAD        125
#define MSG_LEN          125

typedef struct _user_msg_info
{
    struct nlmsghdr hdr;
    char  msg[MSG_LEN];
} user_msg_info;

int main() {
    int skfd;
    int ret;
    user_msg_info u_info;
    socklen_t len;
    struct nlmsghdr nlh;
    struct sockaddr_nl saddr, daddr;
    std::string umsg = "Hello! Re:Kernel!";

    skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_UNIT);
    if (skfd == -1) {
        perror("创建NetLink失败\n");
        return -1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = USER_PORT;
    saddr.nl_groups = 0;
    if (bind(skfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
        perror("连接Bind失败\n");
        close(skfd);
        return -1;
    }

    memset(&daddr, 0, sizeof(daddr));
    daddr.nl_family = AF_NETLINK;
    daddr.nl_pid = 0;
    daddr.nl_groups = 0;

    memset(&nlh, 0, sizeof(nlh));
    nlh.nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
    nlh.nlmsg_flags = 0;
    nlh.nlmsg_type = 0;
    nlh.nlmsg_seq = 0;
    nlh.nlmsg_pid = saddr.nl_pid;

    memcpy(NLMSG_DATA(&nlh), umsg.c_str(), umsg.size());

    ret = sendto(skfd, &nlh, nlh.nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
    if (ret == -1) {
        perror("向ReKernel发送消息失败！\n");
        close(skfd);
        return -1;
    }

    while (true) {
        memset(&u_info, 0, sizeof(u_info));
        len = sizeof(struct sockaddr_nl);
        ret = recvfrom(skfd, &u_info, sizeof(user_msg_info), 0, (struct sockaddr *)&daddr, &len);
        if (ret == -1) {
            perror("从ReKernel接收消息失败！\n");
            close(skfd);
            return -1;
        }

        std::cout << "来自ReKernel的消息: " << u_info.msg << std::endl;
          /*
          这时候ReKernel会发送的消息可能是type=Network 这是网络解冻你需要去进行解冻 target=xxxxx; 这里指的是uid 你需要给uid进行解冻操作
          如果是这样的消息type=binder 你就需要使用uid 进行解冻Binder解冻操作 同时你需要临时解冻一会
          */
        const char *ptr = strstr(u_info.msg, "target=");
        if (ptr != nullptr) {
            int uid = atoi(ptr + 7);
            std::cout << "触发Binder事件,需要进行临时解冻的uid为:" << uid << std::endl;
            // your code 在这里你需要填写你的解冻代码
        }
    }

    close(skfd);
    return 0;
}