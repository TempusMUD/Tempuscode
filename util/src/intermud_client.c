/************************************************************************
* File: intermud_client.c                                               *
*                                                                       *
* Server deals with intermud related messages                           *
*                                                                       *
* All rights reserved.  See /doc/license.doc for more information       *
*                                                                       *
* Copyright (C) 1995, 96 by Chris Austin (Stryker@Tempus)               *
* CircleMUD (C) 1993, 94 by the trustees of Johns Hopkins University.   *
* CircleMUD is based on DikuMUD, which is Copyright (C) 1990, 1991.     *
*                                                                       *
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/un.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>
#include "intermud.h"

#ifdef NeXT /* Next compatibilty curtesy of Kilian of MultiMUD */
#include <libc.h>
#define O_NONBLOCK      00004   /* non-blocking open */
#undef DEBUG
#endif


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

#define DEBUG(x)  if (debug == 1) interlog x

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))

#define CREATE(result, type, number)  do  {                        \
    if (!((result) = (type *) calloc ((number), sizeof(type))))\
    {perror("malloc failure"); abort(); } } while(0)

#define REMOVE_FROM_LIST(item, head, next)       \
if ((item) == (head))		         \
    head = (item)->next;		         \
else {				         \
    temp = head;			         \
    while (temp && (temp->next != (item)))    \
	temp = temp->next;		         \
    if (temp)				         \
	temp->next = (item)->next;	            \
}
/* Service #defines, don't change or add to these */

#define ST_INTERWIZ      (1 << 0)
#define ST_INTERTELL     (1 << 1)
#define ST_INTERPAGE     (1 << 2)
#define ST_INTERWHO      (1 << 3)
#define ST_INTERBOARD    (1 << 4)


#define CH_INTERWIZ      (1 << 0)

#define BD_WIZARD        (1 << 0)

/* Structures */

struct dns_record {
    unsigned long      ip_address;
    unsigned short     port;
    char               mud_name[50];
    char               mud_port[8];
    char               mud_version[20];
    char               intermud_version[20];
    int                mud_type;
    unsigned long      kb_in;
    unsigned long      kb_out;
    unsigned long      msg_in;
    unsigned long      msg_out;
    int                time_to_live;
    int                muted;
    int                bootmaster;
    long               services;
    long               boards;
    long               channels;
    struct dns_record *next;
};

struct bullet  {
    int bytes;
} bullet =  {0};


/* Global variables */

int debug = 1;
int net_sockfd;
int mud_sockfd;
int intermud_shutdown = 0;
struct sockaddr_un mud_addr;
int mudaddrlen;
struct dns_record *muddns = NULL;
struct dns_record *dns_table = NULL;
char   message[MAX_NET_MESSAGE+500];
char   message2[MAX_NET_MESSAGE+500];
char logbuf[700];
char services[8];
char *Service = 0;
char *To = 0;
char *From = 0;
char *Remote_Mud = 0;
char *Our_Mud = 0;
char *Message = 0;
char *Text = 0;
char *Mud_Port = 0;
char *Intermud_Port = 0;
char *Remote_IP_addr = 0;
char *Mud_Version = 0;
char *Intermud_Version = 0;
char *Mud_Type = 0;
char *Services = 0;

/* Local function prototypes */

void   init_net_socket(int port);
void   mud_connect(void);
void   server_loop(void);
void   recv_net_message(void);
void   recv_mud_message(void);
struct dns_record *check_dns_record(struct sockaddr_in remote_address);
void   purge_dns_table(void);
void   net_send_startup(void);
void   net_send_mudlist(struct dns_record *remote_mud);
void   net_recv_startup(char *net_message, struct sockaddr_in remote_address);
void   net_recv_interwiz(char *net_message, struct sockaddr_in remote_address);
void   net_recv_intertel(char *net_message, struct sockaddr_in remote_address);
void   net_recv_interwho(char *net_message);
void   net_recv_interpage(char *net_message, struct sockaddr_in remote_address);
void   net_recv_mudlist(char *net_message);
void   net_recv_pingrpy(char *net_message, struct sockaddr_in remote_address);
void   net_recv_pingreq(char *net_message, struct sockaddr_in remote_address);
void   net_recv_dnsupdate(char *net_message, struct sockaddr_in remote_address);
void   net_recv_mudlist_req(char *net_message, struct sockaddr_in remote_address);
void   mud_recv_mudlist_req(char *message);
void   mud_recv_intertell(char *message);
void   mud_recv_interwho(char *message);
void   mud_recv_interpage(char *message);
void   mud_recv_interwiz(char *message);
void   mud_recv_mudinfo(char *message);
void   mud_recv_dnspurge(char *message);
void   mud_recv_reget(char *message);
void   mud_recv_stats(char *message);
void   mud_recv_mute(char *message);
void   mud_recv_debug(char *message);
void   update_statistics(struct dns_record *remote_mud, int direction, int msg_size);
void   net_keep_alive(void);
int    validate_net_message(struct sockaddr_in remote_address, char *net_message);
void   interlog(char *fmt, ...);
int    mud_recv_data(int fd, char *buf);
int    mud_send_data(int fd, char *buf);
int    str_cmp(char *arg1, char *arg2);
char  *calc_services(void);
char  *encode_services(struct dns_record *remote_mud);
void   decode_services(char *bitmap, struct dns_record *remote_mud);


int main (int argc, char **argv)
{
    int port;
  
    if (argc != 2)  {
	fprintf(stderr, "Usage: %s [net port #]\n", argv[0]);
	exit(1);
    }
  
    if (!isdigit(*argv[1]))  {
	fprintf(stderr, "Usage: %s [net port #]\n", argv[0]);
	exit(1);
    }
    else if ((port = atoi(argv[1])) <= 1024)  {
	fprintf(stderr, "Illegal port number specified.\n");
	exit(1);
    }
  
    init_net_socket(port);

    mud_connect();
  
    server_loop();
  
    return(0);
}


