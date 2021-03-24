#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int checkPow2(int x);
void fill( unsigned long long* arr, int num);
void DirectDirect( FILE *file, int cacheSize1, int block, int cacheSize2);
void fill2d( unsigned long long** arr, int x, int y);
void DirectAssoc( FILE *file, int cacheSize1, int block, int cacheSize2);
void DirectNAssoc(FILE *file, int cacheSize1, int block, int cacheSize2, int assoc);
void AssocDirect ( FILE *file, int cacheSize1, int block, int cacheSize2, char* policy);
void AssocAssoc ( FILE *file, int cacheSize1, int block, int cacheSize2, char* policy);
void AssocNAssoc ( FILE *file, int cacheSize1, int block, int cacheSize2, int assoc, char* policy);
void NAssocDirect( FILE *file, int cacheSize1, int assoc, int block, int cacheSize2, char* policy);
void NAssocAssoc( FILE *file, int cacheSize1, int assoc, int block, int cacheSize2, char* policy);
void NAssocNAssoc ( FILE *file, int cacheSize1, int assoc1, int block, int cacheSize2, int assoc2, char* policy);
unsigned long long pow2(unsigned long x);



int main ( int argc, char* argv[argc+1]){

	if ( argc != 9 ){
		printf("error\n");
		return EXIT_SUCCESS;
	}
	int cache = atoi(argv[1]);
	int block = atoi(argv[4]);
	int cacheL2 = atoi(argv[5]);
	if ( checkPow2(block) == 0 || checkPow2(cache) == 0 || checkPow2(cacheL2) == 0){
		printf("error\n");
		return EXIT_SUCCESS;
	}

	FILE *file = fopen(argv[8], "r");
	if ( file == 0){
		printf("error\n");
		return EXIT_SUCCESS;
	} else {

		char* assoc = argv[2];
		if ( assoc[0] == 'd'){
			char* assoc2 = argv[6];
			if ( assoc2[0] == 'd'){
				DirectDirect(file, cache, block, cacheL2);
			} else {
				if ( assoc2[5] == '\0'){
					DirectAssoc(file, cache, block, cacheL2);
				} else {
					int x = atoi(&(argv[6][6]));
					DirectNAssoc(file, cache, block, cacheL2, x);
				}
			}
		} else {

			char* policy = argv[3];
			if ( assoc[5] == '\0'){
				char* assoc2 = argv[6];
				if ( assoc2[0] == 'd'){
					AssocDirect(file, cache, block, cacheL2, policy);
				} else {
					if ( assoc2[5] == '\0'){
						AssocAssoc(file, cache, block, cacheL2, policy);
					} else {

						int x = atoi(&(argv[6][6]));
						AssocNAssoc( file, cache, block, cacheL2, x, policy);

					}
					
				}
				
			} else {
				int y = atoi(&(argv[2][6]));
				char* assoc2 = argv[6];
				if ( assoc2[0] == 'd'){
					NAssocDirect(file, cache, y, block, cacheL2, policy);
				} else {
					if ( assoc2[5] == '\0'){
						NAssocAssoc(file, cache, y, block, cacheL2, policy);
					} else {
						int x = atoi(&(argv[6][6]));
						NAssocNAssoc(file, cache, y, block, cacheL2, x, policy);
					}
				}
			}
		}
	}

	fclose(file);
	return EXIT_SUCCESS;
}

void NAssocDirect( FILE *file, int cacheSize1, int assoc, int block, int cacheSize2, char* policy){

	int set1 = cacheSize1/(block * assoc);
	int LinePerSet1 = assoc;
	int set2 = cacheSize2/block;
	int offsetBits = log2(block);
	int setBits1 = log2(set1);
	int setBits2 = log2(set2);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long** cache1 = malloc(sizeof(unsigned long long*) * set1);
	for ( int i = 0; i < set1; i++){
		cache1[i] = malloc(sizeof(unsigned long long) * LinePerSet1);
	}
	unsigned long long* cache2 = malloc(sizeof(unsigned long long) * set2);
	unsigned long long** address = malloc(sizeof(unsigned long long*) * set1);
	for ( int i = 0; i < set1; i++){
		address[i] = malloc(sizeof(unsigned long long) * LinePerSet1);
	}
	fill2d(cache1, set1, LinePerSet1);
	fill(cache2, set2);
	fill2d(address, set1, LinePerSet1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
	while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

		//calculate proper set and tag for l1 cache
		int setNum1 = (hex >> offsetBits) & ( pow2(setBits1) - 1 );
		unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
		//calculate proper set and tag for l2 cache
		int setNum2 = (hex >> offsetBits) & ( pow2(setBits2) - 1 );
		unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
		int i;
		int ref1 = 0; 
		for ( i = 0; i < LinePerSet1; i++){
			if ( cache1[setNum1][i] == tagNum1){
				ref1 = 1;
				break;
			}
		}
		//hit in l1
		if ( ref1 == 1){
			hitL1++;
			if ( c == 'W'){
				memwrites++;
			}
			//lru policy
			if ( policy[0] != 'f'){
				int v;
				int ref6 = 0;
				for ( v = 0; v < LinePerSet1; v++){
					if ( cache1[setNum1][v] == -1){
						ref6 = 1;
						break;
					}
				}

				//there is space in cache
				if ( ref6 == 1){
					for ( int k = i; k < v - 1; k++){
						cache1[setNum1][k] = cache1[setNum1][k+1];
						address[setNum1][k] = address[setNum1][k+1];
					}

					address[setNum1][v - 1] = hex;
					cache1[setNum1][v - 1] = tagNum1;

				}
				//no space
				else {

					//shift everything over
					for ( int m = i; m < LinePerSet1 - 1; m++){
						cache1[setNum1][m] = cache1[setNum1][m+1];
						address[setNum1][m] = address[setNum1][m+1];
					}
					cache1[setNum1][LinePerSet1-1] = tagNum1;
					address[setNum1][LinePerSet1-1] = hex;
					
				}
			}
		}

		else {
			//checking l2
			if ( cache2[setNum2] == tagNum2){
				hitL2++;
				missL1++;
				if ( c == 'W'){
					memwrites++;
				}
					
				// no empty space, have to evict a tag to l2, then add current to cache1

				unsigned long long tempHex = address[setNum1][0];
				int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
				unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);

				for ( int k = 0; k < LinePerSet1-1; k++){
					cache1[setNum1][k] = cache1[setNum1][k+1];
					address[setNum1][k] = address[setNum1][k+1];
				}
				cache1[setNum1][LinePerSet1 - 1] = tagNum1;
				address[setNum1][LinePerSet1 - 1] = hex;
				cache2[tempSetNum2] = tempTagNum2;

				if ( tempSetNum2 != setNum2){
					cache2[setNum2] = -1;
				}

					

			}
			//not in either
			else {

				int m;
				int ref3 = 0; 
				for ( m = 0; m < LinePerSet1; m++){
					if ( cache1[setNum1][m] == -1){
						ref3 = 1;
						break;
					}
				}
				//empty space in l1 cache
				if ( ref3 == 1){

					missL1++;
					missL2++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
					cache1[setNum1][m] = tagNum1;
					address[setNum1][m] = hex;

				} 
				//no empty space in l1 cache, must evict to l2
				else {
					unsigned long long tempHex = address[setNum1][0];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					for ( int l = 0; l < LinePerSet1-1; l++){
						cache1[setNum1][l] = cache1[setNum1][l+1];
						address[setNum1][l] = address[setNum1][l+1];
					}
					cache1[setNum1][LinePerSet1-1] = tagNum1;
					address[setNum1][LinePerSet1 - 1] = hex;
					cache2[tempSetNum2] = tempTagNum2;
					missL1++;
					missL2++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}


				}

			}
		} 

	}

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);

	for ( int i = 0; i < set1; i++){
		free(cache1[i]);
	}
	free(cache1);
	free(cache2);
	for ( int i = 0; i < set1; i++){
		free(address[i]);
	}
	free(address);
	
}


