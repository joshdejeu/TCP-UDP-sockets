#include "client_utils.h" // Client specific headers

#include <fcntl.h> // Socket mode control - setting non-blocking (fcntl)

const int RETRY_INTERVAL = 1;          // Retry interval in seconds
const int MAX_RETRIES = 10;            // Limit retries to avoid infinite loop
const int RESPONSE_BUFFER_SIZE = 1024; // Server response buffer size in bytes

int attempt_new_TCP_connection(const sockaddr_in &serverAddress);                // Attempt TCP socket connection with timeout
int attempt_send(int c_socket, string &message, int flags = 0);                  // Sending message to server host via TCP
int await_and_display_server_response(int c_socket, sockaddr_in &clientAddress); // Handle response or no response from server

int main(int argc, char *argv[])
{
    sockaddr_in serverAddress{}; // IPv4 Server address and port setup
    // Validate ALL arguments <ip> <amount> <years> <rate>
    if (validate_command_line_arguments(argc, argv, serverAddress) != 0)
    {
        return 1; // Exit program
    }

    sockaddr_in clientAddress; // Used to get port number that client listens on for server response (for logging)

    int c_socket = -1; // Initialize socket variable for access outside while loop

    // Attempt to connect via TCP to server on a new socket each iteration
    int retries = 0;
    while (retries < MAX_RETRIES)
    {
        c_socket = attempt_new_TCP_connection(serverAddress); // Create new TCP socket
        if (c_socket != -1)
        {
            log("INFO", "Connected to server", string(argv[1]) + ":" + to_string(SERVER_PORT));
            // Gets the local address and port assigned to client socket (used for logging later)
            // Documentation on getsockname - https://man7.org/linux/man-pages/man2/getsockname.2.html
            socklen_t clientAddressLength = sizeof(clientAddress);
            if (getsockname(c_socket, (sockaddr *)&clientAddress, &clientAddressLength) == -1)
            {
                log("ERROR", "getsockname failed", strerror(errno));
                close(c_socket); // Close current socket
                return 1;        // Exit program
            }
            break; // Exit loop on succesful connection
        }
        retries++;
        sleep(RETRY_INTERVAL); // Wait before retrying
    }

    // Handle failed connection after MAX_RETRIES attempts
    if (c_socket == -1)
    {
        log("ERROR", "Failed to connect after " + to_string(MAX_RETRIES) + " attempts");
        return 1; // Exit program
    }

    // Client should be succesfully connected by this point
    // Attempt to send initial command line message
    string message = string(argv[2]) + " " + argv[3] + " " + argv[4]; // Message with validated arguments <amount> <ip> <rate>
    int message_to_send = attempt_send(c_socket, message);
    if (message_to_send == 0)
    {
        // Wait for server response and display it
        int s_response = await_and_display_server_response(c_socket, clientAddress);
    }

    // This whole program is only meant to send messages typed in the command line
    // If you want to send a new message to the server you have to type the command again
    close(c_socket); // Close socket
    return 0;        // Exit program
}