void net_send_startup(void)
{
    char startup[160];
    struct sockaddr_in bm_addr;
    struct dns_record *dummy = 0;
    char *services_buffer = 0;
  
    if (dns_table == NULL)  {
  
	/* Now we will add ourselves */
    
	CREATE(dummy, struct dns_record, 1);
    
	dummy->ip_address = inet_addr(MUDIPADDR);
	dummy->port = htons(atoi(INTERMUD_PORT));
	dummy->time_to_live = 3;
	strcpy(dummy->mud_name, MUDNAME);
	strcpy(dummy->mud_port, MUDPORT);
	strcpy(dummy->mud_version, MUD_VERSION);
	strcpy(dummy->intermud_version, INTERMUD_VERSION);
	dummy->mud_type = MUD_TYPE;
    
	if (SRV_INTERWIZ == 1)
	    SET_BIT(dummy->services, ST_INTERWIZ);
	if (SRV_INTERTELL == 1)
	    SET_BIT(dummy->services, ST_INTERTELL);
	if (SRV_INTERPAGE == 1)
	    SET_BIT(dummy->services, ST_INTERPAGE);
	if (SRV_INTERWHO == 1)
	    SET_BIT(dummy->services, ST_INTERWHO);
	if (SRV_INTERBOARD == 1)
	    SET_BIT(dummy->services, ST_INTERBOARD);
    
	dummy->next = dns_table;
	dns_table = dummy;

	muddns = dummy;
    
	DEBUG(("INFO: Added DNS record for myself\n"));
    }
  
    bzero((char *) &bm_addr, sizeof(bm_addr));
    bm_addr.sin_family = AF_INET;
    bm_addr.sin_addr.s_addr = inet_addr(BOOTMASTER_IP);
    bm_addr.sin_port = htons(BOOTMASTER_PORT);
  
    services_buffer = encode_services(muddns);
  
    sprintf(startup, "1000|%s|%s|%s|%d|%s|%s|%s|", INTERMUD_PORT, MUDNAME, MUDPORT, MUD_TYPE, 
	    MUD_VERSION, INTERMUD_VERSION, services_buffer);
  
    DEBUG(("SNET: %s\n", startup));
  
    if (sendto(net_sockfd, startup, strlen(startup), 0, (struct sockaddr *) &bm_addr, sizeof(bm_addr)) != strlen(startup))
	fprintf(stderr, "bugs in sending to server\n");
  
    DEBUG(("INFO: Sent startup message to BOOTMASTER, IP = %s\n", inet_ntoa(bm_addr.sin_addr)));
}


void server_loop(void)
{
    struct timeval timeout;
    fd_set input_set, except_set;
    int    retval;
  
    timeout.tv_sec = 300;
    timeout.tv_usec = 0;
  
  
    while (!intermud_shutdown)  {
    
	FD_ZERO(&input_set);
	FD_ZERO(&except_set);
	FD_SET(net_sockfd, &input_set);
	FD_SET(mud_sockfd, &input_set);
	FD_SET(mud_sockfd, &except_set);
	FD_SET(net_sockfd, &except_set);
    
	errno = 0;
    
	retval = select(mud_sockfd + 1, &input_set, (fd_set *) 0, &except_set, &timeout);
	if (retval < 0)
	    interlog("INFO: Waking up to process signal\n");
	else  {
	    if (retval == 0)  {
		net_keep_alive();
		timeout.tv_sec = 300;
	    }
	    if (FD_ISSET(net_sockfd, &except_set))
		DEBUG(("ERRO: Exception detected on net socket\n"));
	    if (FD_ISSET(mud_sockfd, &except_set))  {
		DEBUG(("ERRO: Exception detected on mud socket\n"));
		FD_CLR(mud_sockfd, &input_set);
		FD_CLR(mud_sockfd, &except_set);
		abort();
	    }
	    if (FD_ISSET(net_sockfd, &input_set))
		recv_net_message();
	    if (FD_ISSET(mud_sockfd, &input_set))
		recv_mud_message();
	}
    }
}

void net_keep_alive(void)  
{
    struct dns_record *dummy = 0, *temp = 0;
    struct sockaddr_in remote_address;
    char buffer[160];
    char *Services_Buffer = 0;
  
    for (dummy = dns_table; dummy; dummy = dummy->next)  {
    
	if (strcmp(dummy->mud_name, MUDNAME) == 0)
	    continue;
	else if (dummy->time_to_live == 0)
	    REMOVE_FROM_LIST(dummy, dns_table, next)
		else  {
		    dummy->time_to_live--;  
		    bzero((char *) &remote_address, sizeof(remote_address));
		    remote_address.sin_family = AF_INET;
		    remote_address.sin_addr.s_addr = dummy->ip_address;
		    remote_address.sin_port = dummy->port;
      
		    Services_Buffer = encode_services(muddns);
      
		    sprintf(buffer, "1020|%s|%s|%s|%d|%s|%s|%s|", INTERMUD_PORT, MUDNAME, MUDPORT, 
			    MUD_TYPE, MUD_VERSION, INTERMUD_VERSION,
			    Services_Buffer);
  
		    DEBUG(("SNET: [ PINGREQ ] %s - %s\n", buffer, inet_ntoa(remote_address.sin_addr)));
      
		    if (sendto(net_sockfd, buffer, strlen(buffer), 0, 
			       (struct sockaddr *) &remote_address, 
			       sizeof(remote_address)) != strlen(buffer))
			fprintf(stderr, "bugs in sending to server\n");  
		}
    }
}

void recv_net_message(void)
{
    struct sockaddr_in remote_address;
    int                msgnum, numbytes, addr_len;
    char               msgnum_t[5];
    addr_len = sizeof(struct sockaddr_in);
  
    numbytes = recvfrom(net_sockfd, message, MAX_NET_MESSAGE, 0, 
			(struct sockaddr *) &remote_address, &addr_len);
  
    message[numbytes] = '\0';
  
    if (numbytes < 0)
	interlog("ERRO: ZERO bytes received on net socket\n");
    else {
	strncpy(msgnum_t, message, 4);
	msgnum_t[4] = '\0';
	msgnum = atoi(msgnum_t);
    
	switch(msgnum) {
	case 1020: net_recv_pingreq(message, remote_address); break;
	case 1030: net_recv_pingrpy(message, remote_address); break;
	case 1050: net_recv_mudlist(message); break;
	case 1070: net_recv_dnsupdate(message, remote_address); break;
	case 2000:
	case 2010: net_recv_intertel(message, remote_address); break;
	case 2020:
	case 2030: net_recv_interwho(message); break;
	case 2040: net_recv_interpage(message, remote_address); break;
	case 3000: net_recv_interwiz(message, remote_address); break;
	default:   DEBUG(("RNET: [UNKNOWN  ] %s - %s\n", message, 
			  inet_ntoa(remote_address.sin_addr)));
	interlog("INFO: Invalid message type received (%d) from (%s)\n", 
		 msgnum, inet_ntoa(remote_address.sin_addr));
	}
    }
}