void NAssocAssoc( FILE *file, int cacheSize1, int assoc, int block, int cacheSize2, char* policy){
	int set1 = cacheSize1/(block * assoc);
	int LinePerSet1 = assoc;
	int LinePerSet2 = cacheSize2/block;
	int offsetBits = log2(block);
	int setBits1 = log2(set1);
	int setBits2 = log2(1);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long** cache1 = malloc(sizeof(unsigned long long*) * set1);
	for ( int i = 0; i < set1; i++){
		cache1[i] = malloc(sizeof(unsigned long long) * LinePerSet1);
	}
	unsigned long long* cache2 = malloc(sizeof(unsigned long long) * LinePerSet2);
	unsigned long long** address = malloc(sizeof(unsigned long long*) * set1);
	for ( int i = 0; i < set1; i++){
		address[i] = malloc(sizeof(unsigned long long) * LinePerSet1);
	}
	fill2d(cache1, set1, LinePerSet1);
	fill(cache2, LinePerSet2);
	fill2d(address, set1, LinePerSet1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
	while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){
		int setNum1 = (hex >> offsetBits) & ( pow2(setBits1) - 1 );
		unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
		unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
		int i;
		int ref1 = 0; 
		for ( i = 0; i < LinePerSet1; i++){
			if ( cache1[setNum1][i] == tagNum1){
				ref1 = 1;
				break;
			}
		}
		//hit in l1
		if ( ref1 == 1){
			hitL1++;
			if ( c == 'W'){
				memwrites++;
			}
			//lru policy
			if ( policy[0] != 'f'){
				int v;
				int ref6 = 0;
				for ( v = 0; v < LinePerSet1; v++){
					if ( cache1[setNum1][v] == -1){
						ref6 = 1;
						break;
					}
				}

				//there is space in cache
				if ( ref6 == 1){
					for ( int k = i; k < v - 1; k++){
						cache1[setNum1][k] = cache1[setNum1][k+1];
						address[setNum1][k] = address[setNum1][k+1];
					}

					address[setNum1][v - 1] = hex;
					cache1[setNum1][v - 1] = tagNum1;

				}
				//no space
				else {

					//shift everything over
					for ( int m = i; m < LinePerSet1 - 1; m++){
						cache1[setNum1][m] = cache1[setNum1][m+1];
						address[setNum1][m] = address[setNum1][m+1];
					}
					cache1[setNum1][LinePerSet1-1] = tagNum1;
					address[setNum1][LinePerSet1-1] = hex;
					
				}
			}
		}

		else {
			//check if in l2 cache
			int j;
			int ref2 = 0;
			for ( j = 0; j < LinePerSet2; j++){
				if ( cache2[j] == tagNum2){
					ref2 = 1;
					break;
				}
			}

			//is in second cache
			if ( ref2 == 1){
				missL1++;
				hitL2++;
				if ( c == 'W'){
					memwrites++;
				}

				//moving cache2 tag number out and sorting it
				cache2[j] = -1;
				for ( int m = j; m < LinePerSet2 -1 ; m++){
					cache2[m] = cache2[m+1];
				}
				cache2[LinePerSet2 - 1] = -1;
				// evicting 1st block from cache1
				unsigned long long tempHex = address[setNum1][0];
				unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);

				for ( int k = 0; k < LinePerSet1 - 1; k++){
					cache1[setNum1][k] = cache1[setNum1][k+1];
					address[setNum1][k] = address[setNum1][k+1];
				}
				cache1[setNum1][LinePerSet1-1] = tagNum1;
				address[setNum1][LinePerSet1 - 1] = hex;
				//finding first -1 slot to place evicted cahce1 block
				int n;
				int ref3 = 0;
				for ( n = 0; n < LinePerSet2; n++){
					if ( cache2[n] == -1){
						ref3 = 1;
						break;
					}
				}
				if ( ref3 == 1){
					//have found empty slot for tagnum
					cache2[n] = tempTagNum2;
				} else {
					//cache2 is also filled up, so have to move every array element one slot up
					for ( int k = 0; k < LinePerSet2 - 1; k++){
						cache2[k] = cache2[k+1];
					}
					cache2[LinePerSet2-1] = tempTagNum2;
				}

			}

			else {

				//not in cache1 or cache2
				//checking for empty space in l1 cache
				int s;
				int ref4 = 0; 
				for ( s = 0; s < LinePerSet1; s++){
					if ( cache1[setNum1][s] == -1){
						ref4 = 1;
						break;
					}
				}
				//empty space in l1 cache
				if ( ref4 == 1){

					missL1++;
					missL2++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
					cache1[setNum1][s] = tagNum1;
					address[setNum1][s] = hex;

				}
				else {

					//no empty space
					unsigned long long tempHex = address[setNum1][0];
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					for ( int l = 0; l < LinePerSet1-1; l++){
						cache1[setNum1][l] = cache1[setNum1][l+1];
						address[setNum1][l] = address[setNum1][l+1];
					}
					cache1[setNum1][LinePerSet1-1] = tagNum1;
					address[setNum1][LinePerSet1-1] = hex;
					missL1++;
					missL2++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
					//put evicted block in right spot
					int t;
					int ref5 = 0;
					for ( t = 0; t < LinePerSet2; t++){
						if ( cache2[t] == -1){
							ref5 = 1;
							break;
						}
					}

					//there is an empty slot in cache2
					if ( ref5 == 1){
						cache2[t] = tempTagNum2;
					} else {
						//move everything one to the left
						for ( int k = 0; k < LinePerSet2 - 1; k++){
							cache2[k] = cache2[k+1];
						}
						cache2[LinePerSet2-1] = tempTagNum2; 
					}
				 }
			}


		}

	}

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);

	for ( int i = 0; i < set1; i++){
		free(cache1[i]);
	}
	free(cache1);
	free(cache2);
	for ( int i = 0; i < set1; i++){
		free(address[i]);
	}
	free(address);


}


