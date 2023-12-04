# Distributed System Topics

The repo is primarily used for CS428 Distributed System Course at UIUC. I extended something based on the projects for this course.

There are 2 topics of distribued system related stuffs implemented. 

#### 1. Distributed Network Application

The first is a simple network application with a **<u>centralized logger</u>** and **<u>multiple nodes</u>**. The primary design is to make nodes send messages to the logger, and the logger should be able to log the received messages in an atomic manner. A simple analysis is also performed in the logger. 

- The original design is performed in a local machine with multi-processors.
- I slightly extended the design with containerization by utilizing docker and just for fun. 

#### 2. Decentralized Transaction Model

The second is a network application focusing on global ordering of events in order to perform consistent actions. In this project, the concepts of **<u>Reliable Multicast</u>** and **<u>Replica</u>** should be realized. The general idea is to guarantee the consistency and consensus between different nodes. 

- The original design is performed in a local machine with multi-processors.
- The extended design is in updating.