void net_recv_interpage(char *net_message, struct sockaddr_in remote_address)
{
    int numbytes;
  
    DEBUG(("RNET: [INTERPAGE] %s\n", net_message));
  
    if (validate_net_message(remote_address, net_message) == 0)
	return;
  
    numbytes = mud_send_data(mud_sockfd, net_message);  

    DEBUG(("SMUD: [INTERPAGE] %s\n", net_message));
  
    if (numbytes <= 0)  {
	interlog("ERRO: mud_send_data returned 0 bytes in net_recv_interpage, exiting\n");
	exit(0);
    }
}


void net_recv_interwiz(char *net_message, struct sockaddr_in remote_address)
{
    int numbytes;
  
    DEBUG(("RNET: [INTERWIZ ] %s\n", net_message));
  
    if (validate_net_message(remote_address, net_message) == 0)
	return;
  
    numbytes = mud_send_data(mud_sockfd, net_message);  

    DEBUG(("SMUD: [INTERWIZ ] %s\n", net_message));
  
    if (numbytes <= 0)  {
	interlog("ERRO: mud_send_data returned 0 bytes in net_recv_interwiz, exiting\n");
	exit(0);
    }
}

void net_recv_intertel(char *net_message, struct sockaddr_in remote_address)
{
    int numbytes;
  
    DEBUG(("RNET: [INTERTELL] %s\n", net_message));
  
    if (validate_net_message(remote_address, net_message) == 0)
	return;
  
    numbytes = mud_send_data(mud_sockfd, net_message);
  
    DEBUG(("SMUD: [INTERTELL] %s\n", net_message));
  
    if (numbytes <= 0)  {
	interlog("ERRO: mud_send_data returned 0 bytes in net_recv_intertel, exiting\n");
	exit(0);
    }
}

void net_recv_interwho(char *net_message)
{
    int numbytes;
  
    DEBUG(("RNET: [INTERWHO ] %s\n", net_message));
  
    numbytes = mud_send_data(mud_sockfd, net_message);
  
    DEBUG(("SMUD: [INTERWHO ] %s\n", net_message));
  
    if (numbytes <= 0)  {
	interlog("ERRO: mud_send_data returned 0 bytes in net_recv_interwho, exiting\n");
	exit(0);
    }
}

void net_recv_mudlist(char *net_message)
{
    struct dns_record  *dummy = 0;     
    struct sockaddr_in  remote_address;
    char buffer[100];
    char *services_buffer = 0;
   
  
    purge_dns_table();
  
    DEBUG(("RNET: [MUDLIST  ] %s\n", net_message));
  
    interlog("INFO: Received MUDLIST = %s\r\n", net_message);
  
    Service = strtok(net_message, "|");
   
    Remote_Mud = strtok(NULL, "|");
  
    while (Remote_Mud != NULL)  {
	if (str_cmp(Remote_Mud, "FIRST") == 0)
	    break;
    
	Mud_Port = strtok(NULL, "|");
	Remote_IP_addr = strtok(NULL, "|");
	Intermud_Port = strtok(NULL, "|");
	Mud_Type = strtok(NULL, "|");
	Mud_Version = strtok(NULL, "|");
	Intermud_Version = strtok(NULL, "|");
	Services = strtok(NULL, "|");
    
	if (Mud_Port != NULL && Remote_IP_addr != NULL && Intermud_Port != NULL)  {
	    CREATE(dummy, struct dns_record, 1);
  
	    dummy->time_to_live = 3;
	    dummy->ip_address = inet_addr(Remote_IP_addr);
	    dummy->port = htons(atoi(Intermud_Port));
	    strcpy(dummy->mud_name, Remote_Mud);
	    strcpy(dummy->mud_port, Mud_Port);    
	    strcpy(dummy->mud_version, Mud_Version);
	    strcpy(dummy->intermud_version, Intermud_Version);
	    dummy->mud_type = atoi(Mud_Type);
      
	    dummy->services = 0;
      
	    decode_services(Services, dummy);
      
	    dummy->next = dns_table;
	    dns_table = dummy;
      
	    bzero((char *) &remote_address, sizeof(remote_address));
	    remote_address.sin_family = AF_INET;
	    remote_address.sin_addr.s_addr = dummy->ip_address;
	    remote_address.sin_port = dummy->port;
      
	    services_buffer = encode_services(muddns);
      
	    sprintf(buffer, "1070|%s|%s|%s|%s|", MUDNAME, MUDPORT, INTERMUD_PORT, services_buffer); 
  
	    if (str_cmp(Remote_IP_addr, BOOTMASTER_IP) != 0) {
		DEBUG(("SNET: [DNSUPDATE] %s - %s\n", buffer, inet_ntoa(remote_address.sin_addr)));
		if (sendto(net_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &remote_address, 
			   sizeof(remote_address)) != strlen(buffer))
		    fprintf(stderr, "bugs in sending to server\n");
	    }
	}
	Remote_Mud = strtok(NULL, "|");
    }
}

