#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include "htfile.h"
#include "http_str.h"

static inline int get_http_req_end(FILE *fp);
int send_file(FILE* fp, unsigned int f_index);
int chomp(char *input);
extern s_options *opts;
char *get_ip(struct sockaddr_in *addr);

int net_init()
{
    int htsock;
    int rc;
    struct sockaddr_in localAddr;
    memset(&localAddr, '\0', sizeof(localAddr));
 
    localAddr.sin_family = AF_INET;

    if (! opts->port)
    {
        printf("Using default port: %d\n", DEFAULT_PORT);
        opts->port = DEFAULT_PORT;
    }

    localAddr.sin_port = htons(opts->port);
    localAddr.sin_addr.s_addr = INADDR_ANY;
 
    if ((htsock = socket(PF_INET , SOCK_STREAM , 0)) == -1)
        RETERROR("socket() - %s\n", strerror(errno));
        
    if (NULL != opts->ifname) /* specific interface requested by user */
    {
        printf("Using network interface: %s\n", opts->ifname);
        rc = setsockopt(htsock, 
                SOL_SOCKET, 
                SO_BINDTODEVICE, 
                opts->ifname, 
                strlen(opts->ifname)
        );
        if (rc != 0)
            die("Failed setting requested interface up for listening - %s\n",
                strerror(errno)
            );
    }
    
    if (bind(htsock, (struct sockaddr *)&localAddr, sizeof(localAddr)) != 0 )
        RETERROR("bind() - %s\n", strerror(errno));

    if (listen(htsock,1) != 0 )
        RETERROR("listen() - %s\n", strerror(errno));

    return(htsock);
}

int get_connection(int htsock)
{
    int cd;
    struct sockaddr_in remoteAddr;
    unsigned int remoteAddr_size = sizeof(struct sockaddr_storage);
    
    while(1)
    {
        PRINTV("Waiting for a new connection on port %d\n", opts->port);
        cd = accept(htsock,(struct sockaddr *)&remoteAddr, &remoteAddr_size);
        if (cd == -1)
        {
            fprintf(stderr, "Failed accepting connection - %s.\n", strerror(errno));
            continue;
        }
        else
            break;
    }
    PRINTV("Got a connection from %s\n", get_ip(&remoteAddr));
    return(cd);
            
}

int handle_connection(FILE *fp, const char *reply_error)
{
    char s[1024] = { '\0' };
    char file_req_line[FILENAME_MAXLEN + 16] = { '\0' };
    int i;
    
    while (fgets(s, sizeof(s), fp) != 0  &&  strncmp(s, "bye\n", 4) != 0)
    {
        chomp(s);
        if ( strncmp(s, HTTP_REQ_START, HTTP_REQ_START_LEN) == 0)
        {
            PRINTV("Got http request start. Looking for files we offer.\n");
            for(i=0; i < opts->files_num; i++)
            {
                snprintf(
                            file_req_line, 
                            sizeof(file_req_line), 
                            "GET /%s HTTP/1.",
                            /* Skip first char if it's a slash */
                            opts->files[i] + (*(opts->files[i]) == '/')
//                          opts->files[i]
                );
                
                if ( strncmp(s, file_req_line, strlen(file_req_line)) == 0 )
                {
                    printf("Got request for file: %s\n", opts->files[i]);
                    if (SUCCESS == get_http_req_end(fp))
                    {
                        PRINTV("Request Valid. Sending file [%s]...\n", opts->files[i]);
                        if (send_file(fp, i) == SUCCESS)
                            return(SUCCESS);
                        else
                            RETERROR("Failed sending [%s]\n", opts->files[i]);
                    }
                    else
                    {
                        fprintf(stderr, "Something went wrong - "
                                "HTTP request did not end correctly. "
                                "Aborting this connection.\n"
                        );
                        return(FAILURE);
                    }
                }
            }
            
            PRINTV("Unknown file requested by client\n");
            if (SUCCESS == get_http_req_end(fp)) /* Scroll to end of request */
            {
                /* Send a list of the files we offer */
                fprintf(fp,"%s", reply_error);
                fflush(fp);
            }
            else
            {
                fprintf(stderr, "Something went wrong - "
                        "HTTP request did not end correctly. "
                        "Aborting this connection.\n"
                );
            }
            return(FAILURE);
        }
        /* First input line from client was not a valid HTTP request */
        PRINTV("HTTP request not understood. Aborting connection.\n");
        return(FAILURE);
    }
    return(FAILURE); /* Client disconnected */
}

