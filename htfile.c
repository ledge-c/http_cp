#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>

#include "htfile.h"
#include "http_str.h"

inline _Bool check_file(char *filename);
void prep_reply_text(char *str, size_t len);
int net_init();
int get_connection(int htsock);
void inline close_conn(FILE **fp, int *sd);
int handle_connection(FILE* fp, const char *reply_error);
unsigned int validate_port(const char *port_str);
void sigint_handler(int sig);
char *get_ip(struct sockaddr_in *addr);

s_options *opts;

/* used by signal handler */
static struct  {
    int *conn_ptr;   
    int *htsock_ptr;
    FILE **fp_ptr;
} conn_hdls;

int main(int argc, char **argv)
{
    s_options _opts;
    opts = &_opts; /* Set global pointer */ 

    memset(opts,'\0',sizeof(s_options));
    opts->fcheck = 1; // Check files are available before setting up socket
    
    int opt = getopt( argc, argv, "Cvc:p:" );
	while( opt != -1 )
	{
	    switch( opt ) {
                case 'v':
                        opts->verbose = 1;
                        break;
                case 'c':
                        opts->count = *optarg;
                        break;
                case 'C':
                        /* Don't check if files exist before listening */
                        opts->fcheck = 0;
                        break;
                case 'p':
                        opts->port = validate_port(optarg);
                        break;
                default:
                        die(USAGE_MSG, argv[0]);
	    };
	    opt = getopt( argc, argv, "Cvc:p:" );
	}
    opts->files = argv + optind;
    opts->files_num = argc - optind;
    
    // Validate arguments
    if (! opts->files[0] || strlen(*(opts->files))==0)
        die("No files to send.\n" USAGE_MSG, argv[0]);
        
    if (opts->files_num > 10)
        die("Too many files to offer. Maximum is 10 files, got %d\n", opts->files_num);
    
    int i;
    size_t a = 0;
    size_t file_args_length = 0; 

    for(i=0;i<opts->files_num;i++)
    {
        PRINTV("checking element %s\n",opts->files[i]);
        a = strlen(opts->files[i]);
        if ( a > FILENAME_MAXLEN)
            die("File name number %d is too long.\n", i);
                
        /* Check if file exists */
        if ((opts->fcheck) && (! check_file(opts->files[i])) )
            die(
                    "Failed accessing %s - %s\n", 
                    opts->files[i], 
                    strerror(errno)
            );
        file_args_length += a; // used later in http replies to calculate length
    };


    int htsock;
    conn_hdls.htsock_ptr = &htsock;

    if ((htsock = net_init()) == FAILURE)
        die("Failed initializing network connection\n");
        

    FILE *fp = NULL;
//    fp_ptr = &fp;
    conn_hdls.fp_ptr = &fp;
    int conn;
//    conn_ptr = &conn;
    conn_hdls.conn_ptr = &conn;

        
    /* Added to line breaks at the end of the reply, adjusted length accordingly */
    size_t reply_err_len = 254 + 140 + 2*file_args_length + 27*(opts->files_num); 
//    unsigned int reply_err_len = 254 + 138 + 2*file_args_length + 27*(opts->files_num); 
    char reply_error[ reply_err_len ];
    prep_reply_text(reply_error, reply_err_len);
        

    if (signal(SIGINT,sigint_handler) == SIG_ERR)
        die("Failed installing signal handler (%s)", strerror(errno));
        
    
    while((conn = get_connection(htsock)))
    {
		
        if ((fp = fdopen(conn, "w+")) == NULL)
        {
            /* Failed open a stream for this connection, close
             * connection and listen for another one
             */
            close_conn(NULL,&conn);
            continue;
        }

        if (FAILURE == handle_connection(fp, reply_error))
        {
            close_conn(&fp,&conn);
            continue;
        }
        else
        {
            /* Success */
            close_conn(&fp,&conn);
            break;
        }
                
    } //while()
    
    if (close(htsock))
        fprintf(stderr, "Failed closing socket - %s", strerror(errno));

    get_ip(NULL); /* Free IP address string memory */
    return(EXIT_SUCCESS); 
} // main()

void sigint_handler(int sig)
{
    int status;
    
    printf("Cleaning up...\n");
    close_conn(conn_hdls.fp_ptr, conn_hdls.conn_ptr);

    if (conn_hdls.htsock_ptr != NULL)
    {
        status = close(*(conn_hdls.htsock_ptr));
        PRINTV("Closing socket (returned code: %d)\n", status);
    }
        
    get_ip(NULL); /* Free IP address string memory */
    exit(EXIT_FAILURE);
}

void prep_reply_text(char *str, size_t len)
{
        char *tmp_ptr;
        int i;

        tmp_ptr = str + sprintf(str, HTTP_NOT_FOUND, len);
        for(i=0; i < opts->files_num; i++)
            tmp_ptr +=  sprintf(tmp_ptr, 
                                "<li><a href=\"/%s\">%s</a></li>", 
                                /* Skip first char if it's a slash */
                                opts->files[i] + (*(opts->files[i]) == '/'),
                                opts->files[i]
                        );

        sprintf(tmp_ptr,"\n</body>\n</html>\r\n");
}