void net_recv_pingrpy(char *net_message, struct sockaddr_in remote_address)
{
    struct dns_record *dummy = 0, *dns = 0;
    int found = 0;
  
    DEBUG(("RNET: [ PINGRPY ] %s\n", net_message));
  
    Service = strtok(net_message, "|");
    Remote_Mud = strtok(NULL, "|");
    Mud_Port = strtok(NULL, "|");
    Intermud_Port = strtok(NULL, "|");
    Mud_Type = strtok(NULL, "|");
    Mud_Version = strtok(NULL, "|");
    Intermud_Version = strtok(NULL, "|");
    Services = strtok(NULL, "|");
  
    if (Service != NULL && Intermud_Port != NULL && Remote_Mud != NULL && Mud_Port != NULL)  {
	for (dns = dns_table; dns; dns = dns->next)
	    if (dns->ip_address == remote_address.sin_addr.s_addr)  {
		if (dns->time_to_live < 3)
		    dns->time_to_live++;
		found = 1;
		break;
	    }
    
	if (found != 1)  {
	    CREATE(dummy, struct dns_record, 1);
      
	    dummy->time_to_live = 3;
	    dummy->ip_address = remote_address.sin_addr.s_addr;
	    dummy->port = htons(atoi(Intermud_Port));
	    strcpy(dummy->mud_name, Remote_Mud);
	    strcpy(dummy->mud_port, Mud_Port);
	    strcpy(dummy->mud_version, Mud_Version);
	    strcpy(dummy->intermud_version, Intermud_Version);
	    dummy->mud_type = atoi(Mud_Type);
	    dummy->services = 0;
      
	    decode_services(Services, dummy);
       
	    dummy->next = dns_table;
	    dns_table = dummy;
    
	    interlog("INFO: Added DNS record for %s\r\n", inet_ntoa(remote_address.sin_addr));
	}
    }
    else  {
	interlog("INFO: Received ambiguous PINGRPY from %s\r\n", inet_ntoa(remote_address.sin_addr));
    }
}

void net_recv_pingreq(char *net_message, struct sockaddr_in remote_address)
{
    struct dns_record *dummy = 0, *dns = 0;
    char buffer[160];
    int found = 0;
    char *Services_Buffer = 0;
    char mud_buffer[200];
  
    DEBUG(("RNET: [ PINGREQ ] %s - %s\n", net_message, inet_ntoa(remote_address.sin_addr)));
   
    Service = strtok(net_message, "|");
    Intermud_Port = strtok(NULL, "|");
    Remote_Mud = strtok(NULL, "|");
    Mud_Port = strtok(NULL, "|");
    Mud_Type = strtok(NULL, "|");
    Mud_Version = strtok(NULL, "|");
    Intermud_Version = strtok(NULL, "|");
    Services = strtok(NULL, "|");
  
    if (Service != NULL && Intermud_Port != NULL && Remote_Mud != NULL && Mud_Port != NULL)  {
	for (dns = dns_table; dns; dns = dns->next)  {
	    if (dns->ip_address == remote_address.sin_addr.s_addr)  {
		if (dns->time_to_live < 3)
		    dns->time_to_live++;
		found = 1;
		break;
	    }
	}
    
	if (found != 1)  {
	    CREATE(dummy, struct dns_record, 1);
      
	    dummy->time_to_live = 3;
	    dummy->ip_address = remote_address.sin_addr.s_addr;
	    dummy->port = htons(atoi(Intermud_Port));
	    strcpy(dummy->mud_name, Remote_Mud);
	    strcpy(dummy->mud_port, Mud_Port);
	    strcpy(dummy->mud_version, Mud_Version);
	    strcpy(dummy->intermud_version, Intermud_Version);
	    dummy->mud_type = atoi(Mud_Type);

	    dummy->services = 0;
      
	    decode_services(Services, dummy); 
      
	    dummy->next = dns_table;
	    dns_table = dummy;
    
	    interlog("INFO: Added DNS record for %s\r\n", inet_ntoa(remote_address.sin_addr));

	    sprintf(mud_buffer, "1100|%s has just connected to the intermud network.||", Remote_Mud);
      
	    DEBUG(("SMUD: %s\r\n", mud_buffer));
      
	    mud_send_data(mud_sockfd, mud_buffer);    
    
	}
    
	Services_Buffer = encode_services(muddns);
     
	sprintf(buffer, "1030|%s|%s|%s|%d|%s|%s|%s|", MUDNAME, MUDPORT, INTERMUD_PORT, 
		MUD_TYPE, MUD_VERSION, INTERMUD_VERSION,
		Services_Buffer); 
  
	if (sendto(net_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &remote_address, 
		   sizeof(remote_address)) != strlen(buffer))
	    fprintf(stderr, "bugs in sending to server\n");
  
	DEBUG(("SNET: [ PINGRPY ] %s - %s\n", buffer, inet_ntoa(remote_address.sin_addr)));
    }
}

void net_recv_dnsupdate(char *net_message, struct sockaddr_in remote_address)
{
    struct dns_record *dummy = 0, *dns = 0;
    char   mud_buffer[200];
    int    found = 0;
                 
    DEBUG(("RNET: [DNSUPDATE] %s\n", net_message));
  
    Service = strtok(net_message, "|");
    Remote_Mud = strtok(NULL, "|");
    Mud_Port = strtok(NULL, "|");
    Intermud_Port = strtok(NULL, "|");
    Mud_Version = strtok(NULL, "|");
    Intermud_Version = strtok(NULL, "|");
    Mud_Type = strtok(NULL, "|");
    Services = strtok(NULL, "|");
     
    if (Service != NULL && Intermud_Port != NULL && Remote_Mud != NULL && 
	Mud_Port != NULL && Mud_Version != NULL && Intermud_Version != NULL &&
	Mud_Type != NULL && Services != NULL)  {
	for (dns = dns_table; dns; dns = dns->next)
	    if (dns->ip_address == remote_address.sin_addr.s_addr)  {
		found = 1;
		break;
	    }
    
	if (found != 1)  {
	    CREATE(dummy, struct dns_record, 1);
    
	    dummy->time_to_live = 3;
	    dummy->ip_address = remote_address.sin_addr.s_addr;
	    dummy->port = htons(atoi(Intermud_Port));
	    strcpy(dummy->mud_name, Remote_Mud);
	    strcpy(dummy->mud_port, Mud_Port);
	    strcpy(dummy->mud_version, Mud_Version);
	    strcpy(dummy->intermud_version, Intermud_Version);
	    dummy->mud_type = atoi(Mud_Type);
	    dummy->services = 0;
       
	    decode_services(Services, dummy);
      
	    dummy->next = dns_table;
	    dns_table = dummy;
    
	    interlog("INFO: Added DNS record for %s\r\n", inet_ntoa(remote_address.sin_addr));
      
	    sprintf(mud_buffer, "1100|%s has just connected to the intermud network.||", Remote_Mud);
      
	    DEBUG(("SMUD: %s\r\n", mud_buffer));
      
	    mud_send_data(mud_sockfd, mud_buffer);    
	}
    }
    else  {
	interlog("INFO: Received ambiguous DNSUPDATE from %s\r\n", 
		 inet_ntoa(remote_address.sin_addr));
    }
}


