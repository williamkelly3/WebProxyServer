#include "Proxy_Worker.h"
#include <sstream>
using namespace std;

/*
 * Purpose: constructor
 * receive: cs is the socket that is already connected to the requesting 
 *          client
 * Return:  none
 */
Proxy_Worker::Proxy_Worker(TCP_Socket *cs) {
    client_sock = cs;
    port = 80; // For a full blown proxy, the server information should be 
               // obtained from each request. However, we simply assume it 
               // to be 80 for our labs.
    server_url = NULL;
               // Must be obtain from each request.
}

/*
 * Purpose: destructor
 * receive: none
 * Return:  none
 */
Proxy_Worker::~Proxy_Worker() {
    if(server_url != NULL)
    {
        delete server_url;
    }

    if(client_request != NULL)
    {
        delete client_request;
    }

    if(server_response != NULL)
    {
        delete server_response;
    }
    server_sock.Close();
    client_sock->Close();
}

/*
 * Purpose: handles a request by requesting it from the server_url 
 * Receive: none
 * Return:  none
*/
void Proxy_Worker::handle_request() {
    string buffer;

    // Get HTTP request from the client, check if the request is valid by 
    // parsing it. (parsing is done using HTTP_Request::receive)
    // From the parsed request, obtain the server address (in code, 
    // server_url).
    cout << "New connection established." << endl;
    cout << "New proxy child process started." << endl;
    cout << "Getting request from client..." << endl;

    if(!get_request()) 
    {
        // did not get the request, something is wrong, stop this process
        return;
    }  

    // Just outputting the requrst.
    cout << endl << "Received request:" << endl;
    cout << "==========================================================" 
         << endl;
    client_request->print(buffer);
    cout << buffer.substr(0, buffer.length() - 4) << endl;
    cout << "==========================================================" 
         << endl;

    cout << "Checking request..." << endl;
    if(!check_request())
    {
        // request is invalid, something is wrong, stop this process
        return;
    }
    cout << "Done. The request is valid." << endl;

    // Forward the request to the server.
    // Receive the response header and modify the server header field
    // Receive the response body. Handle the default and chunked transfor 
    // encoding.
    cout << endl << "Forwarding request to server..." << endl;
    if(!forward_request_get_response()) {
        return;
    }
  
    //return the response to the client
    return_response();
    cout << "Connection served. Proxy child process terminating." << endl;  

    return;
}

/*
 * Purpose: receives the request from a client and parse it.
 * Receive: none
 * Return:  a boolean indicating if getting the request was succesful or 
 *          not
 */
bool Proxy_Worker::get_request() {
    try
    {
        // Get the request from the client (HTTP_Request::receive)
        client_request = HTTP_Request::receive(*client_sock);
        // Chck if the request is received correctly
        //
        //if(check_request() != true)
        //{
         //   return false;
        //}
        // Obtain the server_url from the request (HTTP_Request::get_host 
        // and HTTP_Request::get_path). url = host + path
        string host;
        client_request->get_host(host);
        string url = host + client_request->get_path();
        // parse the url using URL::parse
        //
        server_url = URL::parse(url);
    }
    // Return an internal server error code (this should never happen, 
    // hopefully)
    //
    catch(string msg)
    {
        cerr << msg << endl;
        proxy_response(500);
        return false;
    }
    // Catch an error that should be returned to the client as a response
    // This should not happen
    catch (unsigned code) {
        proxy_response(code);
        return false;
    }

    return true;
}

/*
 * Purpose: Check if the request just gotten is valid
 * Receive: none
 * Return:  the request is valid or not.
 */
bool Proxy_Worker::check_request()
{
    // 1. Make sure we're pointing to a server URL
    //    Respond a 404 Not Found if the server is invalid
    //    (That is server_url == NULL)
    if(server_url == NULL)
    {
        proxy_response(404);
        return false;
    }
    // 2. Filter out any "host" with the keyword "facebook"
    //    Note that we are filtering out "host" with "facebook".
    //    "path" with facebook is allowed.
    //    Respond a 403 forbidden for host with facebook.
    string host;
    client_request->get_host(host);
    if(host.find("facebook") != string::npos)
    {
        cout << "Request to URL contains facebook.\n"
                "Reject this request by a forbidden." << endl;
        proxy_response(403);
        return false;
    }
    return true;
}