void NAssocNAssoc ( FILE *file, int cacheSize1, int assoc1, int block, int cacheSize2, int assoc2, char* policy){
	int set1 = cacheSize1/(block * assoc1);
	int LinePerSet1 = assoc1;
	int set2 = cacheSize2/(block * assoc2);
	int LinePerSet2 = assoc2;
	int offsetBits = log2(block);
	int setBits1 = log2(set1);
	int setBits2 = log2(set2);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long** cache1 = malloc(sizeof(unsigned long long*) * set1);
	for ( int i = 0; i < set1; i++){
		cache1[i] = malloc(sizeof(unsigned long long) * LinePerSet1);
	}
	unsigned long long** cache2 = malloc(sizeof(unsigned long long*) * set2);
	for ( int i = 0; i < set2; i++){
		cache2[i] = malloc(sizeof(unsigned long long) * LinePerSet2);
	}
	unsigned long long** address = malloc(sizeof(unsigned long long*) * set1);
	for ( int i = 0; i < set1; i++){
		address[i] = malloc(sizeof(unsigned long long) * LinePerSet1);
	}
	fill2d(cache1, set1, LinePerSet1);
	fill2d(cache2, set2, LinePerSet2);
	fill2d(address, set1, LinePerSet1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
	while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

		int setNum1 = (hex >> offsetBits) & ( pow2(setBits1) - 1 );
		unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
		int setNum2 = (hex >> offsetBits) & ( pow2(setBits2) - 1 );
		unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
		int i;
		int ref1 = 0; 
		for ( i = 0; i < LinePerSet1; i++){
			if ( cache1[setNum1][i] == tagNum1){
				ref1 = 1;
				break;
			}
		}
		//hit in l1
		if ( ref1 == 1){
			hitL1++;
			if ( c == 'W'){
				memwrites++;
			}
			//lru policy
			if ( policy[0] != 'f'){
				int v;
				int ref6 = 0;
				for ( v = 0; v < LinePerSet1; v++){
					if ( cache1[setNum1][v] == -1){
						ref6 = 1;
						break;
					}
				}

				//there is space in cache
				if ( ref6 == 1){
					for ( int k = i; k < v - 1; k++){
						cache1[setNum1][k] = cache1[setNum1][k+1];
						address[setNum1][k] = address[setNum1][k+1];
					}

					address[setNum1][v - 1] = hex;
					cache1[setNum1][v - 1] = tagNum1;

				}
				//no space
				else {

					//shift everything over
					for ( int m = i; m < LinePerSet1 - 1; m++){
						cache1[setNum1][m] = cache1[setNum1][m+1];
						address[setNum1][m] = address[setNum1][m+1];
					}
					cache1[setNum1][LinePerSet1-1] = tagNum1;
					address[setNum1][LinePerSet1-1] = hex;
					
				}
			}
		}

		//check l2
		else {
			int j;
			int ref2 = 0;
			for ( j = 0; j < LinePerSet2; j++){
				if ( cache2[setNum2][j] == tagNum2){
					ref2 = 1;
					break;
				}
			}
			//in cachel2
			if ( ref2 == 1){
				missL1++;
				hitL2++;
				if ( c == 'W'){
					memwrites++;
				}

				//moving cache2 tag number out and sorting it
				cache2[setNum2][j] = -1;
				for ( int m = j; m < LinePerSet2 -1 ; m++){
					cache2[setNum2][m] = cache2[setNum2][m+1];
				}
				cache2[setNum2][LinePerSet2 - 1] = -1;
				// evicting 1st block from cache1
				unsigned long long tempHex = address[setNum1][0];
				int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
				unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);

				for ( int k = 0; k < LinePerSet1 - 1; k++){
					cache1[setNum1][k] = cache1[setNum1][k+1];
					address[setNum1][k] = address[setNum1][k+1];
				}
				cache1[setNum1][LinePerSet1-1] = tagNum1;
				address[setNum1][LinePerSet1 - 1] = hex;
				//finding first -1 slot to place evicted cahce1 block
				int n;
				int ref3 = 0;
				for ( n = 0; n < LinePerSet2; n++){
					if ( cache2[tempSetNum2][n] == -1){
						ref3 = 1;
						break;
					}
				}
				if ( ref3 == 1){
					//have found empty slot for tagnum
					cache2[tempSetNum2][n] = tempTagNum2;
				} else {
					//cache2 is also filled up, so have to move every array element one slot up
					for ( int k = 0; k < LinePerSet2 - 1; k++){
						cache2[tempSetNum2][k] = cache2[tempSetNum2][k+1];
					}
					cache2[tempSetNum2][LinePerSet2-1] = tempTagNum2;
				}

			}

			//not in either cache
			else {

				//checking for empty space in l1 cache
				int s;
				int ref4 = 0; 
				for ( s = 0; s < LinePerSet1; s++){
					if ( cache1[setNum1][s] == -1){
						ref4 = 1;
						break;
					}
				}
				//empty space in l1 cache
				if ( ref4 == 1){

					missL1++;
					missL2++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
					cache1[setNum1][s] = tagNum1;
					address[setNum1][s] = hex;

				}

				else {

					//no empty space
					unsigned long long tempHex = address[setNum1][0];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					for ( int l = 0; l < LinePerSet1-1; l++){
						cache1[setNum1][l] = cache1[setNum1][l+1];
						address[setNum1][l] = address[setNum1][l+1];
					}
					cache1[setNum1][LinePerSet1-1] = tagNum1;
					address[setNum1][LinePerSet1-1] = hex;
					missL1++;
					missL2++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
					//put evicted block in right spot
					int t;
					int ref5 = 0;
					for ( t = 0; t < LinePerSet2; t++){
						if ( cache2[tempSetNum2][t] == -1){
							ref5 = 1;
							break;
						}
					}

					//there is an empty slot in cache2
					if ( ref5 == 1){
						cache2[tempSetNum2][t] = tempTagNum2;
					} 
					else {
						//move everything one to the left
						for ( int k = 0; k < LinePerSet2 - 1; k++){
							cache2[tempSetNum2][k] = cache2[tempSetNum2][k+1];
						}
						cache2[tempSetNum2][LinePerSet2-1] = tempTagNum2; 
					}
				}



			}

		}

	}

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);

	for ( int i = 0; i < set1; i++){
		free(cache1[i]);
	}
	free(cache1);
	for ( int i = 0; i < set2; i++){
		free(cache2[i]);
	}
	free(cache2);
	for ( int i = 0; i < set1; i++){
		free(address[i]);
	}
	free(address);



}