void recv_mud_message(void)
{
    int    numbytes;
    char   msgnum_t[5];
    int    msgnum;
  
    numbytes = mud_recv_data(mud_sockfd, message);
  
    if (numbytes <= 0)  {
	purge_dns_table();
	interlog("ERRO: 0 bytes received on mud socket, mud's prolly disappeared.\n");
	close(mud_sockfd);
	mud_connect();
    } 
    else
	message[numbytes] = '\0';
  
    strncpy(msgnum_t, message, 4);
    msgnum_t[4] = '\0';
    msgnum = atoi(msgnum_t);
  
    switch(msgnum) {
    case 1040: mud_recv_mudlist_req(message); break;  
    case 2000:
    case 2010: mud_recv_intertell(message); break;
    case 2020:
    case 2030: mud_recv_interwho(message); break;
    case 2040: mud_recv_interpage(message); break;
    case 2050: mud_recv_mudinfo(message); break;
    case 3000: mud_recv_interwiz(message); break;
    case 4000: mud_recv_dnspurge(message); break;
    case 4020: mud_recv_reget(message); break;
    case 4040: mud_recv_stats(message); break;
    case 4060: mud_recv_mute(message); break;
    case 4070: mud_recv_debug(message); break;
    default  : interlog("INFO: Received unknown message type (%d) from the mud\n", msgnum);
    }
}


void mud_recv_mudlist_req(char *message)
{
    struct dns_record *dummy = 0;
    struct in_addr     address;
    char               mudlist_buffer[8000];
    char               buf[80], muted[2];
    int                numbytes;  
  
    DEBUG(("RMUD: [MUDLISTRQ] %s\n", message));
  
    Service = strtok(message, "|");
    To = strtok(NULL, "|");
    
    sprintf(mudlist_buffer, "1050|%s|", To);
   
    for (dummy = dns_table; dummy; dummy = dummy->next)  {
	address.s_addr = dummy->ip_address;
	if (dummy->muted == 1)
	    strcpy(muted, "*");
	else
	    strcpy(muted, " ");
	sprintf(buf, "%s|%s|%s|%s|", dummy->mud_name, dummy->mud_port, inet_ntoa(address), muted);
	strcat(mudlist_buffer, buf);
    }
    
    numbytes = mud_send_data(mud_sockfd, mudlist_buffer);
  
    if (numbytes <= 0)  {
	interlog("ERRO: mud_send_data returned 0 bytes in net_recv_intertel, exiting\n");
	exit(0);
    }
  
    DEBUG(("SMUD: [MUDLISTRP] %s\n", mudlist_buffer));
    return;
}


