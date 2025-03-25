#include "server_utils.h" // Server specific headers

#include <math.h>    // For power function (pow)
#include <cmath>     // For roundinging (round)
#include <string>    // stringstream, getline, to_string, stoi, stod
#include <algorithm> // For removing commas (remove)
// #include <cctype>    // isspace

using namespace std;

// Split a string by each space and populate a string array
// Return 0 on success, -1 if too many arguments are provided
int split_by_space(string prevalidated_message, string output[])
{
    stringstream ss(prevalidated_message);    // String stream for while loop
    string parsed_argument;                   // Temporary storage for each parsed argument
    int argument_num = 0;                     // For tracking number of arguments
    while (getline(ss, parsed_argument, ' ')) // Split each argument by spaces (should only have 3)
    {
        if (argument_num > 2)
        {
            // Handle too many arguments
            log("ERROR", "Client sent too many arguments");
            return -1; // Failed validation
        }
        // Loop through each character and remove leading spaces (if any)
        while (!parsed_argument.empty() && isspace(parsed_argument.front()))
        {
            parsed_argument.erase(0, 1); // Remove leading space
        }
        // Remove any commas (will give stoi() problems later) (if any)
        parsed_argument.erase(remove(parsed_argument.begin(), parsed_argument.end(), ','), parsed_argument.end()); // Remove commas
        output[argument_num] = parsed_argument;                                                                    // Store new argument in output array (should have no leading spaces, no commas)
        argument_num++;
    }
    return 0; // Success
}

// Validate a message in the format <amount> <years> <rate>
// Return 0 on success, -1 on failure
int validate_message(const string &prevalidated_message)
{
    const int MAX_SIZE = 3;            // Max number of arguments
    string parsed_arguments[MAX_SIZE]; // Array to hold split arguments
    if (split_by_space(prevalidated_message, parsed_arguments) != 0)
    {
        // Handle too many arguments
        return -1; // Fail
    }

    if (validate_amount(parsed_arguments[0]) == 0 && validate_years(parsed_arguments[1]) == 0 && validate_rate(parsed_arguments[2]) == 0)
    {
        // All arguments are valid and have been validated
        return 0; // Success
    }

    return -1; // Assume failure
}

// Round a double to the nearest cent (2 decimal places)
// Return a double with 2 decimal places
double round_to_nearest_cent_amount(double amount)
{
    return round(amount * 100) / 100; // Round to 2 decimal places
}

// Calculate monthly payment for a loan using the amortization formula
// - Formula: (L*R)/(1-[1/(1+R)]^N), where L=amount, R=monthly rate, N=total payments
// Return a double of the calculated monthly payment
double calculate_monthly_payment(int amount, int years, double rate)
{
    double monthly_rate = (rate / 100) / 12; // Convert percentage to decimal and annual rate to monthly
    int total_payments = years * 12;         // Total number of payments (I assume this means per year)

    if (monthly_rate == 0)
    {
        // Handle zero interest case (avoid division by zero), even though rate must be a positive number
        return (double)amount / total_payments; // Cast amount from int to double and divide by total payments
    }

    double exact_amount = (amount * monthly_rate) / (1 - pow(1 + monthly_rate, -total_payments)); // Using the loan formula
    return round_to_nearest_cent_amount(exact_amount);
}

// Removes trailing 0's when applying to_string() to a double
// - Example: to_string(777.06) is "777.060000" which isn't a valid dollar amount
// Returns a valid dollar amount as a string
string format_double(double value)
{
    string str = to_string(value);            // Convert value to string (will have trailing 0's)
    str.erase(str.find_last_not_of('0') + 1); // Remove trailing zeros
    if (str.back() == '.')
        str.pop_back(); // Remove trailing dot if no decimals remain
    return str;         // Return formatted string of a double
}

// Return a report string with monthly and yearly payments from <amount> <years> <rate> given a string array
string generate_payment_report(string terms[])
{
    double monthly_payment = calculate_monthly_payment(stoi(terms[0]), stoi(terms[1]), stod(terms[2])); // Calculate monthly payment
    double yearly_payment = (monthly_payment * 12);                                                      // Total payment per year

    string output = string("\n$") + terms[0] + " loan\nmonthly payment is $" + format_double(monthly_payment) + "\ntotal payment is $" + format_double(yearly_payment); // Formatted output string

    return output; // Return formatted output string
}
