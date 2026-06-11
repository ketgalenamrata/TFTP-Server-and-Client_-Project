📡 TFTP Client–Server in C
TFTP (Trivial File Transfer Protocol) is a lightweight file transfer protocol that operates over UDP. This project implements a complete TFTP client and server in C, demonstrating reliable file transfer over a connectionless network.

✨ Features
UDP-based client–server architecture

Supports RRQ (Read) and WRQ (Write) operations

Reliable transfer using ACK and block numbering

Block-based data transmission

Multiple transfer modes:
Normal (512 bytes per block)

Octet (byte-wise transfer)

Netascii (newline conversion)

🛠️ Tech Stack
Language: C

OS: Linux

Networking: UDP Sockets

Concepts: Protocol Design, Client–Server Communication

⚙️ How It Works
UDP does not guarantee delivery, so reliability is implemented at the application level. Each data packet contains a block number, and the receiver sends an acknowledgment for successful receipt. If an ACK is missing or mismatched, the sender retransmits the packet.

📚 Key Learning Outcomes
Hands-on experience with socket programming

Understanding of reliable communication over UDP

Practical impleme
