// This file is used by client and server both TCP and UDP
// Basic functions used across all files are here such as logging outputs to the terminal
#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H
#define SERVER_PORT 13000 // Default server port for TCP and UDP

#include <iostream>     // For terminal input/output
#include <cstring>      // CLIENT: String length (strlen()) SERVER: memset
#include <ctime>        // Time for log timestamps
#include <iomanip>      // Format timestamp output HH:MM:SS
#include <sys/socket.h> // Socket creation and communication
#include <unistd.h>     // Close socket (close())
#include <netinet/in.h> // Internet address structs (sockaddr_in)
#include <arpa/inet.h>  // IP address conversion (inet_pton, htons)

using namespace std; // Probably not best practice but I don't like typeing std::[name] everywhere

int validate_amount(string prevalidated_amount);                             // Validate <amount> (no negative, no decimal, must be a number) (don't use $ sign in command line it cuts the input short)
int validate_years(string prevalidated_years);                               // Validate <years> (no negative, no decimal, must be a number)
int validate_rate(string prevalidated_rate);                                 // Validate <rate> (no negative, must be a number, can have % sign)
void log(const string &level, const string &msg, const string &detail = ""); // Extra: Consistent formatting for cout messages

#endif // NETWORK_UTILS_H
