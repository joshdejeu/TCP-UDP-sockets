#ifndef SERVER_H_UTILS_H
#define SERVER_H_UTILS_H
#include "../network/network_utils.h" // Headers shared by client & server

#include <string> // For strings from char*

using namespace std;

int split_by_space(string prevalidated_message, string output[]);     // Split string by spaces and populate array of strings
int validate_message(const string &prevalidated_message);             // Validates string with format <amount> <years> <rate>
double round_to_nearest_cent_amount(double amount);                   // Round double to nearest 2nd decimal place
double calculate_monthly_payment(int amount, int years, double rate); // Apply monthly loan calculation
string format_double(double value);                                   // Removes trailing 0's when applying to_string() to a double
string generate_payment_report(string terms[]);                       // Generate string of payment report

#endif // SERVER_H_UTILS_H