#include "client_utils.h" // Client specific headers

const int RETRY_INTERVAL = 1;          // Retry interval in seconds
const int MAX_RETRIES = 10;            // Limit retries to avoid infinite loop
const int RESPONSE_BUFFER_SIZE = 1024; // Server response buffer size in bytes

const string ACK_START = "\nACK_START"; // Custom protocol ACK
const string ACK_END = "\nACK_END";     // Custom protocol ACK

int create_UDP_socket();                                                     // Creates UDP socket
int send_message(int c_socket, string &message, sockaddr_in &serverAddress); // Fire and forget a message to server
int wait_response(int c_socket);                                             // Listen for a response from server
string remove_substring(string &input, const string &substring);             // Removes a substring from an input string

int main(int argc, char *argv[])
{
    sockaddr_in serverAddress{}; // IPv4 Server address and port setup
    // Validate ALL arguments <ip> <amount> <years> <rate>
    if (validate_command_line_arguments(argc, argv, serverAddress) != 0)
    {
        return 1; // Exit program
    }

    int c_socket = -1; // Initialize socket variable for access outside while loop

    c_socket = create_UDP_socket(); // Create UDP socket
    if (c_socket == -1)
    {
        return 1; // If socket creation failed, exit program
    }

    sockaddr_in clientAddress; // Used to get port number that client listens on for server response (for logging)

    string message_to_send = string(argv[2]) + " " + argv[3] + " " + argv[4]; // Pre-validated arguments

    // Send message to server (no connection required)
    int retries = 0;
    while (retries < MAX_RETRIES)
    {
        // Send message
        if (send_message(c_socket, message_to_send, serverAddress) == -1)
        {
            return 1; // Exit program
        }

        int response = wait_response(c_socket); // Wait with my protocol
        if (response == 0)
        {
            // Handle successfully received response
            // Get the local address and port assigned to client socket (used for logging later)
            // Documentation on getsockname - https://man7.org/linux/man-pages/man2/getsockname.2.html
            socklen_t addrLen = sizeof(clientAddress);
            if (getsockname(c_socket, (sockaddr *)&clientAddress, &addrLen) == -1)
            {
                log("ERROR", "getsockname failed", strerror(errno));
                close(c_socket);
                return 1; // Exit program
            }
            break; // Exit on success
        }

        retries++;
        sleep(1); // Wait before re-sending message
    }
    if (retries >= MAX_RETRIES)
    {
        log("ERROR", "Failed to send message after " + to_string(MAX_RETRIES) + " attempts");
    }

    return 0; // Exit program
}

// Create a new UDP socket
// Returns socket on success, -1 on fail
int create_UDP_socket()
{
    int new_socket = socket(AF_INET, SOCK_DGRAM, 0); // Make a new socket using SOCK_DGRAM for UDP
    if (new_socket == -1)
    {
        log("ERROR", "Socket creation failed", strerror(errno));
        return -1; // Fail
    }
    return new_socket; // Success
}

// Send message to server via UDP socket, doesn't check if server received
// Return 0 on success, 1 on partial send, -1 on fail
int send_message(int c_socket, string &message, sockaddr_in &serverAddress)
{
    // Try <amount> without "$" since in C++ that is a variable prefix and must be escaped
    ssize_t bytes_to_send = strlen(message.c_str()); // Get the length of the message (bytes)

    // Send message and store status
    // Params (socket) (buffer) (length) (flags) (server address) (server address length)
    ssize_t bytes_sent = sendto(c_socket, message.c_str(), bytes_to_send, 0, (sockaddr *)&serverAddress, sizeof(serverAddress));

    // Handle different statuses
    if (bytes_sent == -1)
    {
        // Handle fail to send message
        log("ERROR", "Failed to send message", strerror(errno));
        close(c_socket);
        return -1; // Fail
    }
    else if (bytes_sent < bytes_to_send)
    {
        // Handle only partial message sent, since this is UDP it would be a success but with warning
        log("WARNING", "Partial send", to_string(bytes_sent) + " of " + to_string(bytes_to_send) + " bytes sent");
        return 1; // Fail
    }
    else
    {
        // Handle success of message being sent
        log("INFO", "Sent message", to_string(bytes_sent) + " bytes");
        return 0; // Success
    }

    return -1; // Assume failure
}

// Listen for a response from server with RETRY_INTERVAL second timeout
// Return 0 on response received success, -1 on fail
int wait_response(int c_socket)
{
    struct timeval timeout = {RETRY_INTERVAL, 0}; // Set automatic timeout when waiting for response
    // Documentation on SO_RCVTIMEO and SOL_SOCKET - https://linux.die.net/man/7/socket
    setsockopt(c_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); // Apply SO_REUSEADDR to the socket

    char buffer[RESPONSE_BUFFER_SIZE] = {0};               // Initialize a buffer populated with 0's
    sockaddr_in serverAddress{};                           // To store server ip address
    socklen_t serverAddressLength = sizeof(serverAddress); // Length of server ip address

    ssize_t recv_bytes = recvfrom(c_socket, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&serverAddress, &serverAddressLength); // Store server response in buffer

    // Handle different states of received messages
    if (recv_bytes == -1)
    {
        // Handle no response after a timeout
        log("ERROR", "No response after " + to_string(RETRY_INTERVAL) + " seconds", strerror(errno));
        return -1; // Fail
    }
    else
    {
        // Handle message received
        // The custom protocol states that a succesful server response uses the format "ACK_START<result>ACK_END"
        // If "ACK_START" and "ACK_END" aren't detecteed in the response then some of the message must have been lost
        buffer[recv_bytes] = '\0'; // Null-terminate to make a valid C-string when converting to string
        const string SERVER_RESPONSE = string(buffer);
        const size_t ACK_START_FOUND = SERVER_RESPONSE.find(ACK_START); // Position of substring, string::npos If not found
        const size_t ACK_END_FOUND = SERVER_RESPONSE.find(ACK_END);     // Position of substring, string::npos If not found

        // Check for acknowledgement start and end
        if (ACK_START_FOUND != string::npos && ACK_END_FOUND != string::npos)
        {
            // Response contains ACK
            // Remove "ACK_START" and "ACK_END" from response string
            string formated_response = string(buffer);
            remove_substring(formated_response, ACK_START);
            remove_substring(formated_response, ACK_END);
            log("INFO", "Response from server", formated_response);
            return 0; // Success
        }
        else
        {
            // Message contains no ACK or partial ACK
            log("ERROR", "No ACK from server");
            return -1; // Fail
        }
    }
    return -1; // Assume failure
}

// Removes a substring from an input string
// - Assumes: substring is already contained in input string
// Returns string with substring removed
string remove_substring(string &input, const string &substring)
{
    // Double-check that substring is contained in input
    const size_t SUBSTRING_POSITION = input.find(substring); // Position of substring, string::npos If not found
    if (SUBSTRING_POSITION != string::npos)
    {
        // Substring found in input string
        input.erase(SUBSTRING_POSITION, substring.length());
    }

    return input; // Return updated string with substring erased
}