// Attempts a TCP connection with a new socket to some serverAddress
// Returns socket on connect() success, -1 on failure
int attempt_new_TCP_connection(const sockaddr_in &serverAddress)
{
    int new_socket = socket(AF_INET, SOCK_STREAM, 0); // Make a new socket using SOCK_STREAM for TCP
    if (new_socket == -1)
    {
        log("ERROR", "Socket creation failed", strerror(errno));
        return -1;
    }

    // This part is complex, the socket will be set to non-blocking mode and back to blocking in 3 steps
    // When connecting to localhost address (127.x.x.x), the OS handles failures quickly
    // When connecting to a non-localhost address (128.x.x.x), the connection attempt may take longer-
    // potentially longer than my default RETRY_INTERVAL before failing (this causes the program to sit idly for minutes sometimes)
    // I prevent this by setting O_NONBLOCK so connect() returns immediatly, even if the connection is still in progress-
    // allowing me to handle timeouts manually rather than blocking execution

    // Step 1: Get current socket flags to modify later
    // - Why? This is required to avoid overwriting unrelated flags when setting O_NONBLOCK
    // - Documentation on fcntl - https://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html
    int flags = fcntl(new_socket, F_GETFL, 0); // Store current flags on this socket

    // Step 2: Set a non-blocking flag on this socket while preserving other flags
    // - Using bitwise OR with | to add O_NONBLOCK flag without losing other flags
    // - Example: if flags = 02 with O_RDWR enabled, and O_NONBLOCK = 04000, flags | O_NONBLOCK = 04002, this preserves both O_RDWR and O_NONBLOCK
    // - Documentation on flag octal bit values - https://sites.uclouvain.be/SystInfo/usr/include/bits/fcntl.h.html
    // - Documentation on c++ bitwise operators - https://www.geeksforgeeks.org/cpp-bitwise-operators/
    int updated_flag_with_nonblocking = flags | O_NONBLOCK;    // Bitwise OR with | to set O_NONBLOCK while preserving other flag bits
    fcntl(new_socket, F_SETFL, updated_flag_with_nonblocking); // Set non-blocking flag O_NONBLOCK

    int connection_status = connect(new_socket, (sockaddr *)&serverAddress, sizeof(serverAddress)); // Attempt the connection

    if (connection_status == 0)
    {
        // Step 3: Switch socket back to blocking mode, while preserving other flags, after an immediate connection
        // - Using bitwise NOT with ~ to remove O_NONBLOCK, then bitwise AND with & to preserve all other flags
        // - Immediate connections happen when connect() returns 0 instantly such as with localhost or if the server is already connected
        // - Why switching back? O_NONBLOCK will make recv() return too fast causing it to miss the server response
        // - If we don't set the flags back to blocking then later on recv() will continue being non-blocking and will close too quickly before waiting for a server response
        // - Example: if flags for O_RDWR and O_NONBLOCK = 04002, and O_NONBLOCK = 04000, flags & ~O_NONBLOCK = 02, this preserves O_RDWR while removing O_NONBLOCK
        // - Documentation on flag octal bit values - https://sites.uclouvain.be/SystInfo/usr/include/bits/fcntl.h.html
        // - Documentation on c++ bitwise operators - https://www.geeksforgeeks.org/cpp-bitwise-operators/
        // - In previous logs, I noticed 127.0.01 connected right away but would give an error immediately after sending a message since it would not sit to wait for the server to respond. Sometimes the response was received if I kept spamming the program but it failed about 80% of the time (with O_NONBLOCK still set)
        int updated_flag_with_blocking = updated_flag_with_nonblocking & ~O_NONBLOCK; // Invert bits of O_NONBLOCK with ~ while preserving other flag bits with &
        fcntl(new_socket, F_SETFL, updated_flag_with_blocking);                       // Set a blocking flag
        return new_socket;                                                            // Immediate success
    }
    else if (connection_status == -1)
    {
        // This else-if{} block lets me adjust the timeout manually
        // Without this, a request to 128.x.x.x would take minutes to timeout if server doesn't send any response for failure or success
        if (errno == EINPROGRESS)
        {
            // Handles case where connect() fails or is in progress due to O_NONBLOCK
            // This manual timeout control instead of waiting for OS default
            // Connection is in progress with non-blocking mode, select() used to wait for completion
            fd_set write_fds;               // Initialize file descriptor set
            FD_ZERO(&write_fds);            // Clear file descriptor set
            FD_SET(new_socket, &write_fds); // Add current socket for checking writability

            struct timeval timeout;          // Initialize timeout struct
            timeout.tv_sec = RETRY_INTERVAL; // Manually set timeout to RETRY_INTERVAL seconds, this adds 1 second to non-local address connection requests
            timeout.tv_usec = 0;             // Specify no microseconds (since RETRY_INTERVAL is 1 second)

            int sel = select(new_socket + 1, nullptr, &write_fds, nullptr, &timeout); // Waits for socket to become writable to get connection result
            if (sel > 0)
            {
                // Socket is writeable, now check if connnection succeeded or failed
                int error = -1;                                                       // Initiailze to -1 to avoid garbage data
                socklen_t len = sizeof(error);                                        // Get length of error in bytes
                if (getsockopt(new_socket, SOL_SOCKET, SO_ERROR, &error, &len) == -1) // Check socket error status
                {
                    log("ERROR", "getsockopt failed", strerror(errno));
                    close(new_socket);
                    return -1; // Fail
                }
                if (error == 0)
                {
                    fcntl(new_socket, F_SETFL, updated_flag_with_nonblocking & ~O_NONBLOCK); // Reset blocking flag, same as previous Step 3
                    return new_socket;                                                       // Successful connection to non-local address
                }
                else
                {
                    log("ERROR", "Connection timeout", strerror(error));
                }
            }
            else if (sel == 0)
            {
                // Handle server taking too long to respond
                log("ERROR", "Connection timeout", strerror(errno));
            }
            else
            {
                log("ERROR", "select() failed", strerror(errno));
            }
        }
        else if (errno == ECONNREFUSED)
        {
            // Handle server refusing right away
            log("ERROR", "Connection refused", "Server is not accepting connections");
        }
        else
        {
            // Handle any other connection failures
            log("ERROR", "Connection failed", strerror(errno));
        }
    }
    close(new_socket);
    return -1;
}

