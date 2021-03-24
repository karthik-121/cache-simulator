
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int checkPow2(int x);
unsigned long pow2(unsigned long x);
void fill( unsigned long* arr, int num);
void DirectMap( FILE *file, int cacheSize, int block);
void fullyAssoc(FILE *file, int cacheSize, int block, char* policy);
void nAssoc(FILE *file, int cacheSize, int block, int assoc, char* policy);
void fill2d( unsigned long** arr, int x, int y);
int main ( int argc, char* argv[argc+1]){

	// checking for 6 args
	if ( argc != 6 ){
		printf("error\n");
		return EXIT_SUCCESS;
	}

	// checking if cache size and block size are powers of 2
	int cache = atoi(argv[1]);
	int block = atoi(argv[4]);
	if ( checkPow2(block) == 0 || checkPow2(cache) == 0){
		printf("error\n");
		return EXIT_SUCCESS;
	}

	// checking if file exists
	FILE *file = fopen(argv[5], "r");
	if ( file == 0){
		printf("error\n");
		return EXIT_SUCCESS;
	} else {
		char* assoc = argv[2];
		if (assoc[0] == 'd'){
			DirectMap(file, cache, block);
			fclose(file);
		} else {
			char* policy = argv[3];
			if ( assoc[5] == '\0'){
				fullyAssoc(file, cache, block, policy);
			} else {

				int x = atoi(&(argv[2][6]));
				nAssoc(file, cache, block, x, policy);
			}
			
			fclose(file);
		}



		return EXIT_SUCCESS;
	
	}

	


}

void nAssoc(FILE *file, int cacheSize, int block, int assoc, char* policy){

	int sets = cacheSize/(block*assoc);
	int LinePerSet = assoc;
	int offsetBits = log2(block);
	int setBits = log2(sets);
	unsigned long tagBits = 48 - offsetBits - setBits;
	unsigned long** arr = malloc(sizeof(unsigned long*) * sets);
	for ( int i = 0; i < sets; i++){
		arr[i] = malloc(sizeof(unsigned long) * LinePerSet);
	}
	fill2d(arr, sets, LinePerSet);
	char c;
	unsigned long hex;
	int memreads = 0;
	int memwrites = 0;
	int hit = 0;
	int miss = 0;
	if ( policy[0] == 'f'){
		while ( fscanf(file, "%c %lx\n", &c, &hex) != EOF){
			int setNum = (hex >> offsetBits) & ( pow2(setBits) - 1 );
			unsigned long tagNum = (hex >> (offsetBits + setBits)) & (pow2(tagBits) - 1);
			int i;
			int j;
			int ref1 = 0;
			int ref2 = 0;
			for ( i = 0; i < LinePerSet; i++){
				if ( arr[setNum][i] == -1){
					ref1 = 1;
					break;
				}
			}
			if ( ref1 == 0){
				//full cache and FIFO policy

				//checking if address is already present
				for ( j = 0; j < LinePerSet; j++){
					if ( arr[setNum][j] == tagNum){
						ref2 = 1;
						break;
					}
				} 

				//if address is not present
				if ( ref2 == 0){
					for ( int k = 0; k < LinePerSet-1; k++){
						arr[setNum][k] = arr[setNum][k+1];
					}

					arr[setNum][LinePerSet-1] = tagNum;
					miss++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
				} else {

					hit++;
					if ( c == 'W'){
						memwrites++;
					}
				}


			} else {

				//checking if address is already present
				for ( j = 0; j < LinePerSet; j++){
					if ( arr[setNum][j] == tagNum){
						ref2 = 1;
						break;
					}
				}
				//if address is not present
				if ( ref2 == 0){
					arr[setNum][i] = tagNum;
					miss++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
				}  else {

					// if it is present
					hit++;
					if ( c == 'W'){
						memwrites++;
					}

				}
			}


		}

	}
	else {

		while ( fscanf(file, "%c %lx\n", &c, &hex) != EOF){

			int setNum = (hex >> offsetBits) & ( pow2(setBits) - 1 );
			unsigned long tagNum = (hex >> (offsetBits + setBits)) & (pow2(tagBits) - 1);
			int i;
			int j;
			int ref1 = 0;
			int ref2 = 0;

			for ( i = 0; i < LinePerSet; i++){
				if ( arr[setNum][i] == -1){
					ref1 = 1;
					break;
				}
			}

			if ( ref1 == 0){

				//full cache and LRU policy

				//checking if address is already present
				for ( j = 0; j < LinePerSet; j++){
					if ( arr[setNum][j] == tagNum){
						ref2 = 1;
						break;
					}
				}

				//if address is not present

				if ( ref2 == 0){
					for ( int k = 0; k < LinePerSet-1; k++){
						arr[setNum][k] = arr[setNum][k+1];
					}

					arr[setNum][LinePerSet-1] = tagNum;
					miss++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
				} else {

					hit++;
					unsigned long temp = arr[setNum][j];
					for ( int m = j; m < LinePerSet-1; m++){
						arr[setNum][m] = arr[setNum][m+1];
					}
					arr[setNum][LinePerSet-1] = temp;
					if ( c == 'W'){
						memwrites++;
					}


				}




			} else {

				for ( j = 0; j < LinePerSet; j++){
					if ( arr[setNum][j] == tagNum){
						ref2 = 1;
						break;
					}
				}

				if ( ref2 == 0){
					arr[setNum][i] = tagNum;
					miss++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
				} else {
					// if it is present use LRU


					hit++;
					unsigned long temp = arr[setNum][j];
					for ( int t = j; t < i - 1; t++){
						arr[setNum][t] = arr[setNum][t+1];
					}
					arr[setNum][i - 1] = temp;
					if ( c == 'W'){
						memwrites++;
					}
				}


			}


		}

	}

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("cachehit:%d\n", hit);
	printf("cachemiss:%d\n", miss);

	for ( int i = 0; i < sets; i++){
		free(arr[i]);
	}
	free(arr);




}

