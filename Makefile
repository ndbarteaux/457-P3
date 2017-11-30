all: 
	g++ -o manager manager.cc router.cc

clean:
	rm -f manager
	rm *.out