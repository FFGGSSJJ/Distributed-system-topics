# Distributed Network Application

#### Local Machine Usage

To run the program in local machine with multi-processors, simple run the following in the terminals:

```shell
# in terminal 1
./logger <localhost> <port>

# in terminal 2
./test.sh
```



#### Containerization Usage

I extended the code for a containerizaed manner in local machine, without many optimizations for the docker images. To use:

```shell
# create docker network
# can be deprecated as docker compose will create default network
docker network create distributed-nodes
# start up
docker-compose up

# shut down
docker-compose stop
docker-compose down
```



#### Output Check

For both methods, the log file will be generated in `./log`

