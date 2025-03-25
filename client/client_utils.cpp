#include "client_utils.h" // Client specific headers
#include <netdb.h>        // For getaddrinfo

// Expects 4 arguments <ip> <amount> <years> <rate>
// Returns 0 if valid arguments, 1 if too many arguments, -1 if invalid argument is present
int validate_command_line_arguments(int argc, char *argv[], sockaddr_in &serverAddress)
{
    // Make sure only 3 arguments were provided <amount> <years> <rate> (ignoring the first argument of the file name)
    if ((argc - 1) != 4)
    {
        log("ERROR", "Invalid arguments", "Usage: " + string(argv[0]) + " <ip> <amount> <years> <rate>");
        log("INFO", "Example", string(argv[0]) + " 127.0.0.1 150,000 30 4.69%");
        return 1; // Fail
    }
    // Validate <ip>, either hostname or IPv4 and confingure sockaddr_in
    if (validate_ip(argv[1], serverAddress) != 0)
    {
        return -1; // Fail
    }
    // Validate <amount> (must be a positive number, non-decimal, allow commas)
    if (validate_amount(string(argv[2])) != 0)
    {
        return -1; // Fail
    }
    // Validate <years> (must be a positive number, non-decimal)
    if (validate_years(string(argv[3])) != 0)
    {
        return -1; // Fail
    }
    // Validate <rate> (must be a positive number)
    if (validate_rate(string(argv[4])) != 0)
    {
        return -1; // Fail
    }
    return 0; // All arguments are valid
}

// Validate <ip>, resolves hostname to IPv4 address, or accepts IPv4 address and configure sockaddr_in
// Return 0 if <ip> valid, -1 if invalid
int validate_ip(char *address, sockaddr_in &serverAddress)
{
    // Documentation on addrinfo - https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    struct addrinfo hints; // Hints is a filter for getaddrinfo
    struct addrinfo *res;  // Pointer to struct addrinfo which holds an address filtered by getaddrinfo

    // Set up temporary server ip address and port number configurations
    // Documentation on htons - https://linux.die.net/man/3/htons
    serverAddress.sin_family = AF_INET;          // Set address family to IPv4
    serverAddress.sin_port = htons(SERVER_PORT); // Assign port number in network

    // Set up hints for getaddrinfo (to translate <ip>)
    // Documentation on memset - https://en.cppreference.com/w/cpp/string/byte/memset
    // Documentation on hints - https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    memset(&hints, 0, sizeof(hints)); // Initialize all data to 0 to avoid garbage values
    hints.ai_family = AF_INET;        // Set address family to IPv4
    hints.ai_socktype = 0;            // Filter based on socket type, 0 means no filter

    // Resolve the address (hostname or IP)
    // Documentation on getaddrinfo - https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    int status = getaddrinfo(address, NULL, &hints, &res);
    if (status != 0)
    {
        log("ERROR", "Invalid address or hostname", gai_strerror(status));
        return -1; // Fail
    }

    // Copy the resolved IPv4 address into [serverAddress.sin_addr]
    // res->ai_addr - Get a pointer to generic socket address from [struct addrinfo]
    // (struct sockaddr_in *)[prev] - Cast it to IPv4 specific sockaddr_in
    // [prev]->sin_addr - Finally, extract the resolved IP from [struct sockaddr_in]
    serverAddress.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;

    // Log success
    // Documentation on inet_ntop and INET_ADDRSTRLEN - https://man7.org/linux/man-pages/man3/inet_ntop.3.html
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &serverAddress.sin_addr, ip_str, INET_ADDRSTRLEN);
    string resolved_ip = string(ip_str);
    if (string(address) != string(ip_str))
    {
        // Check if address resolved to a different ip to avoid redundant output
        // Example: (127.0.0.1 -> 127.0.0.1) should just be (127.0.0.1)
        resolved_ip = string(address) + " -> " + resolved_ip;
    }
    log("INFO", "IP is valid", resolved_ip);

    // Documentation on freeaddrinfo - https://pubs.opengroup.org/onlinepubs/009604599/functions/freeaddrinfo.html
    freeaddrinfo(res); // Frees memory allocated to linked list res
    return 0;          // Success
}