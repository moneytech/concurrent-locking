// Peter A. Buhr and Ashif S. Harji, Concurrent Urban Legends, Concurrency and Computation: Practice and Experience,
// 2005, 17(9), Figure 3, p. 1151

#ifdef FAST
// Want same meaning for FASTPATH for both experiments.
#undef FASTPATH
#define FASTPATH(x) __builtin_expect(!!(x), 1)
#endif // FAST

enum Intent { DontWantIn, WantIn };
static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static volatile TYPE intents[2] CALIGN = { DontWantIn, DontWantIn }, last CALIGN = 0;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define inv( c ) ((c) ^ 1)
#define await( E ) while ( ! (E) ) Pause()

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	int other = inv( id );								// int is better than TYPE

#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
#ifdef FLICKER
			for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
#endif // FLICKER
			intents[id] = WantIn;
			// Necessary to prevent the read of intents[other] from floating above the assignment
			// intents[id] = WantIn, when the hardware determines the two subscripts are different.
			Fence();									// force store before more loads
			while ( intents[other] == WantIn ) {
				if ( FASTPATH( last == id ) ) {
#ifdef FLICKER
					for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
#endif // FLICKER
					intents[id] = DontWantIn;
					while( last == id ) Pause();		// low priority busy wait
#ifdef FLICKER
					for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
#endif // FLICKER
					intents[id] = WantIn;
					// Necessary to prevent the read of intents[other] from floating above the assignment
					// intents[id] = WantIn, when the hardware determines the two subscripts are different.
					Fence();							// force store before more loads
				} else {
					Pause();
				} // if
			} // while
			CriticalSection( id );
#ifdef FLICKER
			for ( int i = id; i < 100; i += 1 ) last = i % 2; // flicker
#endif // FLICKER
			last = id;									// exit protocol
#ifdef FLICKER
			for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
#endif // FLICKER
			intents[id] = DontWantIn;

#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			other = inv( id );
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
#ifdef FAST
		id = oid;
		other = inv( id );
#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	if ( N != 2 ) {
		printf( "\nUsage: N=%d must be 2\n", N );
		exit( EXIT_FAILURE);
	} // if
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=DekkerC Harness.c -lpthread -lm" //
// End: //