void AssocDirect ( FILE *file, int cacheSize1, int block, int cacheSize2, char* policy){

	int LinePerSet1 = cacheSize1/block;
	int set2 = cacheSize2/block;
	int offsetBits = log2(block);
	int setBits1 = log2(1);
	int setBits2 = log2(set2);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long* cache1 = malloc(sizeof(unsigned long long) * LinePerSet1);
	unsigned long long* cache2 = malloc(sizeof(unsigned long long) * set2);
	unsigned long long* address = malloc(sizeof(unsigned long long) * LinePerSet1);
	fill(cache1, LinePerSet1); 
	fill(cache2, set2);
	fill(address, LinePerSet1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
	if ( policy[0] == 'f'){
		while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

			unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
			int setNum2 = (hex >> offsetBits) & ( pow2(setBits2) - 1 );
			unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
			int i;
			int ref1 = 0; 
			for ( i = 0; i < LinePerSet1; i++){
				if ( cache1[i] == tagNum1){
					ref1 = 1;
					break;
				}
			} 

			//tag present in first cache
			if ( ref1 == 1){
				hitL1++;
				if ( c == 'W'){
					memwrites++;
				}
			} 
			
			//have to check second cache
			else {
				//if present in second cache
				if ( cache2[setNum2] == tagNum2){
					hitL2++;
					missL1++;
					if ( c == 'W'){
						memwrites++;
					}
					
					// no empty space, have to evict a tag to l2, then add current to cache1

					unsigned long long tempHex = address[0];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);

					for ( int k = 0; k < LinePerSet1-1; k++){
						cache1[k] = cache1[k+1];
						address[k] = address[k+1];
					}
					cache1[LinePerSet1 - 1] = tagNum1;
					address[LinePerSet1 - 1] = hex;
					cache2[tempSetNum2] = tempTagNum2;

					if ( tempSetNum2 != setNum2){
						cache2[setNum2] = -1;
					}

					

				}
				//not present in either cache 
				else {

					//checking for empty space in l1 cache
					int m;
					int ref3 = 0; 
					for ( m = 0; m < LinePerSet1; m++){
						if ( cache1[m] == -1){
							ref3 = 1;
							break;
						}
					}
					//empty space in l1 cache
					if ( ref3 == 1){

						missL1++;
						missL2++;
						memreads++;
						if ( c == 'W'){
							memwrites++;
						}
						cache1[m] = tagNum1;
						address[m] = hex;

					} 
					//no empty space in l1 cache, must evict to l2
					else {
						unsigned long long tempHex = address[0];
						int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
						unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
						for ( int l = 0; l < LinePerSet1-1; l++){
							cache1[l] = cache1[l+1];
							address[l] = address[l+1];
						}
						cache1[LinePerSet1-1] = tagNum1;
						address[LinePerSet1 - 1] = hex;
						cache2[tempSetNum2] = tempTagNum2;
						missL1++;
						missL2++;
						memreads++;
						if ( c == 'W'){
							memwrites++;
						}


					}
				}



			}

		}
	}
	else {
		while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

			unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
			int setNum2 = (hex >> offsetBits) & ( pow2(setBits2) - 1 );
			unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
			int i;
			int ref1 = 0; 
			for ( i = 0; i < LinePerSet1; i++){
				if ( cache1[i] == tagNum1){
					ref1 = 1;
					break;
				}
			}

			//tag present in first cache, have to move tag to correct position in array
			if ( ref1 == 1){
				hitL1++;
				if ( c == 'W'){
					memwrites++;
				}
				//check if there is space in array
				int j;
				int ref2 = 0;
				for ( j = 0; j < LinePerSet1; j++){
					if ( cache1[j] == -1){
						ref2 = 1;
						break;
					}
				}

				//there is space in cache
				if ( ref2 == 1){
					for ( int k = i; k < j - 1; k++){
						cache1[k] = cache1[k+1];
						address[k] = address[k+1];
					}

					address[j - 1] = hex;
					cache1[j - 1] = tagNum1;

				}
				//no space
				else {

					//shift everything over
					for ( int m = i; m < LinePerSet1 - 1; m++){
						cache1[m] = cache1[m+1];
						address[m] = address[m+1];
					}
					cache1[LinePerSet1-1] = tagNum1;
					address[LinePerSet1-1] = hex;
					
				}
			} 
			//not present in L1, checking l2
			else {

				// if it is in cache2
				if ( cache2[setNum2] == tagNum2){

					hitL2++;
					missL1++;
					if ( c == 'W'){
						memwrites++;
					}
					//move l1 tag to l1 cache in proper place
					//l1 cache is filled bc there is stuff in l2 cache
					unsigned long long tempHex = address[0];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);

					for ( int k = 0; k < LinePerSet1-1; k++){
						cache1[k] = cache1[k+1];
						address[k] = address[k+1];
					}
					cache1[LinePerSet1 - 1] = tagNum1;
					address[LinePerSet1 - 1] = hex;
					cache2[tempSetNum2] = tempTagNum2;
					//remove value from setNum2
					if ( tempSetNum2 != setNum2){
						cache2[setNum2] = -1;
					} 
					


				} 
				else {
					//not in either cache
					//checking for empty space in l1 cache
					int m;
					int ref3 = 0; 
					for ( m = 0; m < LinePerSet1; m++){
						if ( cache1[m] == -1){
							ref3 = 1;
							break;
						}
					}
					//empty space in l1 cache
					if ( ref3 == 1){

						missL1++;
						missL2++;
						memreads++;
						if ( c == 'W'){
							memwrites++;
						}
						cache1[m] = tagNum1;
						address[m] = hex;

					}

					// no space in l1 cache, have to evict to l2
					else {
						unsigned long long tempHex = address[0];
						int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
						unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
						for ( int l = 0; l < LinePerSet1-1; l++){
							cache1[l] = cache1[l+1];
							address[l] = address[l+1];
						}
						cache1[LinePerSet1-1] = tagNum1;
						address[LinePerSet1 - 1] = hex;
						cache2[tempSetNum2] = tempTagNum2;
						missL1++;
						missL2++;
						memreads++;
						if ( c == 'W'){
							memwrites++;
						}


					}

				}


			} 

		}

	}

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);

	free(cache1);
	free(cache2);
	free(address);





}