void mud_recv_intertell(char *message)
{
    struct dns_record *dummy = 0;
    struct sockaddr_in remote_address;
  
    bzero((char *) &remote_address, sizeof(remote_address));
    remote_address.sin_family = AF_INET;
  
    DEBUG(("RMUD: [INTERTELL] %s\n", message));
  
    strcpy(message2, message);
	
    Service    = strtok(message, "|");
    To         = strtok(NULL, "|");
    Remote_Mud = strtok(NULL, "|");
    From       = strtok(NULL, "|");
    Our_Mud    = strtok(NULL, "|");
    Text       = strtok(NULL, "|");
    
    for (dummy = dns_table; dummy; dummy = dummy->next)  {
	if (str_cmp(Remote_Mud, dummy->mud_name) == 0)  {
	    remote_address.sin_addr.s_addr = dummy->ip_address;
	    remote_address.sin_port = dummy->port;
	    DEBUG(("SNET: [INTERTELL] %s\n", message2));
	    sendto(net_sockfd, message2, strlen(message2), 0, 
		   (struct sockaddr *) &remote_address, sizeof(remote_address));
	    update_statistics(dummy, 0, strlen(message2));
	    break;
	}
    }
}

  
void mud_recv_interwho(char *message)
{
    struct dns_record *dummy = 0;
    struct sockaddr_in remote_address;
  
    bzero((char *) &remote_address, sizeof(remote_address));
    remote_address.sin_family = AF_INET;
  
    DEBUG(("RMUD: [INTERWHO ] %s\n", message));
  
    strcpy(message2, message);
  
    Service = strtok(message, "|");
    Remote_Mud = strtok(NULL, "|");
    
    for (dummy = dns_table; dummy; dummy = dummy->next)
	if (str_cmp(Remote_Mud, dummy->mud_name) == 0)  {
	    remote_address.sin_addr.s_addr = dummy->ip_address;
	    remote_address.sin_port = dummy->port;
	    DEBUG(("SNET: [INTERWHO ] %s\n", message2));
	    sendto(net_sockfd, message2, strlen(message2), 0, 
		   (struct sockaddr *) &remote_address, sizeof(remote_address));
	    update_statistics(dummy, 0, strlen(message2));
	    break;
	}
}
  
  
void mud_recv_interpage(char *message)
{
    struct dns_record *dummy = 0;
    struct sockaddr_in remote_address;
  
    bzero((char *) &remote_address, sizeof(remote_address));
    remote_address.sin_family = AF_INET;
  
    DEBUG(("RMUD: [INTERPAGE] %s\n", message));
       
    strcpy(message2, message);

    Service = strtok(message, "|");
    To      = strtok(NULL, "|");
    Remote_Mud = strtok(NULL, "|");
    
    for (dummy = dns_table; dummy; dummy = dummy->next)
	if (str_cmp(Remote_Mud, dummy->mud_name) == 0)  {
	    remote_address.sin_addr.s_addr = dummy->ip_address;
	    remote_address.sin_port = dummy->port;
	    DEBUG(("SNET: [INTERPAGE] %s\n", message2)); 
	    sendto(net_sockfd, message2, strlen(message2), 0, 
		   (struct sockaddr *) &remote_address, sizeof(remote_address));
	    update_statistics(dummy, 0, strlen(message2));
	}
}

  
void mud_recv_interwiz(char *message)
{
    struct dns_record *dummy = 0;
    struct sockaddr_in remote_address;
  
    bzero((char *) &remote_address, sizeof(remote_address));
    remote_address.sin_family = AF_INET;

    DEBUG(("RMUD: [INTERWIZ ] %s\n", message));
  
    strcpy(message2, message);
  
    for (dummy = dns_table; dummy; dummy = dummy->next)  {
	if (dummy != muddns) {
	    remote_address.sin_addr.s_addr = dummy->ip_address;
	    remote_address.sin_port = dummy->port;
	    DEBUG(("SNET: [INTERWIZ ] %s\n", message2));
	    sendto(net_sockfd, message, strlen(message), 0, (struct sockaddr *) &remote_address, 
		   sizeof(remote_address));
	    update_statistics(dummy, 0, strlen(message2));
	}
    }
}
  
  
void mud_recv_mudinfo(char *message)
{
    struct dns_record *dummy = 0;
    char mudlist_buffer[8000];
    char mud_services[10];
  
    DEBUG(("RMUD: [MUDINFO  ] %s\n", message));
  
    Service = strtok(message, "|");
    Remote_Mud = strtok(NULL, "|");
    From = strtok(NULL, "|");
    
    for (dummy = dns_table; dummy; dummy = dummy->next) {
	if (str_cmp(Remote_Mud, dummy->mud_name) == 0) {
	    if (IS_SET(dummy->services, ST_INTERWIZ))
		strcpy(mud_services, "1");
	    else
		strcpy(mud_services, "0");
	    if (IS_SET(dummy->services, ST_INTERTELL))
		strcat(mud_services, "1");
	    else
		strcat(mud_services, "0");
	    if (IS_SET(dummy->services, ST_INTERPAGE))
		strcat(mud_services, "1");
	    else
		strcat(mud_services, "0");
	    if (IS_SET(dummy->services, ST_INTERWHO))
		strcat(mud_services, "1");
	    else
		strcat(mud_services, "0");
	    if (IS_SET(dummy->services, ST_INTERBOARD))
		strcat(mud_services, "1");
	    else
		strcat(mud_services, "0");
	
	    sprintf(mudlist_buffer, "2055|%s|%s|%d|%s|%s|%s|",From, Remote_Mud, dummy->mud_type,
		    dummy->mud_version, dummy->intermud_version,
		    mud_services);
	    DEBUG(("SMUD: [MUDINFO  ] %s\n", mudlist_buffer));
	    mud_send_data(mud_sockfd, mudlist_buffer);
	    break;
	}
    }
}

  
void mud_recv_dnspurge(char *message)
{
  
    DEBUG(("RMUD: [DNSPURGE ] %s\n", message));
  
    Service = strtok(message, "|");
    From    = strtok(NULL, "|");
    
    purge_dns_table();
    
    DEBUG(("INFO: %s requested DNS table purge.\r\n", From));
}
  

void mud_recv_reget(char *message)
{
    struct sockaddr_in remote_address;
  
    bzero((char *) &remote_address, sizeof(remote_address));
    remote_address.sin_family = AF_INET;

    DEBUG(("RMUD: [  REGET  ] %s\n", message));
  
    Service = strtok(message, "|");
    From    = strtok(NULL, "|");
    
    bzero((char *) &remote_address, sizeof(remote_address));
    remote_address.sin_family = AF_INET;
    remote_address.sin_addr.s_addr = inet_addr(BOOTMASTER_IP);
    remote_address.sin_port = htons(BOOTMASTER_PORT);

    
    sprintf(message2, "1045|%s|", MUDNAME);
                 
    DEBUG(("SNET: [  REGET  ] %s\n", message2));
    
    sendto(net_sockfd, message2, strlen(message2), 0,
	   (struct sockaddr *) &remote_address, sizeof(remote_address));
}


void mud_recv_stats(char *message)
{
    struct dns_record *dummy = 0;
    char               mudlist_buffer[2000], muted[2];
    char               buf[80];
    int                numbytes;
  
    DEBUG(("RMUD: [  STATS  ] %s\n", message));
  
    Service = strtok(message, "|");
    From = strtok(NULL, "|");
    
    sprintf(mudlist_buffer, "4050|%s|", From);
    
    for (dummy = dns_table; dummy; dummy = dummy->next)  {
	if (dummy->muted == 1)
	    strcpy(muted, "*");
	else
	    strcpy(muted, " ");

	sprintf(buf, "%s|%lu|%lu|%lu|%lu|%d|%s|", dummy->mud_name, dummy->kb_in, dummy->kb_out,
		dummy->msg_in, dummy->msg_out,
		dummy->time_to_live, muted);
	strcat(mudlist_buffer, buf);
    }
    
    DEBUG(("SMUD: [  STATS  ] %s\n", mudlist_buffer));
	
    numbytes = mud_send_data(mud_sockfd, mudlist_buffer);
}
  

void mud_recv_mute(char *message)
{
    struct dns_record *dummy = 0;
  
    DEBUG(("RMUD  [  MUTE   ] %s\n", message));
  
    Service = strtok(message, "|");
    Remote_Mud = strtok(NULL, "|");
    
    for (dummy = dns_table; dummy; dummy = dummy->next)  {
	if (str_cmp(dummy->mud_name, Remote_Mud) == 0)  {
	    if (dummy->muted == 1) {
		dummy->muted = 0;
		interlog("INFO: %s has been unmutted\n", Remote_Mud);
	    }
	    else {
		dummy->muted = 1;
		interlog("INFO: %s has been mutted\n", Remote_Mud);
	    }
	    break;
	}
    }
}
  
  
void mud_recv_debug(char *message)
{

    DEBUG(("RMUD: [  DEBUG  ] %s\n", message));
  
    Service = strtok(message, "|");
    From = strtok(NULL, "|");
    
    if (debug == 0) {
	debug = 1;
	interlog("INFO: %s has turned debugging on\n", From);
    }
    else {
	debug = 0;
	interlog("INFO: %s has turned debugging off\n", From);
    }
}
  
  
void purge_dns_table(void)
{
    struct dns_record *dummy = 0, *temp = 0;
  
    for (dummy = dns_table; dummy; dummy = dummy->next)
	if (str_cmp(dummy->mud_name, MUDNAME) != 0)
	    REMOVE_FROM_LIST(dummy, dns_table, next);
}


