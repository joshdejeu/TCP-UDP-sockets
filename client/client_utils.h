#ifndef CLIENT_H_UTILS_H
#define CLIENT_H_UTILS_H
#include "../network/network_utils.h" // Headers shared by client & server

int validate_command_line_arguments(int argc, char *argv[], sockaddr_in &serverAddress); // Validates ALL 4 command line arguments with helper methods
int validate_ip(char *address, sockaddr_in &serverAddress);                              // Validate an <ip> either hostname or numeric address (IPv4 only) and configure sockaddr_in

#endif // CLIENT_H_UTILS_H