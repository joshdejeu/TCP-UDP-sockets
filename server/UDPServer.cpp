#include "server_utils.h"

#include <sstream> // For splitting message by commas

const string ACK_START = "\nACK_START"; // Custom protocol ACK
const string ACK_END = "\nACK_END";   // Custom protocol ACK

string await_message(int s_socket, sockaddr_in &clientAddress, const int BUFFER_SIZE = 1024); // Listen for a response from server
void respond(int c_socket, const string &message, sockaddr_in &clientAddress);                // Fire and forget a response to client

int main()
{
    // Create the server socket
    int s_socket = socket(AF_INET, SOCK_DGRAM, 0); // Make a new socket using SOCK_DGRAM for UDP
    if (s_socket == -1)
    {
        log("ERROR", "Socket creation failed");
        return 1; // Exit program
    }

    // Configured specific IP and port for listening
    // Documentation on htons - https://linux.die.net/man/3/htons
    // Documentation on INADDR_ANY - https://man7.org/linux/man-pages/man7/ip.7.html
    sockaddr_in serverAddress{};                 // Initialize server address
    serverAddress.sin_family = AF_INET;          // Set address family to IPv4
    serverAddress.sin_port = htons(SERVER_PORT); // Assign port number in network byte order
    serverAddress.sin_addr.s_addr = INADDR_ANY;  // Listine to incoming connections from any source

    // Bind the socket to the configured server address and port
    if (bind(s_socket, (sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        log("ERROR", "Bind failed");
        close(s_socket);
        return 1; // Exit program
    }

    log("INFO", "Server ready on port", to_string(SERVER_PORT));

    // Always stay open and await responses
    while (true)
    {
        sockaddr_in clientAddress{};                                          // Struct to store client ip address if a message is received
        const string client_message = await_message(s_socket, clientAddress); // Wait for a message
        // Validate client message
        if (!client_message.empty() && validate_message(client_message) == 0)
        {
            // Handle a succesfully received message that has also been validated
            respond(s_socket, client_message, clientAddress);
        }
    }

    close(s_socket); // Close server socket for cleanup
    return 0;        // Exit program
}

// Awaits a string message from client socket
// Return string of message on success, empty string on fail
string await_message(int s_socket, sockaddr_in &clientAddress, const int BUFFER_SIZE)
{
    // No timeout here since the server should always wait for incoming messages (unlike UDP client)
    char buffer[BUFFER_SIZE] = {0};                        // Buffer for the reply
    socklen_t clientAddressLength = sizeof(clientAddress); // Length of client ip address

    ssize_t recv_bytes = recvfrom(s_socket, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&clientAddress, &clientAddressLength); // Store client message in buffer

    // Handle different states of received messages
    if (recv_bytes == -1)
    {
        // Handle no message received
        log("ERROR", "No response", strerror(errno));
        return ""; // Fail
    }
    else
    {
        // Handle succesfully received message
        buffer[recv_bytes] = '\0'; // Null-terminate to make a valid C-string when converting to string
        log("INFO", "Message from client", string(buffer));
        return string(buffer); // Success
    }
    return ""; // Assume failure
}

// Fire and forget a response to client, no check if they received it
// No return
void respond(int c_socket, const string &message, sockaddr_in &clientAddress)
{
    const int MAX_SIZE = 3;
    string terms[MAX_SIZE];
    split_by_space(message, terms);
    string response_message = ACK_START + generate_payment_report(terms) + ACK_END; // My custom UDP protocol

    ssize_t bytes_to_send = strlen(response_message.c_str());
    ssize_t sent_bytes = sendto(c_socket, response_message.c_str(), bytes_to_send, 0, (sockaddr *)&clientAddress, sizeof(clientAddress)); // Send message
    if (sent_bytes == -1)
    {
        log("ERROR", "Failed to send response", strerror(errno));
        // return 1;
    }
    else
    {
        log("INFO", "Response to client", response_message);
    }
}