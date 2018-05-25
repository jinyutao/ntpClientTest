#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define PORT (123)
#define NTP_IP ("182.92.12.11")  // ntp1.aliyun.com

#define PADDED_2_US(us) ((us * 1000000.0 / UINT32_MAX) + 0.5)

char* ntp_timesnap_2_str(uint32_t s,uint32_t us)
{
    static char tmp[512];
    memset(tmp,0,sizeof(tmp));
    double ret = PADDED_2_US(us);

    time_t tt = (time_t)s;
    struct tm *ptm = gmtime(&tt);
    long len = strftime(tmp,sizeof(tmp),"%F %T",ptm);
    sprintf(tmp + len,".%06ld",(uint64_t)ret);
    return tmp;
}

uint32_t convert_us_padded(uint64_t us)
{
    double ret = us * UINT32_MAX / 1000000.0;
    ret+=0.5;
    return (uint32_t)ret;
}

void print_timeinfo(char * msg,uint32_t s,uint32_t us)
{
    printf("%s: %s %08X-%08X\n",
        msg, ntp_timesnap_2_str(ntohl(s) - 2208988800u,ntohl(us)),s,us);
}

void print_bit(char * p,int l)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint32_t Reference = (*((uint32_t*)(p+16)));
    uint32_t Reference_us =(*((uint32_t*)(p+20)));
    uint32_t Originate = (*((uint32_t*)(p+24)));
    uint32_t Originate_us = (*((uint32_t*)(p+28)));
    uint32_t Receive = (*((uint32_t*)(p+32)));
    uint32_t Receive_us = (*((uint32_t*)(p+36)));
    uint32_t Transmit = (*((uint32_t*)(p+40)));
    uint32_t Transmit_us = (*((uint32_t*)(p+44)));

    printf("\nRemote NTP time\n");
    print_timeinfo(" Reference Timestamp -- ",Reference,Originate_us);
    print_timeinfo(" Originate Timestamp(T1)",Originate,Originate_us);
    print_timeinfo(" Receive Timestamp  (T2)",Receive,Receive_us);
    print_timeinfo(" Transmit Timestamp (T3)",Transmit,Transmit_us);

    Originate = ntohl(Originate) - 2208988800u;
    double T1 = Originate * 1000000.0 + PADDED_2_US(ntohl(Originate_us));
    Receive = ntohl(Receive) - 2208988800u;
    double T2 = Receive * 1000000.0 + PADDED_2_US(ntohl(Receive_us));
    Transmit = ntohl(Transmit) - 2208988800u;
    double T3 = Transmit * 1000000.0 + PADDED_2_US(ntohl(Transmit_us));
    double T4 = (tv.tv_sec * 1000000.0 + tv.tv_usec);

    char tmp[512];
    memset(tmp,0,sizeof(tmp));
    struct tm *ptm = gmtime(&tv.tv_sec);
    strftime(tmp,sizeof(tmp),"%F %T",ptm);
    printf("\nLoacl receive time(T4): %s.%06ld\n\n", tmp, tv.tv_usec);

    uint64_t offset_time_us = (uint64_t)(((T2-T1)+(T3-T4))/2);

    tv.tv_usec += offset_time_us;
    tv.tv_sec += (tv.tv_usec / 1000000);
    tv.tv_usec %= 1000000;
    ptm = gmtime(&tv.tv_sec);
    strftime(tmp,sizeof(tmp),"%F %T",ptm);
    printf("Offset time form NTP-server to local ((T2-T1)+(T3-T4))/2: = %ldus\n", offset_time_us);
    printf("Synced time = %s.%06ld\n\n", tmp, tv.tv_usec);
}

int main(int agvc,char* agvr[])
{
    int addr_len;
    unsigned char message[256];

    int socket_descriptor;
    struct sockaddr_in addr;

    char * p_ip = NTP_IP;

    if(agvc>=2)
    {
        p_ip = agvr[1];
    }

    unsigned char tmp[]={
        0xdb, 0x00, 0x11, 0xe9,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};

    printf("send to ip: %s \n",p_ip);
    bzero(&addr,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=inet_addr(p_ip);
    addr.sin_port=htons(PORT);
    addr_len=sizeof(addr);
    char flg = 1;
    socket_descriptor=socket(AF_INET,SOCK_DGRAM,0);
    //while(flg)
    {

        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint32_t s = htonl(tv.tv_sec + 2208988800u);
        uint32_t us = convert_us_padded(tv.tv_usec);
        us = htonl(us);
        // Transmit Timestamp
        memcpy(tmp + 40, &s, 4);
        memcpy(tmp + 44, &us, 4);
        print_timeinfo("local Transmit Timestamp",s,us);

        int r =sendto(socket_descriptor, tmp, sizeof(tmp), 0, (struct sockaddr *)&addr, addr_len);
        if(r < 0)
        {
            printf("send fail!!!(%d)%s \n", errno, strerror(errno));
        }

        int getdatalen = recvfrom(
                socket_descriptor,
                message,
                sizeof(message),
                0,
                NULL,
                NULL);

        flg  = 0;
        if(getdatalen == sizeof(tmp))
        {
            flg  = 1;
            int i;
            for(i = 0; i < 8; i++)
            {
                if(tmp[40+i] != message[24+i])
                {
                    flg  = 0;
                    break;
                }
            }
        }
        print_bit(message,getdatalen);
        printf("recvfrom() = %d. Data is [%s]\n",getdatalen,flg==1?"OK":"Err");
    }
    close(socket_descriptor);
    printf("exit\n");
    return EXIT_SUCCESS;
}
