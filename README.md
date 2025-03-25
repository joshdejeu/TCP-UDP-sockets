# Financial loan calculator over TCP/UDP

## Overview
A client-server application in C++ that calculates financial data (loan interest) using TCP and UDP sockets. Demonstrates network communication, custom protocols, and error handling.

## Features
- Supports both TCP and UDP for client-server communication.
- Validates input for amount, years, and interest rate.
- Logs events with timestamps and status (INFO, WARNING, ERROR).

## Programming Language
- This program is written in C++ version: 201703
- Compiled with g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0

## How to Compile Binaries
- **TCPClient**: `g++ client/TCPClient.cpp client/client_utils.cpp network/network_utils.cpp -o compiled/TCPClient`
- **TCPServer**: `g++ server/TCPServer.cpp server/server_utils.cpp network/network_utils.cpp -o compiled/TCPServer`
- **UDPClient**: `g++ client/UDPClient.cpp client/client_utils.cpp network/network_utils.cpp -o compiled/UDPClient`
- **UDPServer**: `g++ server/UDPServer.cpp server/server_utils.cpp network/network_utils.cpp -o compiled/UDPServer`
#### *Note*: Binaries are named based on assignment details (page 2), although it states the command should include `Cal`, this way things are more consistent


# How to Run Code
- Binary file must be created from previous step before starting
- Open a terminal in the top level directory path (should include README.md and Sample.txt)

### TCP Client
- **Example Command**: `compiled/TCPClient 127.0.0.1 150,000 30 4.69%`
- **Binary Path**: `compiled/TCPClient`
- **Command Line Arguments**: `<ip> <amount> <years> <rate>`
- **Notes**: Local port changes on each run, see `Sample.txt`
- Connection attempts are limited to 10 before terminating

### TCP Server
- **Example Command**: `compiled/TCPServer`
- **Binary Path**: `compiled/TCPServer`
- **Note**: Listens on port 13000 by default (set in `network/network_utils.h`)

### UDP Client
- **Example Command**: `compiled/UDPClient 127.0.0.1 150,000 30 4.69%`
- **Binary Path**: `compiled/UDPClient`
- **Command Line Arguments**: `<ip> <amount> <years> <rate>`
- **Notes**: Local port changes on each run, see `Sample.txt`
- Message attempts are limited to 10 before terminating

### UDP Server
- **Example Command**: `compiled/UDPServer`
- **Binary Path**: `compiled/UDPServer`
- **Note**: Listens on port 13000 by default (set in `network/network_utils.h`)

# Terminal Output Format
- Example: `[00:42:05] [ERROR] Connection failed: Connection refused`
- In order to keep outputs as consistent as possible for ease of readability and debugging:
- Each output comes from the `log()` function in `network/network_utils.cpp`, they will contain the following data:
- `[time]`: A timestamp in the format HH:MM:SS
- `[status]`: One of three color-coded types, `[INFO]`, `[WARNING]`, or `[ERROR]`
- `[message]`: A basic description of what's happening
- `[error code]`: Optional, relevant data or error codes
- Because of this it is much easier to see how the timeouts apply for TCP and UDP on the client side

# Custom Protocol
TCP and UDP uses the same protocol for validating messages client-side before sending to server, and also server-side when receiving a message 
- Message to server must contain this format `<amount>` `<years>` `<rate>` (separated by spaces)
- `<amount>`: Must be a positive integer with no decimal places, it can contain commas but no `$` sign
- `<years>`: Must be a positive integer with no decimal places
- `<rate>`: Must be a positive float with/without decimal places

## TCP Protocol
#### TCP Client-Side
- A TCP socket connection with `AF_INET` must be established before sending the validated message (separated by spaces)
- If socket flag is set to non-blocking `(O_NONBLOCK)`, wait 1 second for a server response `(reset to blocking after a response)`
- If no server response, wait 1 additional second before retrying connection
- Attempt up to `MAX_RETRIES` connections before closing socket (to avoid infinite looping)

#### TCP Server-Side
- A TCP socket with `AF_INET` is created and binded to port `13000`, listens to all network interfaces 0.0.0.0 (set by `INADDR_ANY`)
- Accepts any incoming connection request and validates message data (should have the format `<amount>` `<years>` `<rate>`)
- Applies calculation and returns basic string message with no custom ACK, then listens for new connection

## UDP Custom Protocol
#### UDP Client-Side
- A UDP socket with `AF_INET` automatically sends the validated message (separated by spaces)
- Wait 1 second for a response from server
- If no response after 1 second, wait an additional second before re-sending message (prevents spamming server)
- If response received, check for `ACK_START` and `ACK_END` at the start and end of the response
- If response doesn't contain both `ACK_START` and `ACK_END`, send the message again and wait 1 second for a response
- If response contains both `ACK_START` and `ACK_END`, remove the ACK string from the start and end and display the output
- Attempt sending up to `MAX_RETRIES` messages before closing socket (to avoid infinite looping)

#### UDP Server-Side 
- A UDP socket with `AF_INET` is created and binded to port `13000`, listens to all network interfaces 0.0.0.0 (set by `INADDR_ANY`)
- Accepts any incoming connection request and validates message data (should have the format `<amount>` `<years>` `<rate>`)
- Applies calculation and returns basic string message
- Appends `ACK_START` to start and `ACK_END` to end of message before sending response to client, then listens for new message

## Known Bugs
- The client mishandles input with a `$` character for the `<amount>`.
From my research I believe this happens because the terminal reads `$` as a variable prefix meaning that it needs to be escaped with an extra character. If you give an input like `$150,000`, this gets read as `50000` from `argv[1]` before it can reach my validation function Although you would expect `$150,000`, in C++ it is a bit more complicated to implement an easy solution and not as elegant, so I've decided to assume all inputs are without it.
- Only IPv4 address are accepted, this is less of a bug and more of a design choice to avoid further complexity. There is a graceful response from the client side when an IPv6 address is used and it displays: `Address family for hostname not supported`.
- Numbers for `<amount>` `<years>` and `<rate>` are limited since the BUFFER_SIZE on both clients is set to 1024 bytes, in theory if a large enough number was entered you could get an overflow and see unexpected behavior. There are no checks for input size or minimal validation such as non-negative, non-character, and non-decimal (only for `<amount>` and `<years>`)
- In TCP client I don't check if fcntl() succeeds or fails to set the flags back to blocking mode, in rare cases this can cause the program to not wait for a server response and give an error message too quickly instead of waiting. The reason I chose not to include the check is that there is already a massive nested if-else statement and this operation has very high success anyways. For this project, since the messages are so small it is negligible. 

## MIT License
MIT License

Copyright (c) 2025 Josh Dejeu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.