void AssocAssoc ( FILE *file, int cacheSize1, int block, int cacheSize2, char* policy){
	int LinePerSet1 = cacheSize1/block;
	int LinePerSet2 = cacheSize2/block;
	int offsetBits = log2(block);
	int setBits1 = log2(1);
	int setBits2 = log2(1);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long* cache1 = malloc(sizeof(unsigned long long) * LinePerSet1);
	unsigned long long* cache2 = malloc(sizeof(unsigned long long) * LinePerSet2);
	unsigned long long* address = malloc(sizeof(unsigned long long) * LinePerSet1);
	fill(cache1, LinePerSet1);
	fill(cache2, LinePerSet2);
	fill(address, LinePerSet1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
		while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

			unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
			unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
			int i;
			int ref1 = 0; 
			for ( i = 0; i < LinePerSet1; i++){
				if ( cache1[i] == tagNum1){
					ref1 = 1;
					break;
				}
			} 

			//tag present in first cache
			if ( ref1 == 1){
				hitL1++;
				if ( c == 'W'){
					memwrites++;
				}
				if ( policy[0] != 'f'){
					int v;
					int ref6 = 0;
					for ( v = 0; v < LinePerSet1; v++){
						if ( cache1[v] == -1){
							ref6 = 1;
							break;
						}
					}

					//there is space in cache
					if ( ref6 == 1){
						for ( int k = i; k < v - 1; k++){
							cache1[k] = cache1[k+1];
							address[k] = address[k+1];
						}

						address[v - 1] = hex;
						cache1[v - 1] = tagNum1;

					}
					//no space
					else {

						//shift everything over
						for ( int m = i; m < LinePerSet1 - 1; m++){
							cache1[m] = cache1[m+1];
							address[m] = address[m+1];
						}
						cache1[LinePerSet1-1] = tagNum1;
						address[LinePerSet1-1] = hex;
					
					}
				}
			}

			else {
				//checking if in second cache
				int j;
				int ref2 = 0;
				for ( j = 0; j < LinePerSet2; j++){
					if ( cache2[j] == tagNum2){
						ref2 = 1;
						break;
					}
				}

				//is in second cache
				if ( ref2 == 1){
					missL1++;
					hitL2++;
					if ( c == 'W'){
						memwrites++;
					}

					//moving cache2 tag number out and sorting it
					cache2[j] = -1;
					for ( int m = j; m < LinePerSet2 -1 ; m++){
						cache2[m] = cache2[m+1];
					}
					cache2[LinePerSet2 - 1] = -1;
					// evicting 1st block from cache1
					unsigned long long tempHex = address[0];
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);

					for ( int k = 0; k < LinePerSet1 - 1; k++){
						cache1[k] = cache1[k+1];
						address[k] = address[k+1];
					}
					cache1[LinePerSet1-1] = tagNum1;
					address[LinePerSet1 - 1] = hex;
					//finding first -1 slot to place evicted cahce1 block
					int n;
					int ref3 = 0;
					for ( n = 0; n < LinePerSet2; n++){
						if ( cache2[n] == -1){
							ref3 = 1;
							break;
						}
					}
					if ( ref3 == 1){
						//have found empty slot for tagnum
						cache2[n] = tempTagNum2;
					} else {
						//cache2 is also filled up, so have to move every array element one slot up
						for ( int k = 0; k < LinePerSet2 - 1; k++){
							cache2[k] = cache2[k+1];
						}
						cache2[LinePerSet2-1] = tempTagNum2;
					}

				}
				else {
					//not in cache1 or cache2
					//checking for empty space in l1 cache
					int s;
					int ref4 = 0; 
					for ( s = 0; s < LinePerSet1; s++){
						if ( cache1[s] == -1){
							ref4 = 1;
							break;
						}
					}
					//empty space in l1 cache
					if ( ref4 == 1){

						missL1++;
						missL2++;
						memreads++;
						if ( c == 'W'){
							memwrites++;
						}
						cache1[s] = tagNum1;
						address[s] = hex;

					} 
					else {
						//no empty space
						unsigned long long tempHex = address[0];
						unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
						for ( int l = 0; l < LinePerSet1-1; l++){
							cache1[l] = cache1[l+1];
							address[l] = address[l+1];
						}
						cache1[LinePerSet1-1] = tagNum1;
						address[LinePerSet1-1] = hex;
						missL1++;
						missL2++;
						memreads++;
						if ( c == 'W'){
							memwrites++;
						}
						//put evicted block in right spot
						int t;
						int ref5 = 0;
						for ( t = 0; t < LinePerSet2; t++){
							if ( cache2[t] == -1){
								ref5 = 1;
								break;
							}
						}

						//there is an empty slot in cache2
						if ( ref5 == 1){
							cache2[t] = tempTagNum2;
						} else {
							//move everything one to the left
							for ( int k = 0; k < LinePerSet2 - 1; k++){
								cache2[k] = cache2[k+1];
							}
							cache2[LinePerSet2-1] = tempTagNum2; 
						}


					}
				}

				
			} 

		}

	 

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);

	free(cache1);
	free(cache2);
	free(address);





}