int validate_net_message(struct sockaddr_in remote_address, char *net_message)
{
    struct dns_record *dummy = 0;
  
    dummy = check_dns_record(remote_address);
  
    if (dummy != NULL && dummy->muted != 1)  {
	update_statistics(dummy, 1, strlen(net_message));
	return(1);
    }
    else
	return(0);
}

struct dns_record *check_dns_record(struct sockaddr_in remote_address)
{
    int                found = 0;
    char               buffer[50];
    struct dns_record *dns_temp = 0, *dummy = 0;
    char *Services_Buffer = 0;
  
    for (dns_temp = dns_table; dns_temp; dns_temp = dns_temp->next)
	if (dns_temp->ip_address == remote_address.sin_addr.s_addr) {
	    dummy = dns_temp;
	    found = 1;
	    break;
	}
  
    if (found == 0)  {
    
	Services_Buffer = encode_services(muddns);
    
	sprintf(buffer, "1020|%s|%s|%s|%s|", INTERMUD_PORT, MUDNAME, MUDPORT, Services_Buffer);
    
	if (sendto(net_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &remote_address, sizeof(remote_address)) != strlen(buffer))
	    fprintf(stderr, "bugs in sending to server\n");
  
	DEBUG(("SNET: [PINGREQ] %s - %s\n", buffer, inet_ntoa(remote_address.sin_addr)));
	return(NULL);
    }
    else
	return(dummy);  
}
  
void update_statistics(struct dns_record *remote_mud, int direction, int msg_size)
{
    if (remote_mud != NULL)  {
	if (direction == 0)  {
	    remote_mud->kb_out += msg_size;
	    remote_mud->msg_out++;
	}
	if (direction == 1)  {
	    remote_mud->kb_in += msg_size;
	    remote_mud->msg_in++;
	    if (remote_mud->
		time_to_live < 3)
		remote_mud->time_to_live++;
	}
    }
}

void nonblock(int sockfd)
{
    int flags;
  
    flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) < 0)  {
	interlog("ERRO: Fatal error in setting sockets to nonblocking");
	exit(1);
    }
}

