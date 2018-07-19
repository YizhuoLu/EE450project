all: aws client serverA serverB serverC

aws: aws.c
	g++ -o aws aws.c
serverA: serverA.c
	g++ -o serverA serverA.c
serverB: serverB.c
	g++ -o serverB serverB.c
serverC: serverC.c
	g++ -o serverC serverC.c
client: client.c
	g++ -o client client.c

clean:
	$(RM) aws1 client1 serverA1 serverB1 serverC1