void fullyAssoc(FILE *file, int cacheSize, int block, char* policy){
	int sets = 1;
	int LinePerSet = cacheSize/block;
	int offsetBits = log2(block);
	int setBits = log2(sets);
	unsigned long tagBits = 48 - offsetBits - setBits;
	unsigned long* arr = malloc(sizeof(unsigned long ) * LinePerSet);
	fill(arr, LinePerSet);
	char c;
	unsigned long hex;
	int memreads = 0;
	int memwrites = 0;
	int hit = 0;
	int miss = 0;
	if ( policy[0] == 'f'){
		while ( fscanf(file, "%c %lx\n", &c, &hex) != EOF){
			unsigned long tagNum = (hex >> (offsetBits + setBits)) & (pow2(tagBits) - 1);
			int i;
			int j;
			int ref1 = 0;
			int ref2 = 0;
			for ( i = 0; i < LinePerSet; i++){
				if ( arr[i] == -1){
					ref1 = 1;
					break;
				}
			}

			if ( ref1 == 0){
				//full cache and FIFO policy

				//checking if address is already present
				for ( j = 0; j < LinePerSet; j++){
					if ( arr[j] == tagNum){
						ref2 = 1;
						break;
					}
				} 

				//if address is not present
				if ( ref2 == 0){
					for ( int k = 0; k < LinePerSet-1; k++){
						arr[k] = arr[k+1];
					}

					arr[LinePerSet-1] = tagNum;
					miss++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
				} else {

					hit++;
					if ( c == 'W'){
						memwrites++;
					}
				}


			} else {

				//checking if address is already present
				for ( j = 0; j < LinePerSet; j++){
					if ( arr[j] == tagNum){
						ref2 = 1;
						break;
					}
				}
				//if address is not present
				if ( ref2 == 0){
					arr[i] = tagNum;
					miss++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
				}  else {

					// if it is present
					hit++;
					if ( c == 'W'){
						memwrites++;
					}

				}
			}
		}
	} 

	else {

		while ( fscanf(file, "%c %lx\n", &c, &hex) != EOF){
			unsigned long tagNum = (hex >> (offsetBits + setBits)) & (pow2(tagBits) - 1);
			int i;
			int j;
			int ref1 = 0;
			int ref2 = 0;

			for ( i = 0; i < LinePerSet; i++){
				if ( arr[i] == -1){
					ref1 = 1;
					break;
				}
			}
			if ( ref1 == 0){

				//full cache and LRU policy

				//checking if address is already present
				for ( j = 0; j < LinePerSet; j++){
					if ( arr[j] == tagNum){
						ref2 = 1;
						break;
					}
				}

				//if address is not present

				if ( ref2 == 0){
					for ( int k = 0; k < LinePerSet-1; k++){
						arr[k] = arr[k+1];
					}

					arr[LinePerSet-1] = tagNum;
					miss++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
				} else {

					hit++;
					unsigned long temp = arr[j];
					for ( int m = j; m < LinePerSet-1; m++){
						arr[m] = arr[m+1];
					}
					arr[LinePerSet-1] = temp;
					if ( c == 'W'){
						memwrites++;
					}


				}







			} else {

				for ( j = 0; j < LinePerSet; j++){
					if ( arr[j] == tagNum){
						ref2 = 1;
						break;
					}
				}

				// if address is not present

				if ( ref2 == 0){
					arr[i] = tagNum;
					miss++;
					memreads++;
					if ( c == 'W'){
						memwrites++;
					}
				} else {
					// if it is present use LRU


					hit++;
					unsigned long temp = arr[j];
					for ( int t = j; t < i - 1; t++){
						arr[t] = arr[t+1];
					}
					arr[i - 1] = temp;
					if ( c == 'W'){
						memwrites++;
					}
				}

			}

		}



	}

	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("cachehit:%d\n", hit);
	printf("cachemiss:%d\n", miss);

	free(arr);




}

