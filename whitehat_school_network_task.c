#include <stdlib.h>
#include <stdio.h>
#include <pcap.h>
#include <ctype.h>
#include <arpa/inet.h>


/* Ethernet header */ //데이터 링크 계층 구조체 MAC 주소 뽑아내기 
struct ethheader {
  u_char  ether_dhost[6]; /* destination host address */
  u_char  ether_shost[6]; /* source host address */
  u_short ether_type;     /* protocol type (IP, ARP, RARP, etc) */
};

/* IP Header */ //네트워크 계층 구조체 IP 주소 뽑아내기 
struct ipheader {
  unsigned char      iph_ihl:4, //IP header length
                     iph_ver:4; //IP version
  unsigned char      iph_tos; //Type of service
  unsigned short int iph_len; //IP Packet length (data + header)
  unsigned short int iph_ident; //Identification
  unsigned short int iph_flag:3, //Fragmentation flags
                     iph_offset:13; //Flags offset
  unsigned char      iph_ttl; //Time to Live
  unsigned char      iph_protocol; //Protocol type
  unsigned short int iph_chksum; //IP datagram checksum
  struct  in_addr    iph_sourceip; //Source IP address
  struct  in_addr    iph_destip;   //Destination IP address
};

struct tcpheader { // 전송 계층의 tcp 프로토콜 구조체 
  u_short tcp_sport;               /* source port */
  u_short tcp_dport;               /* destination port */
  u_int   tcp_seq;                 /* sequence number */
  u_int   tcp_ack;                 /* acknowledgement number */
  u_char  tcp_offx2;               /* data offset, rsvd */
#define TH_OFF(th)      (((th)->tcp_offx2 & 0xf0) >> 4)
  u_char  tcp_flags;
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80
#define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
  u_short tcp_win;                 /* window */
  u_short tcp_sum;                 /* checksum */
  u_short tcp_urp;                 /* urgent pointer */
};


void got_packet(u_char *args, const struct pcap_pkthdr *header,
                              const u_char *packet)
{
  struct ethheader *eth = (struct ethheader *)packet;

 

  if (ntohs(eth->ether_type) == 0x0800) { // 0x0800 is IP type
    struct ipheader * ip = (struct ipheader *)
                           (packet + sizeof(struct ethheader)); 

    struct tcpheader *tcp = (struct tcpheader *)(packet + sizeof(struct ethheader) + ip->iph_ihl * 4);

    printf("       ether_src_mac: ");
    for (int i = 0; i < 6; i++) {
      printf("%02x", eth->ether_shost[i]);
  }
  printf("\n");
  printf("       ether_dest_mac: ");
    for (int i = 0; i < 6; i++) {
      printf("%02x", eth->ether_dhost[i]);
  }
  printf("\n");

    printf("       From: %s\n", inet_ntoa(ip->iph_sourceip));   
    printf("         To: %s\n", inet_ntoa(ip->iph_destip));    
    printf("       tcp_src_port: %u\n", ntohs(tcp->tcp_sport));
    printf("       tcp_dest_port: %u\n", ntohs(tcp->tcp_dport));

    int ip_header_len = ip->iph_ihl * 4;
    int tcp_header_len = TH_OFF(tcp) * 4;
    int total_len = ntohs(ip->iph_len);

    const u_char *message = packet + sizeof(struct ethheader) + ip_header_len + tcp_header_len;
    int message_len = total_len - ip_header_len - tcp_header_len;

    printf("       Message (%d bytes): ", message_len);
    for (int i = 0; i < message_len && i < 64; i++) {
      if (isprint(message[i])) {
          printf("%c", message[i]);
      } else {
          printf(".");
      }
  }
  printf("\n");

    /* determine protocol */
    switch(ip->iph_protocol) {                                 
        case IPPROTO_TCP:
            printf("   Protocol: TCP\n");
            return;
        case IPPROTO_UDP:
            printf("   Protocol: UDP\n");
            return;
        case IPPROTO_ICMP:
            printf("   Protocol: ICMP\n");
            return;
        default:
            printf("   Protocol: others\n");
            return;
    }
  }
}

int main()
{
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  struct bpf_program fp;
  char filter_exp[] = "tcp";
  bpf_u_int32 net;

  // Step 1: Open live pcap session on NIC with name enp0s3
  handle = pcap_open_live("enp0s3", BUFSIZ, 1, 1000, errbuf);

  // Step 2: Compile filter_exp into BPF psuedo-code
  pcap_compile(handle, &fp, filter_exp, 0, net);
  if (pcap_setfilter(handle, &fp) !=0) {
      pcap_perror(handle, "Error:");
      exit(EXIT_FAILURE);
  }

  // Step 3: Capture packets
  pcap_loop(handle, -1, got_packet, NULL);

  pcap_close(handle);   //Close the handle
  return 0;
} 
