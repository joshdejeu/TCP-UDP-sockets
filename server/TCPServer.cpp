#include "server_utils.h" // Server specific headers

#define MAX_PENDING_CONNECTIONS 5 // Max pending connections

const int MESSAGE_BUFFER_SIZE = 1024; // Client message buffer size in bytes

int respond(int c_socket, string &validated_client_message); // Send response to client

int main(int argc, char *argv[])
{
    // Create the server socket
    int s_socket = socket(AF_INET, SOCK_STREAM, 0); // Make a new socket using SOCK_STREAM for TCP
    if (s_socket == -1)
    {
        log("ERROR", "Socket creation failed", strerror(errno));
        return 1; // Exit program
    }

    // Allow port reuse to allow quick rebind to the same port
    // Prevents "bind failed: Address already in use" error when restarting the program quickly
    // SO_REUSEADDR tells the kernel to reuse the port even if in a waiting state from a previous connection
    // Documentation on SO_REUSEADDR -  https://man7.org/linux/man-pages/man7/socket.7.html
    int opt = 1;                                                       // Set enable resuse option
    setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Apply SO_REUSEADDR to the socket

    // Configured specific IP and port for listening
    // Documentation on htons - https://linux.die.net/man/3/htons
    // Documentation on INADDR_ANY - https://man7.org/linux/man-pages/man7/ip.7.html
    sockaddr_in serverAddr{};                 // Initialize server address
    serverAddr.sin_family = AF_INET;          // Set address family to IPv4
    serverAddr.sin_port = htons(SERVER_PORT); // Assign port number in network byte order
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // Listine to incoming connections from any source

    // Bind the socket to the configured server address and port
    if (bind(s_socket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        log("ERROR", "Bind failed");
        close(s_socket);
        return 1; // Exit program
    }

    // Listen for incoming connections
    if (listen(s_socket, MAX_PENDING_CONNECTIONS) == -1)
    {
        log("ERROR", "Listen failed");
        close(s_socket);
        return 1; // Exit program
    }

    log("INFO", "Server listening on port", to_string(SERVER_PORT)); // Log that server is ready to listen

    int c_socket = -1; // Initialize socket variable for access outside while loop

    // Infinite loop to listen to new request after each client socket connection closes
    while (true)
    {
        // Accept incoming client connections in an infinite loop
        sockaddr_in clientAddress{};                                                   // Initialize client address struct
        socklen_t clientAddressLength = sizeof(clientAddress);                         // Set size of client address struct
        c_socket = accept(s_socket, (sockaddr *)&clientAddress, &clientAddressLength); // Accept new client connection
        if (c_socket == -1)
        {
            // Fail
            log("ERROR", "Accept failed");
            close(s_socket); // Close server socket on fail
            return 1;        // Exit program
        }

        // Log the address which the client socket connected from
        // Documentation on inet_ntoa - https://linux.die.net/man/3/inet_ntoa
        log("INFO", "Client connected from", inet_ntoa(clientAddress.sin_addr) + string(":") + to_string(ntohs(clientAddress.sin_port))); // Log client info

        char buffer[MESSAGE_BUFFER_SIZE] = {0};                            // Initialize buffer to receive client message
        int bytesReceived = recv(c_socket, buffer, sizeof(buffer) - 1, 0); // Store received cliet message in buffer
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0'; // Null-terminate to make a valid C-string when converting to string
            log("INFO", "Message from client", string(buffer));

            // Validate client message
            string client_message = string(buffer);
            if (!client_message.empty() && validate_message(client_message) == 0)
            {
                // Handle a succesfully received message that has also been validated
                if (respond(c_socket, client_message) != 0)
                {
                    // Fail if response doesn't get sent
                    return 1; // Exit program
                }
            }
        }
        else
        {
            log("ERROR", "No response from client");
            break;
        }
        close(c_socket); // Close the client socket each iteration
    }

    // Close all sockets for cleanup
    close(s_socket);
    close(c_socket);

    return 0; // Exit program
}

// Send load payment report response to client socket
// Return 0 if message sent succesfully, -1 if failed
int respond(int c_socket, string &validated_client_message)
{
    const int MAX_SIZE = 3;                                   // Max number of arguments
    string terms[MAX_SIZE];                                   // Array to hold split arguments
    split_by_space(validated_client_message, terms);          // Populates terms[], no need to check for too many arguments since this message has been validated
    string response_message = generate_payment_report(terms); // Generate loan payment report

    ssize_t bytes_to_send = strlen(response_message.c_str());                        // Get the length of the response (bytes)
    ssize_t bytes_sent = send(c_socket, response_message.c_str(), bytes_to_send, 0); // Send response to client and store status - Params (connected socket) (buffer) (length) (flags)
    if (bytes_sent == -1)
    {
        log("ERROR", "Failed to send response");
        return -1; // Fail
    }
    else
    {
        log("INFO", "Response sent to client", response_message);
        return 0; // Success
    }
    return -1; // Assume failure
}