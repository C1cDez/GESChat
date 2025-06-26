# GESChat

**GESChat** is a server-client side console chat application written in C++ for Windows. It uses custom communication protocol (see [GC Protocol](#gc-protocol)).

---

### Build
Idk, I used g++. <br>
Libs: [Winsock2](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/)

---

### Server
**How to run**: <br>
In command line `geschat-server-V-pX.Y.exe`. Additional command line arguments:

| Key | Meaning | Default Value |
|---|---|---|
| -p | Port to open server on | 1488
| -b | Backlog of a server (max users) | 128

e.g. `geschat-server-1.0.0-p1.0.exe -p 1161 -b 18` stands for opening server on port 1161 with server's backlog being 18 clients.

**How to exit**:<br>
Just abort like console program using Ctrl+C.


---

### Client

**How to run**: <br>
In command line `geschat-client-V-pX.Y.exe`. Additional command line arguments:

| Key | Meaning | Default Value |
|---|---|---|
| -h | Host of server to connect to | 127.0.0.1 (localhost)
| -p | Port on server to connect to | 1488

e.g. `geschat-client-1.0.0-p1.0.exe -p 1161 -h 148.81.161.18` will connect to `148.81.161.18:1161`

**Username**<br>
Username must contain only _latin letters_ (upper and lower case), _numbers_ (0-9) and _underscores_ (_). <br>
Username length is 16 characters (16 bytes).

**Sending Private Message**<br>
Type `@` followed by the name of user you want to send private message to. Name is separated from message by comma and space `, `.

**How to exit**:<br>
Type `~!` as a message. It will (probably) safely close all sockets.

---

### GC Protocol

Current Version of GESChat Protocol: 1.1

**Sizes** <br>
<ins>Username</ins> length is 17 bytes long, but only 16 of them are used to store information, last one is reserved as 0 for C-string. <br>
<ins>Message</ins> length is 257 bytes long, again 256 are used to store message and last one is for C-string.

**Packet Structure** <br>
In every packet, 0th byte stands for the _type_ of sent packet:
* 0x01 - Handshake
* 0x02 - Client sends message to server
* 0x03 - Server broadcasts message to clients
* 0x04 - Server broadcasts technical information (joining & leaving of users)
* 0x05 - Sending private message (from client)
* 0x06 - Forwarding private message (from server)

The rest is additional information (depending on the _type_)


**Handshake (20), (4)** <br>
Handshake is initiated by the client imediately after connection:
| Bytes | [0] | [1] - [2] | [3] - [19] |
|-|---|---|---|
| |0x01| MAJOR minor | ... |
| | Handshake type | Client's Protocol Version | Client's Username |



Server responses with:
| Bytes | [0] | [1] | [2] - [3] |
|---|---|---|---|
| | 0x01 | SEC | MAJOR minor |
| | Handshake type | Server Error Code | Server's Protocol version, if client's one is different |

Server Error Code:
* 0x40 - Server OK (no error)
* 0x41 - Incompatable Protocol Version
* 0x42 - Server Transcended User Limit
* 0x43 - Client with this Username is already on the Server
* 0x44 - Invalid username

**Sending Message (258)** <br>
| Bytes | [0] | [1] - [257] |
|---|---|---|
| | 0x02 | ... |
| | Sending Message type | Client's Message

**Broadcasting Message (275)** <br>
| Bytes | [0] | [1] - [17] | [18] - [274] |
|---|---|---|---|
| | 0x03 | ... | ... |
| | Broadcasting Message type | Sender's Username | Client's Message |

**Technical Broadcasting (19)** <br>
| Bytes | [0] | [1] | [2] - [18] |
|---|---|---|---|
| | 0x04 | STBC | ... |
| | Technical Broadcasting type | Server Technical Broadcasting Codes | Client's username

Server Technical Broadcasting Code:
* 0x20 - User joined
* 0x21 - User left

**Sending Private Messages**


**Future** <br>
Make encrypting and decrypting messages

---

That's all. Made in 4 days.