/*
 * Purpose: Forwards a client request to the server and get the response
 *          1. Forward the request to the server
 *          2. Receive the response header and modify the server field
 *          3. Receive the response body. Handle both chunk/default encoding.
 * Receive: none
 * Return:  a boolean indicating if forwarding the request was succesful or not
 */
bool Proxy_Worker::forward_request_get_response() {
    // line 226 - 368 in client.cc can be very helpful for you.

    // pass the client request to the server
    // First, connect to the server
    try
    {
        server_sock.Connect(*server_url);
        client_request->send(server_sock);
    }
    catch(string msg)
    {
        cerr << msg << endl;
        cout << "Unable to connect to server." << endl;
        proxy_response(404);
    }

    // send the request to the server
    // receive the response header from the server
    string response_header, response_body;
    server_response->receive_header(server_sock, response_header, response_body);
    cout << "Response header received." << endl;
    // check the response header, is it default encoding or chunked
    server_response = HTTP_Response::parse(response_header.c_str(), response_header.length());
    if(server_response == NULL)
    {
        cerr << "Unable to parse the response header." << endl;
        exit(1);
    }

    int bytes_written = 0, bytes_left;
    int total_data;
    cout << "Receiving response body..." << endl;
    if(server_response->is_chunked() == false)
    {
        cout << "Default encoding transfer" << endl;
        cout << "Content-length: " << server_response->get_content_len() << endl;
        bytes_left = server_response->get_content_len();
        do
        {
            // If we got a piece of the file in our buffer for the headers,
            // have that piece written out to the file, so we don't lose it.
            server_response->content.append(response_body.c_str());
            bytes_written += response_body.length();
            bytes_left -= response_body.length();
            //cout << "bytes written:" <<  bytes_written << endl;
            //cout << "data gotten:" <<  response_body.length() << endl;

            response_body.clear();
            try
            {
                // Keeps receiving until it gets the amount it expects.
                server_response->receive_data(server_sock, response_body,
                        bytes_left);
            }
            catch(string msg)
            {
                // something bad happend
                cout << msg << endl;
                // clean up
                delete server_response;
                delete server_url;
                server_sock.Close();
                exit(1);
            }
        } while (bytes_left > 0);
    }
    else // chunked encoding
    {
        cout << "Chunked encoding transfer" << endl;

        // As mentioned above, receive_header function already split the
        // data from the header from us. The beginning of this respnse_data
        // now holds the first chunk size.
        //cout << response_body.substr(0,15) << endl;
        int chunk_len = get_chunk_size(response_body);
        cout << "chunk length: " << chunk_len << endl;
        total_data = chunk_len;
        while(1)
        {
            // If current data holding is less than the chunk_len, this
            // piece of data contains only part of this chunk. Receive more
            // until we have a complete chunk to store!
            if(response_body.length() < (chunk_len + 4))
            {
                try
                {
                    // receive more until we have the whole chunk.
                    server_response->receive_data(server_sock, response_body,
                            (chunk_len - response_body.length()));
                    server_response->receive_line(server_sock, response_body);
                    // get the blank line between chunks
                    server_response->receive_line(server_sock, response_body);
                    // get next chunk, at least get the chunk size
                }
                catch(string msg)
                {
                    // something bad happend
                    cout << msg << endl;
                    // clean up
                    delete server_response;
                    delete server_url;
                    server_sock.Close();
                    exit(1);
                }
            }
                // If current data holding is longer than the chunk size, this
                // piece of data contains more than one chunk. Store the chunk.
            else//response_body.length() >= chunk_len
            {
                server_response->content.append(response_body.c_str());
                bytes_written += chunk_len;

                // reorganize the data, remove the chunk from it
                // the + 2 here is to consume the extra CLRF
                response_body = response_body.substr(chunk_len + 2,
                        response_body.length() - chunk_len - 2);
                //get next chunk size
                chunk_len = get_chunk_size(response_body);
                total_data += chunk_len;
                cout << "chunk length: " << chunk_len << endl;
                if(chunk_len == 0)
                {
                    break;
                }
            }
        }
    }

    // This checks if the chunked encoding transfer mode is downloading
    // the contents correctly.
    if((total_data != bytes_written) && server_response->is_chunked() == true)
    {
        cout << "WARNING" << endl
             << "Data received does not match chunk size." << endl;
    }

    if(server_response->get_status_code() != 200)
    {
        cerr << server_response->get_status_code() << " "
             << server_response->get_status_desc() << endl;
    }
    // receive response body

    return true;
}