// Attempt to send command line message to server
// Assumes: A TCP connection has been successfully established or program would have terminated already
// Return 0 when message successfully sent, -1 on fail
int attempt_send(int c_socket, string &message, int flags)
{
    if (c_socket < 0)
    {
        // Make sure socket is valid
        log("ERROR", "Invalid socket descriptor", to_string(c_socket));
        return -1; // Fail
    }

    // Try <amount> without the "$" since in C++ that is a variable prefix and must be escaped
    const char *message_cstr = message.c_str();                              // Convert string to C-string
    ssize_t bytes_to_send = strlen(message_cstr);                            // Get the length of the message (bytes)
    ssize_t bytes_sent = send(c_socket, message_cstr, bytes_to_send, flags); // Send message and store status - Params (connected socket) (buffer) (length) (flags)

    // Handle different statuses
    if (bytes_sent == -1)
    {
        // Handle fail to send message
        log("ERROR", "Failed to send message", strerror(errno));
        return -1; // Fail
    }
    if (bytes_sent < bytes_to_send)
    {
        // Handle only partial message sent, since this is TCP it would be a fail
        log("ERROR", "Partial send", to_string(bytes_sent) + " of " + to_string(bytes_to_send) + " bytes sent");
        return -1; // Fail
    }
    else
    {
        // Handle success of message being sent
        log("INFO", "Sent message", to_string(bytes_sent) + " bytes");
        return 0; // Success
    }

    return -1; // Assume failure
}

// Wait for a reply from the server and display the response
// Return 0 on success, -1 on fail
int await_and_display_server_response(int c_socket, sockaddr_in &clientAddress)
{
    log("INFO", "Awaiting response on port", to_string(ntohs(clientAddress.sin_port)));
    char buffer[RESPONSE_BUFFER_SIZE] = {0};                           // Initialize a buffer populated with 0's
    int bytesReceived = recv(c_socket, buffer, sizeof(buffer) - 1, 0); // Store received server response in buffer
    // Handle different states of received messages
    if (bytesReceived > 0)
    {
        // Handle succesfully received message
        buffer[bytesReceived] = '\0'; // Null-terminate to make a valid C-string when converting to string
        log("INFO", "Received response", string(buffer));
        return 0; // Success
    }
    else if (bytesReceived == 0)
    {
        // Handle message length 0 bytes, server likely closed connection
        log("WARNING", "Connection closed by server", "0 bytes received");
        return -1; // Fail
    }
    else
    {
        // Handle no message received
        log("ERROR", "Receive failed", strerror(errno));
        return -1; // Fail
    }
    return -1; // Assume failure
}
