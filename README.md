# EE450project
socket programming using c


Distributed lookup services (such as the Google File System , and Distributed Hash
Tables ) tackle the problem of locating data that is distributed over multiple nodes in the
network. In this project we shall implement a simplified version of a lookup service that'll
help us explore the core issues involved. Specifically, you'll be given a (distributed)
database consisting of words in the English language, and their definitions. The
database will be in plain text and consist of multiple key (the word), value (the
corresponding definition) pairs.

In this project, you will implement a model of distributed lookup service where a single
client issues a (dictionary) key search to a server which in turn searches for the key
(and it's associated value) over 3 backend servers. The server facing the client then
collects the results from the backend servers, performs additional computation on the
results if required, and communicates it to the client in the required format (This is also
an example of how a cloud-computing service such Amazon Web Services might speed
up a large computation task offloaded by the client). The monitor also receives the
results from the server and receive additional information if required

The server communicating with the client is called AWS (Amazon Web Server). The
three backend servers are named Back-Server A, Back-Server B and Back-Server C.
Back-Server A has access to a database file named dict_A.txt , Back-Server B has
access to a database file named dict_B.txt , and Back-Server C chas access to a
database file named dict_C.txt . The client, monitor and the AWS communicates over
a TCP connection while the communication between AWS and the Back-Servers A, B &
C is over a UDP connection.