void AssocNAssoc ( FILE *file, int cacheSize1, int block, int cacheSize2, int assoc, char* policy){
	int LinePerSet1 = cacheSize1/block;
	int set2 = cacheSize2/(block * assoc);
	int LinePerSet2 = assoc;
	int offsetBits = log2(block);
	int setBits1 = log2(1);
	int setBits2 = log2(set2);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long* cache1 = malloc(sizeof(unsigned long long) * LinePerSet1);
	unsigned long long** cache2 = malloc(sizeof(unsigned long long*) * set2);
	for ( int i = 0; i < set2; i++){
		cache2[i] = malloc(sizeof(unsigned long long) * LinePerSet2);
	}
	unsigned long long* address = malloc(sizeof(unsigned long long) * LinePerSet1);
	fill(cache1, LinePerSet1);
	fill2d(cache2, set2, LinePerSet2);
	fill(address, LinePerSet1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
		while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

			unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
			int setNum2 = (hex >> offsetBits) & ( pow2(setBits2) - 1 );
			unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
			int i;
			int ref1 = 0; 
			for ( i = 0; i < LinePerSet1; i++){
				if ( cache1[i] == tagNum1){
					ref1 = 1;
					break;
				}
			} 

			//tag present in first cache
			if ( ref1 == 1){
				hitL1++;
				if ( c == 'W'){
					memwrites++;
				}
				if ( policy[0] != 'f'){
					int v;
					int ref6 = 0;
					for ( v = 0; v < LinePerSet1; v++){
						if ( cache1[v] == -1){
							ref6 = 1;
							break;
						}
					}

					//there is space in cache
					if ( ref6 == 1){
						for ( int k = i; k < v - 1; k++){
							cache1[k] = cache1[k+1];
							address[k] = address[k+1];
						}

						address[v - 1] = hex;
						cache1[v - 1] = tagNum1;

					}
					//no space
					else {

						//shift everything over
						for ( int m = i; m < LinePerSet1 - 1; m++){
							cache1[m] = cache1[m+1];
							address[m] = address[m+1];
						}
						cache1[LinePerSet1-1] = tagNum1;
						address[LinePerSet1-1] = hex;
					
					}
				}
			}

			else {
				//checking if in second cache
				int j;
				int ref2 = 0;
				for ( j = 0; j < LinePerSet2; j++){
					if ( cache2[setNum2][j] == tagNum2){
						ref2 = 1;
						break;
					}
				}

				//is in second cache
				if ( ref2 == 1){
					missL1++;
					hitL2++;
					if ( c == 'W'){
						memwrites++;
					}

					//moving cache2 tag number out and sorting it
					cache2[setNum2][j] = -1;
					for ( int m = j; m < LinePerSet2 -1 ; m++){
						cache2[setNum2][m] = cache2[setNum2][m+1];
					}
					cache2[setNum2][LinePerSet2 - 1] = -1;
					// evicting 1st block from cache1
					unsigned long long tempHex = address[0];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);

					for ( int k = 0; k < LinePerSet1 - 1; k++){
						cache1[k] = cache1[k+1];
						address[k] = address[k+1];
					}
					cache1[LinePerSet1-1] = tagNum1;
					address[LinePerSet1 - 1] = hex;
					//finding first -1 slot to place evicted cahce1 block
					int n;
					int ref3 = 0;
					for ( n = 0; n < LinePerSet2; n++){
						if ( cache2[tempSetNum2][n] == -1){
							ref3 = 1;
							break;
						}
					}
					if ( ref3 == 1){
						//have found empty slot for tagnum
						cache2[tempSetNum2][n] = tempTagNum2;
					} else {
						//cache2 is also filled up, so have to move every array element one slot up
						for ( int k = 0; k < LinePerSet2 - 1; k++){
							cache2[tempSetNum2][k] = cache2[tempSetNum2][k+1];
						}
						cache2[tempSetNum2][LinePerSet2-1] = tempTagNum2;
					}

				}
				else {
					//not in cache1 or cache2
					//checking for empty space in l1 cache
					int s;
					int ref4 = 0; 
					for ( s = 0; s < LinePerSet1; s++){
						if ( cache1[s] == -1){
							ref4 = 1;
							break;
						}
					}
					//empty space in l1 cache
					if ( ref4 == 1){

						missL1++;
						missL2++;
						memreads++;
						if ( c == 'W'){
							memwrites++;
						}
						cache1[s] = tagNum1;
						address[s] = hex;

					} 
					else {
						//no empty space
						unsigned long long tempHex = address[0];
						int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
						unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
						for ( int l = 0; l < LinePerSet1-1; l++){
							cache1[l] = cache1[l+1];
							address[l] = address[l+1];
						}
						cache1[LinePerSet1-1] = tagNum1;
						address[LinePerSet1-1] = hex;
						missL1++;
						missL2++;
						memreads++;
						if ( c == 'W'){
							memwrites++;
						}
						//put evicted block in right spot
						int t;
						int ref5 = 0;
						for ( t = 0; t < LinePerSet2; t++){
							if ( cache2[tempSetNum2][t] == -1){
								ref5 = 1;
								break;
							}
						}

						//there is an empty slot in cache2
						if ( ref5 == 1){
							cache2[tempSetNum2][t] = tempTagNum2;
						} else {
							//move everything one to the left
							for ( int k = 0; k < LinePerSet2 - 1; k++){
								cache2[tempSetNum2][k] = cache2[tempSetNum2][k+1];
							}
							cache2[tempSetNum2][LinePerSet2-1] = tempTagNum2; 
						}


					}
				}

				
			} 

		}

	 

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);

	free(cache1);
	for ( int i = 0; i < set2; i++){
		free(cache2[i]);
	}
	free(cache2);
	free(address);





}

