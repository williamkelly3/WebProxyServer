#include "proxy.h"

/*
 * Name:    sample_handler
 * Purpose: Handles a single web request
 * Receive: You decide
 * Return:  You decide
 */
void sample_handler(TCP_Socket *client){
    //
    // Create a Proxy_Worker to handle this request
    //
    Proxy_Worker myProx(client);
    myProx.handle_request();

    //delete myProx;
    // Remember to clean up / delete the used Proxy_Worker
}

/*
 * Purpose: Contains the main server loop that handles requests by 
 *          spawing child processes
 * Receive: argc is the number of command line params, argv are the 
 *          parameters
 * Return:  0 on clean exit, and 1 on error
 */
int main(int argc, char *argv[]) {
    TCP_Socket listen_sock;
    TCP_Socket *client_sock;

    unsigned short int port = 0;
    int req_id = 0;

    pid_t pid;

    parse_args(argc, argv);


    // SETUP THE LISTENING TCP SOCKET
    // (BIND, LISTEN ... etc)

    listen_sock.Bind(port);
    listen_sock.Listen();
    // GET THE PORT NUMBER ASSIGNED BY THE OS
    // HAVE A LOOK AT TCP_Socket CLASS
    // You will also gain some insight of these classes by going through the
    // client program.
    listen_sock.get_port(port);
    cout << "Proxy running at " << port << "..." << endl;

    //start the infinite server loop
    while(true) {
        //accept incoming connections
        // For each incoming connectio, you want to new a client_sock
        // and do the accept. (Check out TCP_Sock Class)
        client_sock = listen_sock.Accept();

        // create new process
        pid = fork();
        // if pid < 0, the creation of process is failed.
        if(pid < 0)
        {
            cerr << "Unable to fork new child process." << endl;
            exit(1);
        }
        // if pid == 0, this is the child process
        else if(pid == 0)
        {
            sample_handler(client_sock);
            client_sock->Close();
            // try to close the client_sock            

            break; // Exit this main loop and terminate this child process
        }
        // if pid > 0, this is the parent process
        // Parent process continues the loop to accept new incoming 
        // connections.

        // The child process has done handleing this connection
        // Terminate the child process
    }

    if(pid == 0)
    {
        cout << "Child process terminated." << endl;
    }
    // The parent process
    else
    {
        // close the listening sock
        listen_sock.Close();
        cout << "Parent process terminated." << endl;
    }

    return 0;
}