void DirectMap( FILE *file, int cacheSize, int block){

	int sets = cacheSize/block;
	int offsetBits = log2(block);
	int setBits = log2(sets);
	int tagBits = 48 - offsetBits - setBits;
	unsigned long* arr = malloc(sizeof(unsigned long) * sets);
	fill(arr, sets);
	char c;
	unsigned long hex;
	int memreads = 0;
	int memwrites = 0;
	int hit = 0;
	int miss = 0;
	while ( fscanf(file, "%c %lx\n", &c, &hex) != EOF){

		int setNum = (hex >> offsetBits) & ( pow2(setBits) - 1 );
		unsigned long tagNum = (hex >> (offsetBits + setBits)) & (pow2(tagBits) - 1);
		
		if ( arr[setNum] == tagNum){
			hit++;
			if ( c == 'W'){
				memwrites++;
			}
		} else {
			arr[setNum] = tagNum;
			miss++;
			memreads++;
			if ( c == 'W'){
				memwrites++;
			}

		}
	

	}
	printf("memread:%d\n", memreads);
	printf("memwrite:%d\n", memwrites);
	printf("cachehit:%d\n", hit);
	printf("cachemiss:%d\n", miss);

	free(arr);


}

void fill( unsigned long* arr, int num){
	for ( int i = 0; i < num; i++){
		arr[i] = -1; 
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

unsigned long pow2(unsigned long x){
	unsigned long i = 1;
	for ( int k = 0; k < x; k++){
		i = i*2;
	}
	return i;
}

void fill2d( unsigned long** arr, int x, int y){
	for ( int i = 0; i < x; i++){
		for ( int j = 0; j < y; j++){
			arr[i][j] = -1;
		}
	}
	return;
}