void DirectDirect( FILE *file, int cacheSize1, int block, int cacheSize2){

	int set1 = cacheSize1/block;
	int set2 = cacheSize2/block;
	int offsetBits = log2(block);
	int setBits1 = log2(set1);
	int setBits2 = log2(set2);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long* cache1 = malloc(sizeof(unsigned long long) * set1); 
	unsigned long long* cache2 = malloc(sizeof(unsigned long long) * set2);
	unsigned long long* address = malloc(sizeof(unsigned long long) * set1);
	fill(cache1, set1); 
	fill(cache2, set2);
	fill(address, set1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
	while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

		//calculate proper set and tag for l1 cache
		int setNum1 = (hex >> offsetBits) & ( pow2(setBits1) - 1 );
		unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
		//calculate proper set and tag for l2 cache
		int setNum2 = (hex >> offsetBits) & ( pow2(setBits2) - 1 );
		unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);

	

		if ( cache1[setNum1] == tagNum1){
			hitL1++;
			if ( c == 'W'){
				memwrites++;
			}



		} else {
			if ( cache2[setNum2] == tagNum2){
				hitL2++;
				missL1++;
				if ( c == 'W'){
					memwrites++;
				}
				if ( cache1[setNum1] != -1 ){
					unsigned long long tempHex = address[setNum1];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					cache2[tempSetNum2] = tempTagNum2;

				} 
				else {
					cache2[setNum2] = -1;

				}
				cache1[setNum1] = tagNum1;
				address[setNum1] = hex;


			} else {

				// getting the current address stored in setNum1 index, using that to recalculate proper index and tag for second cache
				// set index is currently filled
				if ( cache1[setNum1] != -1){

					unsigned long long tempHex = address[setNum1];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					cache2[tempSetNum2] = tempTagNum2;



				} 
				cache1[setNum1] = tagNum1;
				address[setNum1] = hex;
				missL1++;
				missL2++;
				memreads++;
				if ( c == 'W'){
					memwrites++;
				}
				

			}
		}

	}

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);

	free(cache1);
	free(cache2);
	free(address);




}



void DirectAssoc(FILE *file, int cacheSize1, int block, int cacheSize2){

	int set1 = cacheSize1/block;
	int LinePerSet2 = cacheSize2/block;
	int offsetBits = log2(block);
	int setBits1 = log2(set1);
	int setBits2 = log2(1);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long* cache1 = malloc(sizeof(unsigned long long) * set1); 
	unsigned long long* cache2 = malloc(sizeof(unsigned long long) * LinePerSet2);
	unsigned long long* address = malloc(sizeof(unsigned long long) * set1); 
	fill(cache1, set1); 
	fill(cache2, LinePerSet2);
	fill(address, set1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
	while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

		//calculate proper set and tag for l1 cache
		int setNum1 = (hex >> offsetBits) & ( pow2(setBits1) - 1 );
		unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
		
		unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
		// if tag is in cache1
		if ( cache1[setNum1] == tagNum1){
			hitL1++;
			if ( c == 'W'){
				memwrites++;
			}



		} else {

			//checking if tag is in cache2
			int i;
			int ref1 = 0;
			for ( i = 0; i < LinePerSet2; i++){
				if ( cache2[i] == tagNum2){
					ref1 = 1;
					break;
				}
			}

			//if tag is in cache2
			if ( ref1 == 1){
				hitL2++;
				missL1++;
				if ( c == 'W'){
					memwrites++;
				}
				//moving cache2 tag number out and sorting it
				cache2[i] = -1;
				for ( int m = i; m < LinePerSet2 -1 ; m++){
					cache2[m] = cache2[m+1];
				}
				cache2[LinePerSet2 - 1] = -1;
			

				//if cache1 slot was filled up must evict to correct slot in cache2
				if ( cache1[setNum1] != -1){
					unsigned long long tempHex = address[setNum1];
					
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					int j;
					int ref2 = 0;
					//finding first -1 slot to place evicted cahce1 block
					for ( j = 0; j < LinePerSet2; j++){
						if ( cache2[j] == -1){
							ref2 = 1;
							break;
						}
					}
					if ( ref2 == 1){
						//have found empty slot for tagnum
						cache2[j] = tempTagNum2;
					} else {
						//cache2 is also filled up, so have to move every array element one slot up
						for ( int k = 0; k < LinePerSet2 - 1; k++){
							cache2[k] = cache2[k+1];
						}
						cache2[LinePerSet2-1] = tempTagNum2;
					}



				} 
				

				cache1[setNum1] = tagNum1;
				address[setNum1] = hex;
				
			} else {

				//tag is not in cache1 or cache2;
				//if slot is filled up, must push to l2
				if ( cache1[setNum1] != -1){

					unsigned long long tempHex = address[setNum1];
					
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					int l;
					int ref3 = 0;
					for ( l = 0; l < LinePerSet2; l++){
						if ( cache2[l] == -1){
							ref3 = 1;
							break;
						}
					}
					//there is an empty slot in cache2
					if ( ref3 == 1){
						cache2[l] = tempTagNum2;
					} else {
						//move everything one to the left
						for ( int k = 0; k < LinePerSet2 - 1; k++){
							cache2[k] = cache2[k+1];
						}
						cache2[LinePerSet2-1] = tempTagNum2; 
					}


				}
				//set tag for cache1 and update address array
				cache1[setNum1] = tagNum1;
				address[setNum1] = hex;
				missL1++;
				missL2++;
				memreads++;
				if ( c == 'W'){
					memwrites++;
				} 





			}

		}

	}

	free(cache1);
	free(address);
	free(cache2);

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);
	


}



