
/* Maximum size of incoming data on internet UDP port.
** Not a good idea to change this value right now.
** 
*/

#define MAX_NET_MESSAGE 1024

/* MUDSOCK_PATH is the absolute path to the Unix Datagram Socket that the
** intermud server and the mud use to communicate.  the mudsock file is 
** created on the fly by the intermud server.  Replace the /usr/pm/bin with
** the path to where you keep your binary for your mud.
** 
*/

#define MUDSOCK_PATH "/tmp/intermud_sock"

/* BOOTMASTER_IP is the IP number of the so called boot server.  After you
** start your mud, the intermud server will send a startup message to the
** boot master so your mud can be registered with the network mudlist.  This
** network mudlist will then be returned to your intermud server so you can
** communicate with other muds within the network.  Leave this set as is, or
** you will not be registered, or get a mudlist.
**
*/

#define BOOTMASTER_IP "-1"

/* BOOTMASTER_PORT is the UDP port number of the boot server
** 
*/

#define BOOTMASTER_PORT 2121


/* INTERMUD_PORT is the port that your intermud server will use to send and
** receive messages from intermud network.  Make sure that the port number
** defined here is the same port number that is used in the inter shell 
** script that is also provided with this package.
**
*/

#define INTERMUD_PORT "2121"

/* MUDNAME is the name of your mud.  Currently you MUST use ONE word for your
** mud name.  This will probably change sometime in the future.
**
*/

#define MUDNAME "Tempus"


/* MUDIPADDR is the IP Address of your mud.
*/

#define MUDIPADDR "129.59.116.84"


/* MUDPORT is the port that your mud uses.
*/

#define MUDPORT "2020"


/* INTERMUD_VERSION is the version of intermud you are running.  Don't change it.
*/

#define INTERMUD_VERSION "0.55"

/* MUDTYPE is defined as 1 for Circle, 2 for Merc, 3 for Envy and 4 for other.
*/

#define MUD_TYPE 1

/* MUD_VERSION is the version of your mud release
*/

#define MUD_VERSION "3.00bpl8+"


#define SRV_INTERWIZ     1

#define SRV_INTERTELL    1

#define SRV_INTERPAGE    1

#define SRV_INTERWHO     1

#define SRV_INTERBOARD   1
