#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define SECONDS_TO_DISCONNECT	15

struct session_t {
	struct session_t *next;
	int s;			// descriptor
	int sent;		// set to 1 when str sent
	int connect_time;	// number of seconds to disconnect
};

int listener = 0;
struct session_t *sessions = NULL;
char *show_str = NULL;
size_t show_len = 0;

void
read_file(const char *path)
{
	struct stat stat_buf;
	FILE *inf;

	if (stat(path, &stat_buf) < 0) {
		perror("Couldn't stat() file");
		exit(-1);
	}

	show_str = (char *)malloc(stat_buf.st_size + 1);
	if (!show_str) {
		perror("Couldn't allocate show_str");
		exit(-1);
	}

	inf = fopen(path, "r");
	if (!inf) {
		perror("Couldn't open() file");
		exit(-1);
	}

	if (fread(show_str, 1, stat_buf.st_size, inf) < 0) {
		fclose(inf);
		perror("Error during fread()");
		free(show_str);
		exit(-1);
	}

	fclose(inf);

	show_len = stat_buf.st_size;
}

void
init_network(int port)
{
	struct sockaddr_in addr;
	int flags;

	// Open listener socket
	listener = socket(PF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		perror("Couldn't open listener");
		exit(-1);
	}

	flags = 1;
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char *)&flags, sizeof(flags)) < 0) {
		perror("setsockopt REUSEADDR");
		exit(-1);
	}
	// Bind to port
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listener, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind to port");
		exit(-1);
	}

	// Set nonblocking
	flags = fcntl(listener, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(listener, F_SETFL, flags) < 0) {
		perror("Couldn't set listener to nonblock");
		exit(-1);
	}

	listen(listener, 10);
}

void
main_network_loop(void)
{
	struct timeval wait_time;
	fd_set in_set, out_set, exc_set;
	int max;
	struct session_t *cur_s, *prev_s, *next_s;
	struct sockaddr_in addr;
	int len;
	int flags;
	long ip;


	FD_ZERO(&in_set);
	FD_ZERO(&out_set);
	FD_ZERO(&exc_set);
	FD_SET(listener, &in_set);

	max = listener;
	for (cur_s = sessions;cur_s;cur_s = cur_s->next) {
		FD_SET(cur_s->s, &in_set);
		FD_SET(cur_s->s, &out_set);
		FD_SET(cur_s->s, &exc_set);
		if (cur_s->s > max)
			max = cur_s->s;
	}

	wait_time.tv_sec = 1;
	wait_time.tv_usec = 0;
	select(max + 1, &in_set, &out_set, &exc_set, &wait_time);

	// Check for new connection
	if (FD_ISSET(listener, &in_set)) {
		cur_s = (struct session_t *)malloc(sizeof(struct session_t));
		cur_s->connect_time = time(NULL);
		cur_s->sent = 0;
		len = sizeof(struct sockaddr_in);
		cur_s->s = accept(listener, (struct sockaddr *)&addr, &len);
		if (cur_s->s < 0)
			free(cur_s);
		else {
			flags = fcntl(listener, F_GETFL, 0);
			flags |= O_NONBLOCK;
			if (fcntl(listener, F_SETFL, flags) < 0) {
				perror("Couldn't set listener to nonblock");
				exit(-1);
			}

			ip = ntohl(addr.sin_addr.s_addr);
			printf("Incoming connection from %lu.%lu.%lu.%lu\n",
				ip >> 24 & 0xFF, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);
			fflush(stdout);
			cur_s->next = sessions;
			sessions = cur_s;
		}
	}

	// Check for disconnections
	for (cur_s = sessions;cur_s;cur_s = next_s) {
		next_s = cur_s->next;
		if (FD_ISSET(cur_s->s, &exc_set) ||
				FD_ISSET(cur_s->s, &in_set) ||
				(time(0) - cur_s->connect_time > SECONDS_TO_DISCONNECT)) {
			if (cur_s != sessions) {
				prev_s = sessions;
				while (prev_s->next != cur_s)
					prev_s = prev_s->next;
				prev_s->next = cur_s->next;
			} else {
				sessions = cur_s->next;
			}
			shutdown(cur_s->s, 2);
			close(cur_s->s);
			free(cur_s);
		}
	}

	// Check for sent status
	for (cur_s = sessions;cur_s;cur_s = cur_s->next) {
		if (FD_ISSET(cur_s->s, &out_set) && !cur_s->sent) {
			if (send(cur_s->s, show_str, show_len, 0) >= 0)
				cur_s->sent = 1;
		}
	}
}

int
main(int argc, char *argv[])
{
	char *c;
	int port;

	if (argc != 3) {
		fprintf(stderr, "Usage: netnotify <port> <file>\n");
		exit(-1);
	}

	c = argv[1];
	while (*c && isdigit(*c))
		c++;
	if (*c) {
		fprintf(stderr, "Port argument must be a number\n");
		exit(-1);
	}

	port = atoi(argv[1]);
	if (port < 1024 && getuid() != 0) {
		fprintf(stderr, "Ports 1024 and below are reserved for root.\n");
		exit(-1);
	}

	read_file(argv[2]);

	init_network(port);

	while (1)
		main_network_loop();

	return 0;
}