void DirectNAssoc(FILE *file, int cacheSize1, int block, int cacheSize2, int assoc){

	int set1 = cacheSize1/block;
	int set2 = cacheSize2/(block * assoc);
	int LinePerSet2 = assoc;
	int offsetBits = log2(block);
	int setBits1 = log2(set1);
	int setBits2 = log2(set2);
	int tagBits1 = 48 - offsetBits - setBits1;
	int tagBits2 = 48 - offsetBits - setBits2;
	unsigned long long* cache1 = malloc(sizeof(unsigned long long) * set1); 
	unsigned long long** cache2 = malloc(sizeof(unsigned long long*) * set2);
	for ( int i = 0; i < set2; i++){
		cache2[i] = malloc(sizeof(unsigned long long) * LinePerSet2);
	}
	unsigned long long* address = malloc(sizeof(unsigned long long) * set1);
	fill(cache1, set1); 
	fill2d(cache2, set2, LinePerSet2);
	fill(address, set1);
	char c;
	unsigned long long hex;
	int memreads = 0;
	int memwrites = 0;
	int hitL1 = 0;
	int missL1 = 0;
	int hitL2 = 0;
	int missL2 = 0;
	while ( fscanf(file, "%c %llx\n", &c, &hex) != EOF){

		//calculate proper set and tag for l1 cache
		int setNum1 = (hex >> offsetBits) & ( pow2(setBits1) - 1 );
		unsigned long long tagNum1 = (hex >> (offsetBits + setBits1)) & (pow2(tagBits1) - 1);
		//calculate proper set and tag for l2 cache
		int setNum2 = (hex >> offsetBits) & ( pow2(setBits2) - 1 );
		unsigned long long tagNum2 = (hex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
		// if tag is in cache1
		if ( cache1[setNum1] == tagNum1){
			hitL1++;
			if ( c == 'W'){
				memwrites++;
			}



		} else {

			//checking if tag is in cache2
			int i;
			int ref1 = 0;
			for ( i = 0; i < LinePerSet2; i++){
				if ( cache2[setNum2][i] == tagNum2){
					ref1 = 1;
					break;
				}
			}

			//if tag is in cache2
			if ( ref1 == 1){
				hitL2++;
				missL1++;
				if ( c == 'W'){
					memwrites++;
				}
				//moving cache2 tag number out and sorting it
				cache2[setNum2][i] = -1;
				for ( int m = i; m < LinePerSet2 -1 ; m++){
					cache2[setNum2][m] = cache2[setNum2][m+1];
				}
				cache2[setNum2][LinePerSet2 - 1] = -1;
			

				//if cache1 slot was filled up must evict to correct slot in cache2
				if ( cache1[setNum1] != -1){
					unsigned long long tempHex = address[setNum1];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					int j;
					int ref2 = 0;
					//finding first -1 slot to place evicted cahce1 block
					for ( j = 0; j < LinePerSet2; j++){
						if ( cache2[tempSetNum2][j] == -1){
							ref2 = 1;
							break;
						}
					}
					if ( ref2 == 1){
						//have found empty slot for tagnum
						cache2[tempSetNum2][j] = tempTagNum2;
					} else {
						//cache2 is also filled up, so have to move every array element one slot up
						for ( int k = 0; k < LinePerSet2 - 1; k++){
							cache2[tempSetNum2][k] = cache2[tempSetNum2][k+1];
						}
						cache2[tempSetNum2][LinePerSet2-1] = tempTagNum2;
					}



				} 
				

				cache1[setNum1] = tagNum1;
				address[setNum1] = hex;
				
			} else {

				//tag is not in cache1 or cache2;
				//if slot is filled up, must push to l2
				if ( cache1[setNum1] != -1){

					unsigned long long tempHex = address[setNum1];
					int tempSetNum2 = (tempHex >> offsetBits) & ( pow2(setBits2) - 1 );
					unsigned long long tempTagNum2 = (tempHex >> (offsetBits + setBits2)) & (pow2(tagBits2) - 1);
					int l;
					int ref3 = 0;
					for ( l = 0; l < LinePerSet2; l++){
						if ( cache2[tempSetNum2][l] == -1){
							ref3 = 1;
							break;
						}
					}
					//there is an empty slot in cache2
					if ( ref3 == 1){
						cache2[tempSetNum2][l] = tempTagNum2;
					} else {
						//move everything one to the left
						for ( int k = 0; k < LinePerSet2 - 1; k++){
							cache2[tempSetNum2][k] = cache2[tempSetNum2][k+1];
						}
						cache2[tempSetNum2][LinePerSet2-1] = tempTagNum2; 
					}


				}
				//set tag for cache1 and update address array
				cache1[setNum1] = tagNum1;
				address[setNum1] = hex;
				missL1++;
				missL2++;
				memreads++;
				if ( c == 'W'){
					memwrites++;
				} 





			}

		}

	}

	free(cache1);
	free(address);
	for ( int i = 0; i < set2; i++){
		free(cache2[i]);
	}
	free(cache2);

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("l1cachehit:%d\n", hitL1);
	printf("l1cachemiss:%d\n", missL1);
	printf("l2cachehit:%d\n", hitL2);
	printf("l2cachemiss:%d\n", missL2);

		


}


void fill( unsigned long long* arr, int num){
	for ( int i = 0; i < num; i++){
		arr[i] = -1; 
	}
	return;
}

void fill2d( unsigned long long** arr, int x, int y){
	for ( int i = 0; i < x; i++){
		for ( int j = 0; j < y; j++){
			arr[i][j] = -1;
		}
	}
	return;
}




int checkPow2(int x) {
 
  for(int i = 0; i < 31; i++) {

    if (x == 1 << i){

    	return 1;
     
    }
  }
  return 0; 
}

unsigned long long pow2(unsigned long x){
	unsigned long i = 1;
	for ( int k = 0; k < x; k++){
		i = i*2;
	}
	return i;
}