void init_net_socket(int port)
{
    struct sockaddr_in  serv_addr;
  
    /* Open our Internet UDP socket */
  
    if ((net_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)  {
	fprintf(stderr, "ERRO: Can't open internet datagram socket.\n");
	exit(1);
    }
  
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(port);
  
    if (bind(net_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  {
	fprintf(stderr, "ERRO: Can't bind local address, port = %d\n", port);
	exit(1);
    }
}

void mud_connect(void)
{
  
    if ((mud_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)  {
	fprintf(stderr, "ERRO: Can't open unix stream socket.\n");
	exit(1);
    }
  
    unlink(MUDSOCK_PATH);
    bzero((char *) &mud_addr, sizeof(mud_addr));
    mud_addr.sun_family      = AF_UNIX;
    strcpy(mud_addr.sun_path, MUDSOCK_PATH);
    mudaddrlen = sizeof(mud_addr.sun_family) + strlen(mud_addr.sun_path);
  
    if (bind(mud_sockfd, (struct sockaddr *) &mud_addr, mudaddrlen) < 0)  {
	fprintf(stderr, "ERRO: Can't bind local mud socket.  Check MUDSOCK_PATH in intermud.h\n");
	exit(1);
    }
  
    listen(mud_sockfd, 1);

    mud_sockfd = accept(mud_sockfd, NULL, NULL);

    DEBUG(("INFO: Accepted connection from mud\n"));
  
    net_send_startup();

    server_loop();
}

void interlog(char *fmt, ...)
{
    va_list args;
    time_t ct;
    struct tm *cct = 0;
    char timebuf[30];
    char buffer[8000];
    char timebit[5];
  
    va_start(args, fmt);
    ct = time(0);
    cct = localtime(&ct);
  
    if (cct->tm_hour < 10)
	sprintf(timebit, "0%d:", cct->tm_hour);
    else
	sprintf(timebit, "%d:", cct->tm_hour);
  
    strcpy(timebuf, timebit);
  
    if (cct->tm_min < 10) 
	sprintf(timebit, "0%d:", cct->tm_min);
    else
	sprintf(timebit, "%d:", cct->tm_min);
  
    strcat(timebuf, timebit);
  
    if (cct->tm_sec < 10)
	sprintf(timebit, "0%d ", cct->tm_sec);
    else
	sprintf(timebit, "%d ", cct->tm_sec);
  
    strcat(timebuf, timebit);
  
    timebuf[strlen(timebuf)] = '\0';
  
    vsprintf(buffer, fmt, args);
  
    fprintf(stderr, "%s%s", timebuf, buffer);
    va_end(args);
}

int mud_recv_data(int fd, char *buf)
{
    int buflen;
    int cc;
  
    cc = recv(fd, &bullet, sizeof(struct bullet), 0);
    if (cc <= 0)
	return(cc);
    else {
	buflen = bullet.bytes;
	while (buflen > 0)  {
	    cc = recv(fd, buf, buflen, 0);
	    if (cc <= 0)  {
		interlog("ERRO: mud_rec_data, most probably fatal\n");
	    }
	    buf += cc;
	    buflen -= cc;
	}
	return(bullet.bytes);
    }
}

int mud_send_data(int fd, char *buf)
{
    int buflen;
    int cc;
  
    if (strlen(buf) <= 0)  {
	interlog("ERRO: mud_send_data requested to send 0 bytes, exiting\n");
	exit(0);
    }
  
    bullet.bytes = strlen(buf);
  
    cc = send(fd, &bullet, sizeof(struct bullet), 0);
    if (cc <= 0)
	return(cc);
    else  {
	buflen = bullet.bytes; 
	while (buflen > 0)  {
	    cc = send(fd, buf, buflen, 0);
	    if (cc <= 0)  {
		interlog("ERRO: mud_send_data, most probably fatal\n");
	    }
	    buf += cc;
	    buflen -= cc;
	}
	return(bullet.bytes);
    }
}

int str_cmp(char *arg1, char *arg2)
{
    int chk, i;

    for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
	if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
	    if (chk < 0)
		return (-1);
	    else
		return (1);
    return (0);
}

void decode_services(char *bitmap, struct dns_record *remote_mud)
{
    int i = 0, j = 0;
    char tmpbuf[32];
    char *bufptr = 0, *bytes = 0;
  
    strcpy(tmpbuf, "00000000000000000000000000000000");
  
    bufptr = tmpbuf;
  
    while (j <= 7) {
	switch (*(bitmap++)) {
	case 'F' : bytes = "1111"; break;
	case 'E' : bytes = "1110"; break;
	case 'D' : bytes = "1101"; break;
	case 'C' : bytes = "1100"; break;
	case 'B' : bytes = "1011"; break;
	case 'A' : bytes = "1010"; break;
	case '9' : bytes = "1001"; break;
	case '8' : bytes = "1000"; break;
	case '7' : bytes = "0111"; break;
	case '6' : bytes = "0110"; break;
	case '5' : bytes = "0101"; break;
	case '4' : bytes = "0100"; break;
	case '3' : bytes = "0011"; break;
	case '2' : bytes = "0010"; break;
	case '1' : bytes = "0001"; break;
	case '0' : bytes = "0000"; break;
	default  : DEBUG(("ERRO: Invalid bitmap from %s\n", remote_mud->mud_name)); 
	    abort();
	    break;
	}
	for (i = 0; i <= 3; i++) {
	    *bufptr = *bytes;
	    bytes++;
	    bufptr++;
	}
    
	j++;
    }
  
    bufptr = tmpbuf;
  
    for (i = 0; i <= 4; i++, bufptr++) {
	if (i == 0 && *bufptr == '1')
	    SET_BIT(remote_mud->services, ST_INTERWIZ);
	else if (i == 1 && *bufptr == '1')
	    SET_BIT(remote_mud->services, ST_INTERTELL);
	else if (i == 2 && *bufptr == '1')
	    SET_BIT(remote_mud->services, ST_INTERPAGE);
	else if (i == 3 && *bufptr == '1')
	    SET_BIT(remote_mud->services, ST_INTERWHO);
	else if (i == 4 && *bufptr == '1')
	    SET_BIT(remote_mud->services, ST_INTERBOARD);
    }
}


char *encode_services(struct dns_record *remote_mud)
{
    int   i;
    char  tmpbuf[32];
    char *bufptr = 0, *bitptr = 0;

    strcpy(tmpbuf, "00000000000000000000000000000000");
    strcpy(services, "00000000");
  
    bufptr = tmpbuf;
    bitptr = services;
  
    if (IS_SET(remote_mud->services, ST_INTERWIZ)) {
	*bufptr = '1';
	bufptr++;
    }
  
    if (IS_SET(remote_mud->services, ST_INTERTELL)) {
	*bufptr = '1';
	bufptr++;
    }

    if (IS_SET(remote_mud->services, ST_INTERPAGE)) {
	*bufptr = '1';
	bufptr++;
    }
  
    if (IS_SET(remote_mud->services, ST_INTERWHO)) {
	*bufptr = '1';
	bufptr++;
    }
  
    if (IS_SET(remote_mud->services, ST_INTERBOARD)) {
	*bufptr = '1';
	bufptr++;
    }

    bufptr = tmpbuf;
  
    for (i = 0;i <= 7;i++) {
	if (strncmp(bufptr, "1111", 4) == 0)
	    *(bitptr++) = 'F';
	else if (strncmp(bufptr, "1110", 4) == 0)
	    *(bitptr++) = 'E';
	else if (strncmp(bufptr, "1101", 4) == 0)
	    *(bitptr++) = 'D';
	else if (strncmp(bufptr, "1100", 4) == 0)
	    *(bitptr++) = 'C';
	else if (strncmp(bufptr, "1011", 4) == 0)
	    *(bitptr)++ = 'B';
	else if (strncmp(bufptr, "1010", 4) == 0)
	    *(bitptr++) = 'A';
	else if (strncmp(bufptr, "1001", 4) == 0)
	    *(bitptr++) = '9';
	else if (strncmp(bufptr, "1000", 4) == 0)
	    *(bitptr++) = '8';
	else if (strncmp(bufptr, "0111", 4) == 0)
	    *(bitptr++) = '7';
	else if (strncmp(bufptr, "0110", 4) == 0)
	    *(bitptr++) = '6';
	else if (strncmp(bufptr, "0101", 4) == 0)
	    *(bitptr++) = '5';
	else if (strncmp(bufptr, "0100", 4) == 0)
	    *(bitptr++) = '4';
	else if (strncmp(bufptr, "0011", 4) == 0)
	    *(bitptr++) = '3';
	else if (strncmp(bufptr, "0010", 4) == 0)
	    *(bitptr++) = '2';
	else if (strncmp(bufptr, "0001", 4) == 0)
	    *(bitptr++) = '1';
	else if (strncmp(bufptr, "0000", 4) == 0)
	    *(bitptr++) = '0';
	bufptr = bufptr + 4;
    }
    bitptr = services;
  
    return(bitptr);
}


char *calc_services(void)
{
    int i;
  
    if (SRV_INTERWIZ == 1)
	strcpy(services, "1");
    else
	strcpy(services, "0");
  
    if (SRV_INTERTELL == 1)
	strcat(services, "1");
    else
	strcat(services, "0");
  
    if (SRV_INTERPAGE == 1)
	strcat(services, "1");
    else
	strcat(services, "0");
  
    if (SRV_INTERWHO == 1)
	strcat(services, "1");
    else
	strcat(services, "0");
  
    if (SRV_INTERBOARD == 1)
	strcat(services, "1");
    else
	strcat(services, "0");
  
    for (i = 5; i <= 32; i++)
	strcat(services, "0");
  
    return(services);
}