void close_conn(FILE **fp, int *sd)
{
    if ((fp != NULL) && (*fp != NULL))
    {
        fclose(*fp);
        *fp = NULL;
    }
    
    if ((sd != NULL) && (*sd != 0))
    {
        close(*sd);
        *sd = 0;
    }
        
}

int send_file(FILE *fp, unsigned int f_index)
{
    struct stat file_stat;
    size_t file_size;
    
    char cbuf[BUFSIZ] = { '\0' };
    
    FILE *fstream;
    if (stat(opts->files[f_index], &file_stat) != 0)
        RETERROR("Could not stat() %s - %s\n",opts->files[f_index], strerror(errno));

    file_size = file_stat.st_size;

    if ((fstream = fopen(opts->files[f_index], "r")) == NULL)
        RETERROR("Could not open file [%s] for reading - %s\n", 
                opts->files[f_index], 
                strerror(errno)
        );
    else
    {
        PRINTV("Opened [%s] for reading.\n", opts->files[f_index]);
    }
    
    fprintf(fp, HTTP_OFFER_FILE, file_size);

    while (feof(fstream) == 0)
    {
        if (NULL != fgets(cbuf, BUFSIZ, fstream))
        {
            if (fputs(cbuf, fp) == EOF)
            {
                fclose(fstream);
                RETERROR("Failed writing to socket - %s\n", strerror(errno));
            }
        }
        else
        {
                fclose(fstream);
                RETERROR("Failed reading from file - %s\n", strerror(errno));
        }
    }

    
    fflush(fp);
    printf("File sent successfully.\n");
    fclose(fstream);
    
    return(SUCCESS);
}

static inline int get_http_req_end(FILE *fp)
{
    char s[1024];
    
    while (fgets(s, sizeof(s), fp) != NULL)
        if ((s[1] == '\n') && (s[0] == '\r'))
        {
            PRINTV("Found request EOF.\n");
            return(SUCCESS);
        }
    
    return(FAILURE);
}


char *get_ip(struct sockaddr_in *addr)
{
	static char *ip;
        if (ip == NULL)
        {
            if ((ip = malloc(IPADDR_LEN)) == NULL)
            {
                fprintf(stderr, 
                        "get_ip() - Memory allocation failed; %s\n",
                        strerror(errno)
                );
                return(NULL);
            }
        }
        
        if (addr == NULL)
        {
            free(ip);
            ip = NULL;
        }
        else
            inet_ntop(AF_INET, &addr->sin_addr.s_addr, ip, IPADDR_LEN);
        
	return(ip);
}

unsigned int validate_port(const char *port_str)
{
    /* Return port_str as a number if it is a valid port */
    long int port;
    char *endptr;
    size_t len = strlen(port_str);
    
    
    
    if ((port = strtol(port_str, &endptr, 10)) != 0)
        // converted successfully
        if ((endptr - port_str) == len) /* String contained only digits */
            if ((port > 0) && (port <= PORT_MAX))
                return((uint16_t)port);

    fprintf(stderr,"Invalid port number specified. Ignored.\n");
    return(0);
}

const char *validate_if(char *ifname)
{
    int dummy_sock;
    struct ifreq if_req;
    
    
    if (strlen(optarg) > IFNAMSIZ)
        die("Requested interface name is too long (max: %d chars)\n", IFNAMSIZ);
    
    /* Is the requested interface up? */
    if ((dummy_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Call to socket() failed - %s\n", strerror(errno));
        return(NULL);
    }
    memset(&if_req, 0, sizeof(if_req));
    strncpy(if_req.ifr_name, ifname, IFNAMSIZ);
    
    if (ioctl(dummy_sock, SIOCGIFFLAGS, &if_req) < 0)
        die("Failed accessing interface %s - %s\n", ifname, strerror(errno));
    
    if (!(if_req.ifr_flags & IFF_UP)) 
        die("Interface %s is not up\n", ifname);
    
    return(strndup(ifname, IFNAMSIZ));
}