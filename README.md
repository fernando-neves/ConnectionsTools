# ConnectionsToolsBoost

This repository contains a set of subprojects related to network communication using TCP and UDP protocols. The project is built with the Boost.Asio library for I/O operations and uses Plog for event logging. The subprojects include a client and a server for both TCP and UDP versions.

## Subprojects

1. **TCP Echo Client**: A client that establishes a TCP connection with a server and sends messages to it. The server responds with the received messages.

2. **TCP Echo Server**: A server that awaits TCP connections from clients and echoes back the received messages from clients.

3. **UDP Echo Client**: A client that sends UDP messages to a server and receives the messages back from the server.

4. **UDP Echo Server**: A server that listens to UDP packets from clients and echoes back the received messages.

## Configuration

1. Clone this repository:

```bash
git clone https://github.com/your-username/ConnectionsTools.git && cd ConnectionsTools && sudo ./bootstrap.sh