/*
 * Purpose: Return the response from the server to the client
 *          Also modify the server field.
 *          
 * Receive: none
 * Return:  a boolean indicating if returning the request was succesful or not
 *          (always true for now)
*/
bool Proxy_Worker::return_response() {
    // Also modify the server field. Change it to whatever you want.
    // As long as it is modified, it is good.
    server_response->set_header_field("Server", "Changed it");

    // just outputting the response, it is interesting to have a look.
    string buffer;
    cout << endl;
    cout << "Returning response to client ..." << endl; 
    cout << "==========================================================" 
         << endl;
    buffer.clear();
    server_response->print(buffer);
    cout << buffer.substr(0, buffer.length() - 4) << endl;
    cout << "==========================================================" 
         << endl;
    
    // Some code to return the response to the client.
    server_response->send_no_error(*client_sock);

    return true;
}

/*
 * Purpose: Create a response "locally" and return it to a client
 *          For error situations like 403, 404, and 500 .. etc
 * Receive: the error code
 * Return:  a boolean indicating if returning the request was succesful
 *          or not (always true for now)
*/
bool Proxy_Worker::proxy_response(int status_code) {
    string buffer;
    HTTP_Response proxy_res(status_code);
    stringstream ss;
    int content_length = int(proxy_res.content.length());
    ss << content_length;

    proxy_res.set_header_field("Content-Length", ss.str());
    cout << endl << "Returning " << status_code << " to client ..." << endl;
    cout << "==========================================================" 
         << endl;
    buffer.clear();
    proxy_res.print(buffer);
    cout << buffer.substr(0, buffer.length() - 4) << endl;
    cout << "==========================================================" 
         << endl;
    proxy_res.send_no_error(*client_sock);
    return true;
}

/*
 * Purpose: Extract the chunk size from a string
 * Receive: the string
 * Return:  the chunk size in int
 *          Note that the chunk size in hex is removed from the string.
 */
// You probably will need this function in 
// Proxy_Worker::forward_request_get_response()
// You can find a similar function in client.h
// You can either remove the hex chunk size or leave it in the data string.
// Both is fine. Maybe you dont want to implement this and you are able 
// to extract the chunk size in the function. It is fine.
// If you dont want this, dont forget to remove the function prototype in 
// Proxy_Worker.h.
int Proxy_Worker::get_chunk_size(string &data)
{
    int chunk_len;          // The value we want to obtain
    int chunk_len_str_end;  // The var to hold the end of chunk length string
    std::stringstream ss;   // For hex to in conversion

    chunk_len_str_end = data.find("\r\n"); // Find the first CLRF
    string chunk_len_str = data.substr(0, chunk_len_str_end);
    // take the chunk length string out

    // convert the chunk length string hex to int
    ss << std::hex << chunk_len_str;
    ss >> chunk_len;

    // reorganize the data
    // remove the chunk length string and the CLRF
    data = data.substr(chunk_len_str_end + 2, data.length() - chunk_len_str_end - 2);

    //cout << "chunk_len_str: " << chunk_len_str << endl;
    //cout << "chunk_len:     " << chunk_len << endl;

    return chunk_len;
}
