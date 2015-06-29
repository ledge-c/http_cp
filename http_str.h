#ifndef HTTP_STR_H
#define	HTTP_STR_H

#ifdef	__cplusplus
extern "C" {
#endif

#define HTTP_REQ_START  "GET /"
#define HTTP_REQ_START_LEN  5
    
#define HTTP_NOT_FOUND  "HTTP/1.1 404 NOT FOUND\n"                            \
                        "Server: htfile\n"                                    \
                        "Accept-Ranges: bytes\n"                              \
                        "Content-Length: %ld\n\n"                              \
                        "<html><body><br>\n"                                  \
                        "<b>The file you were looking for is not offered on " \
                        "this server.<br>\n"                                  \
                        "Offered files are: \n"


#define HTTP_OFFER_FILE "HTTP/1.1 200 OK\n" \
                        "Server: htfile\n" \
                        "Accept-Ranges: bytes\n" \
                        "Content-Length: %ld\n" \
                        "Keep-Alive: timeout=5, max=1000\n" \
                        "Connection: Keep-Alive\n" \
                        "Content-Type: application/octet-stream; charset=UTF-8\n" \
                        "\n"

#ifdef	__cplusplus
}
#endif

#endif	/* HTTP_STR_H */

