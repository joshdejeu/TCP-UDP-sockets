#include "network_utils.h" // Headers shared by client & server
#include <cstring>         // CLIENT: String length (strlen()) SERVER: memset
#include <iostream>        // Console input/output
#include <ctime>           // Time for log timestamps
#include <iomanip>         // Format timestamp output
#include <sys/socket.h>    // Socket creation and communication
#include <unistd.h>        // Close socket (close())
#include <netinet/in.h>    // Internet address structs (sockaddr_in)
#include <arpa/inet.h>     // IP address conversion (inet_pton, htons)
#include <algorithm>       // For removing commas from string (validate_amount)

using namespace std;

// Validate <amount> (don't use $ sign in command line it cuts the input short)
int validate_amount(string amount_str)
{
    amount_str.erase(remove(amount_str.begin(), amount_str.end(), ','), amount_str.end()); // Remove commas
    double amount;
    try
    {
        amount = stod(amount_str);
        if (amount <= 0)
        {
            log("ERROR", "Invalid amount", "Must be positive");
            return 1;
        }
    }
    catch (const exception &e)
    {
        log("ERROR", "Invalid amount", amount_str + " is not a number");
        return 1;
    }
    log("INFO", "Amount is valid", amount_str);
    return 0; // Amount is valid
}

// Valid <years> (no negative, no decimal)
int validate_years(string years_str)
{
    // Check for decimal point
    if (years_str.find('.') != string::npos)
    {
        log("ERROR", "Invalid years", years_str + " is not an integer (has decimal)");
        return 1;
    }
    try
    {
        int years = stoi(years_str);
        if (years <= 0)
        {
            log("ERROR", "Invalid years", "Must be positive");
            return 1;
        }
    }
    catch (const exception &e)
    {
        log("ERROR", "Invalid years", years_str + " is not an integer");
        return 1;
    }
    log("INFO", "Years are valid", years_str);
    return 0; // Years is valid
}

// Validate <rate> (no negative, can have % sign)
int validate_rate(string rate_str)
{
    // Remove % if present
    if (!rate_str.empty() && rate_str.back() == '%')
    {
        rate_str.pop_back();
    }

    float rate;
    try
    {
        rate = stof(rate_str);
        if (rate < 0)
        {
            log("ERROR", "Invalid rate", "Must be positive");
            return 1;
        }
    }
    catch (const exception &e)
    {
        log("ERROR", "Invalid rate", rate_str + " is not a number");
        return 1;
    }
    log("INFO", "Rate is valid", rate_str);
    return 0; // Rate is valid
}

// Extra: Consistent formatting for cout messages
void log(const string &level, const string &msg, const string &detail)
{
    // Extra: This is just a stylistic way to consistently log all messages with timestamps

    // Get current time
    time_t now = time(nullptr);
    struct tm *local_time = localtime(&now);

    // Format timestamp as HH:MM:SS
    stringstream ss;
    ss << setfill('0') << setw(2) << local_time->tm_hour << ":"
       << setfill('0') << setw(2) << local_time->tm_min << ":"
       << setfill('0') << setw(2) << local_time->tm_sec;

    // I'm color coding INFO vs WARNING vs ERROR
    const string RESET = "\033[0m";
    const string GRAY = "\033[90m";
    const string TIMESTAMP_COLOR = "\033[36m";
    string color;
    if (level == "ERROR")
    {
        color = "\033[31m"; // Red
    }
    else if (level == "WARNING")
    {
        color = "\033[33m"; // Yellow
    }
    else if (level == "INFO")
    {
        color = "\033[32m"; // Green
    }
    else
    {
        color = RESET;
    }
    cerr << TIMESTAMP_COLOR << "[" << ss.str() << "] " << color << "[" << level << "] " << RESET << msg << GRAY;
    if (!detail.empty())
    {
        cerr << ": " << detail;
    }
    cerr << RESET << endl;
}
