// reductions.c
// ----------------------------------------------------------------------------
// Purpose: This program computes the Nesting Index of a double occurrence word
//          either specified explicitly on the command line or in a list of
//          words from a text file that is specified on the command line. For a
//          formal definition of the Nesting Index see documentation in:
//          http://arxiv.org/abs/1311.3543 or
//          http://scholarcommons.usf.edu/etd/4979/.
// ----------------------------------------------------------------------------
// Author: Ryan Arredondo
// E-mail: ryan.c.arredondo@gmail.com
// Date: 12.28.2011
// ----------------------------------------------------------------------------
// Update: 04.20.2012 - Fixed bug in sequences(w) function. Possible that it
//                      could have gone out of index.
//		      - Update to reduction algorithm since old one did not
//		        recognize exact nesting index for all words.
// Update: 12.31.2016 - Some cleanup
// ----------------------------------------------------------------------------
// Command Line Arguments:
// -h or --help:  Prints a usage message.
// -t or --text:  For input of white-space delimited double occurrence words
//                from a text file. Optionally include the name of an output
//                text file.
// -c or --count: Uses input of white-space delimited double occurrence words
//                from a text file, presents a summary of counts on the number
//                of double occurrence words recognizing a certain nesting
//                index.
// -i or --isos:  For each word algorithm also considers all words that are
//                cyclically equivalent. Program will print each word in the
//                equivalence class as well as its Nesting Index
// ----------------------------------------------------------------------------
// To compile, run:
// >> make    (assuming Makefile is present)
// or
// >> gcc -lm -Wall NestIndex.c -o NestIndex
// ----------------------------------------------------------------------------


// Standard C libs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>	//contains isdigit function
#include <math.h>	//contains pow function


// Function templates
unsigned short ** step(unsigned short *, int, int *, int *);
int get_NI(unsigned short *, int);
unsigned short * get_letters(unsigned short *, int);
int * occurrences(unsigned short *, int, unsigned short);
short is_double_occurrence(unsigned short *, int);
unsigned short ** sequences(unsigned short *, int, int *);
unsigned short ** get_repeat_return_words(unsigned short *, int, int *);
unsigned short * remove_seqs(unsigned short *, int, unsigned short **, int, int *);
short is_in_seq(short, unsigned short **, int);
unsigned short * remove_ltr(unsigned short *, int, unsigned short);
unsigned short * relabel(unsigned short *, int);
void print_word(unsigned short *, int, short);
void file_print_word(FILE *, unsigned short *, int, short);
unsigned short * get_word(char *, int *);
void copy_words(unsigned short **, int *, unsigned short **, int *, int *);
void usage_message();
unsigned short * get_reverse(unsigned short *, int);
unsigned short ** get_isomorphisms(unsigned short *, int, int *);


//// main function
// Parses information on command line, gets nesting index of specified
// double occurrence words (DOWs) and prints result to console or outfile.
int main(int argc, char * argv[])
{
    unsigned short ** isomorphisms;
    unsigned short * word;
    char word_string[256];
    int counts[20];
    int NI = 0, size = 0, i = 0, bufferchar = 0, count = 0;
    FILE * InFile = NULL, * OutFile = NULL;
	
    for(i = 0; i < 20; i++) counts[i] = 0;
    for(i = 0; i < 256; i++) word_string[i] = (char) 0;
	
    if(argc < 2 || argc > 4){  // Too little or too many arguments
	usage_message();
    }
    else if(argc == 2){	// Input is direct word or help is desired
	if(!(strncmp(argv[1], "-h", 2)) || !(strncmp(argv[1], "--help", 6)))
	    usage_message();
	else{
	    size = strlen(argv[1]);
	    // Converts chars to ints (shorts)
	    for(i = 0; i < size; i++){
		if(!isdigit(argv[1][i]) && !ispunct(argv[1][i])){
		    printf("Argument for word was not recognized \r\n");
		    usage_message();
		}
	    }
	    word = get_word(argv[1], &size);

	    NI = get_NI(word, size);
	    print_word(word, size, 0);  // 0 means don't print \r\n
	    if(NI == -1) printf(": not DOW \r\n");
	    else printf(": %d \r\n", NI);
	    free(word);
	    return 0;
	}
    }
    else if(argc == 3){
	if(!(strncmp(argv[1], "-t",2)) || !(strncmp(argv[1], "--text", 6)) || \
	   !(strncmp(argv[1], "-c", 2)) || !(strncmp(argv[1], "--count", 7))){	
	    // Text file input
	    InFile = fopen(argv[2], "r");
	}
	else if(!(strncmp(argv[1], "-i", 2)) || !(strncmp(argv[1], "--isos", 6))){
	    size = strlen(argv[2]);
	    // Converts chars to ints (shorts)
	    for(i = 0; i < size; i++){
		if(!isdigit(argv[2][i]) && !ispunct(argv[2][i])){
		    printf("Argument for word was not recognized \r\n");
		    usage_message();
		}
	    }
	    word = get_word(argv[2], &size);
			
	    isomorphisms = get_isomorphisms(word, size, &count);	
	    for(i = 0; i < count; i++){
		NI = get_NI(isomorphisms[i], size);
		print_word(isomorphisms[i], size, 0);
		printf(": %d\r\n", NI);
		free(isomorphisms[i]);
	    }
	    free(isomorphisms);
	    free(word);
	    return 0;
	}
	else{
	    printf("Error interpreting input \r\n");
	    usage_message();
	}
    }
    else if(argc == 4){
	if(!(strncmp(argv[1], "-t",2)) || !(strncmp(argv[1], "--text", 6))){
	    InFile = fopen(argv[2], "r");
	    OutFile = fopen(argv[3], "w");
	}
	else{
	    printf("Error interpreting input \r\n");
	    usage_message();
	}
    }
    // Check that files specified are good
    if(InFile == NULL){
	printf("Couldn't open file: %s \r\n", argv[2]);
	exit(1);
    }
    if(argc == 4 && OutFile == NULL){
	printf("Couldn't open file: %s \r\n", argv[3]);
	exit(1);
    }
    // Gets to first number in file
    do{
	bufferchar = fgetc(InFile);
    } while(!isdigit(bufferchar) && bufferchar != EOF);

    // Goes till end of file
    while(bufferchar != EOF){
	size = 0;
	// Gets word and puts to word_string
	while(!isspace(bufferchar) && bufferchar != EOF){
	    word_string[size++] = (char) bufferchar;
	    bufferchar = fgetc(InFile);
	}
	word = get_word(word_string, &size);
	NI = get_NI(word, size);

	// Outputs word and nesting index
	if(NI != 0){
	    if(!(strncmp(argv[1], "-c", 2)) || !(strncmp(argv[1], "--count", 7))){
		if(NI > sizeof(counts)/sizeof(counts[0])){
		    printf("'-c' cannot be used for words with such large NI\\r\n");
		    printf("Try altering code to resize counts array");
		    exit(1);
                }
		counts[NI - 1] += 1;
	    }
	    else{
		if(OutFile == NULL){	// Print to console
		    print_word(word, size, 0);
		    printf(": %d\r\n", NI);
		}
		else{	// Print to file
		    file_print_word(OutFile, word, size, 0);
		    fprintf(OutFile, ": %d\r\n", NI);
		}
	    }
	}
	free(word);
	if(bufferchar != EOF) bufferchar = fgetc(InFile);
    }
    if(!(strncmp(argv[1], "-c", 2)) || !(strncmp(argv[1], "--count", 7))){
	for(i = 0; i < 20; i++){
	    if(counts[i] != 0)
		printf("NI = %d: %d\r\n", i+1, counts[i]);
	}
    }
    fclose(InFile);
    if(OutFile != NULL) fclose(OutFile);
    return 0;
}


//// usage_message function
// Prints a message to the user demonstrating how program should be used
void usage_message(){
    printf("Try: \r\n\t./NestIndex 123321 or ./NestIndex 1,2,3,3,2,1 \r\n");
    printf("\t where 123321 and 1,2,3,3,2,1 can be replaced by any double  \r\n");
    printf("\t occurrence word (DOW) over the set of natural numbers\r\n\r\n");
    printf("For text file input use: \r\n\t");
    printf("./NestIndex -t Infile.txt [Outfile.txt]\r\n\r\n");
    printf("To get a frequency of recognized nesting indices use: \r\n\t");
    printf("./NestIndex -c Infile.txt [Outfile.txt]\r\n\r\n");
    printf("To consider the class of cyclically equivalent words use: \r\n\t");
    printf("./NestIndex -i 123321\r\n\r\n");
    exit(0);
}


//// get_word function
// Given string and a pointer to an int (size), returns representation of
// word in string as an array of unsigned shorts and updates size with the
// length of the returned array.
unsigned short * get_word(char * str_arg, int * size)
{
    int new_size = 0, i = 0, punct_ctr = 0;
    char * token = NULL;
    unsigned short * word = NULL;
    short isdelimited = 0;
	
    for(i = 0; i < *size && !isdelimited; i++)
	isdelimited = ispunct(str_arg[i]);

    if(!isdelimited){
	word = (unsigned short *) malloc(sizeof(unsigned short)*(*size));
	if(word == NULL){
	    printf("Memory could not be alloc'd for word(1)");
	    exit(1);
	}
	// Converts chars to ints
	for(i = 0; i < *size; i++)
	    word[i] = (unsigned short) (str_arg[i] - '0');
    }	
    else{
	// Counts delimiters so that correct amount of mem can be alloc'd
	for(i = 0; i < *size; i++)
	    if(ispunct(str_arg[i])) punct_ctr++;
	word = (unsigned short *) calloc((punct_ctr + 1), sizeof(unsigned short));
	if(word == NULL){
	    printf("Memory could not be alloc'd for word(2)");
	    exit(1);
	}
	// Word is written letter by letter by tokenizing each letter in str_arg
	new_size = 0;
	token = strtok(str_arg, ",-.!#$%&'*+/");
	do{
	    // Parses token, makes conversion digit by digit
	    for(i = 0; i < strlen(token); i++){
		word[new_size] += (unsigned short) \
		    pow(10, (strlen(token)-i-1)) * (token[i] - '0');
	    }
	    new_size++;
	    // Prepares for next letter
	    token = strtok(NULL, ",-.!#$%&'*+/");
	}while(token != NULL && new_size - 1 != punct_ctr);
	*size = new_size;
    }
    return word;
}



//// step function - performs one reduction step
// Given a DOW word, its size, and two pointers to ints (count and sizes),
// returns an array of words obtained from either operation 1: removing maximal
// subwords from word and or from operation 2: removing a of letter of word not
// contained in a maximal subword. Recall from http://arxiv.org/abs/1311.3543
// that a maximal subword is repeat word or return word that contains no other
// repeat word or return word as a subword. Function updates count with number
// of words in returned array and sizes with the size of each of the those words.
unsigned short ** step(unsigned short * word, int size, int * count, int * sizes)
{
    unsigned short ** reduction_list = NULL, ** return_list = NULL;
    unsigned short * letters = NULL;
    unsigned short drop_list[size/2];
    int seq_count = 0, drop_ctr = 0, i = 0, n = 0;
    int new_size = 0;	// Variable gets size of first word in return_list
    // Only important size since rest will be size - 2
	
    letters = get_letters(word, size);
    *count = 0;  // Number of words returned

    // If size <= 4, step results in empty word, regardless of DOW given
    if(size <= 4){
	free(letters);
	return NULL;
    }
    reduction_list = get_repeat_return_words(word, size, &seq_count);

    // Creates list of letters, not in repeat/return word, to be dropped
    if(reduction_list != NULL){	
	// Checks if letters are in repeat/return word
	for(i = 0; i < size/2; i++){		
	    if(!is_in_seq(letters[i], reduction_list, seq_count))
		drop_list[drop_ctr++] = letters[i];
	}
	*count = drop_ctr + 1; // +1 for word - reduction_list
    }
    else{ // Every letter in word goes to drop list
	for(i = 0; i < size/2; i++)
	    drop_list[i] = letters[i];
	*count = size/2;	// count = # of distinct letters in word
    }
    free(letters);

    // Allocates mem for return_list
    return_list = (unsigned short **) \
	malloc(sizeof(unsigned short *)*(*count));
    if(return_list == NULL) {
	for(i = 0; i < seq_count; i++) free(reduction_list[i]);
	free(reduction_list);
	free(word);
	printf("Memory could not be alloc'd for return_list");
	exit(1);
    }
    // Return list gets word with seqs removed in first position
    if(reduction_list != NULL){
	return_list[0] = remove_seqs(word, size, reduction_list,
				     seq_count, &new_size);
	return_list[0] = relabel(return_list[0], new_size);
	if(new_size == 0){
	    // Frees allocated memory
	    free(return_list[0]);
	    free(return_list);
	    for(i = 0; i < seq_count; i++) free(reduction_list[i]);
	    free(reduction_list);
	    return NULL;
	}
	sizes[0] = new_size;

	// Frees memory allocated for seqs
	for(i = 0; i < seq_count; i++)
	    free(reduction_list[i]);
	free(reduction_list);
    }
    // This gets words with letter from drop_list removed
    n = 0;
    for(i = (reduction_list == NULL)? 0: 1; i < *count; i++){
	return_list[i] = remove_ltr(word, size, drop_list[n++]);
	return_list[i] = relabel(return_list[i], size - 2);
	sizes[i] = size - 2;
    }
    return return_list;
}

//// get_NI function
// Given a word and its size, returns nesting index of word
int get_NI(unsigned short * word, int size)
{
    unsigned short ** current_words = NULL, ** next_words = NULL,
	** step_words = NULL;
    int * step_sizes = NULL, * next_sizes = NULL, * current_sizes = NULL;
    int sizes_sum = 0;
    int step_count = 0, current_count = 0, next_count = 0;
    int i = 0, j = 0, NI = 0;
		
    // Checks if word is double occurrence
    if(!is_double_occurrence(word, size)) return -1;

    // Handles case if word is empty word
    NI = 0;
    if(size == 0) return NI;
	
    step_sizes = (int *) malloc(sizeof(int)*(size/2));
    if(step_sizes == NULL) {
	free(word);
	printf("Memory could not be alloc'd for step_sizes");
	exit(1);
    }
    current_count = 0;
    step_words = step(word, size, &current_count, step_sizes);
    NI++;
    if(step_words == NULL){	// First step gives empty word
	free(step_sizes);
	return NI;
    }
    current_words = (unsigned short **) malloc(sizeof(unsigned short *)*(current_count));
    current_sizes = (int *) malloc(sizeof(int)*(current_count));
    if(current_sizes == NULL || current_words == NULL){
	free(step_sizes);
	free(word);
	if(current_sizes != NULL) free(current_sizes);
	if(current_words != NULL) free(current_words);
	printf("Memory could not be alloc'd for current_sizes/current_words");
	exit(1);
    }
    copy_words(current_words, current_sizes, step_words, step_sizes,
	       &current_count);
    free(step_words);
    step_words = NULL;

    // Runs while there is no empty empty word
    while(1){
	NI++;
	// Gets upper bound on # of branch points from current words for memory
	// allocation
	sizes_sum = 0;
	for(i = 0; i < current_count; i++)
	    sizes_sum += current_sizes[i];
	sizes_sum /= 2;
	next_words = malloc(sizeof(unsigned short *)*sizes_sum);
	next_sizes = malloc(sizeof(int)*sizes_sum);
	if(next_sizes == NULL || next_words == NULL){
	    free(step_sizes);
	    free(step_words);
	    for(i = 0; i < current_count; i++) free(current_words[i]);
	    free(current_words);
	    if(next_sizes != NULL) free(next_sizes);
	    if(next_words != NULL) free(next_words);
	    free(word);
	    printf("Memory could not be alloc'd for next_sizes/next_words");
	    exit(1);
	}
	next_count = 0;
	for(i = 0; i < current_count; i++){ // Iterates over words in current_words
	    step_count = 0;
	    step_words = step(current_words[i], current_sizes[i], &step_count,
	                      step_sizes);
	    if(step_words == NULL){
		for(j = 0; j < current_count; j++) free(current_words[j]);
		free(current_words);
		free(current_sizes);
		for(j = 0; j < next_count; j++) free(next_words[j]);
		free(next_words);
		free(next_sizes);
		free(step_sizes);
		return NI;
	    }
			
	    memcpy((next_words + next_count), step_words, 
		   sizeof(unsigned short *)*(step_count));
	    memcpy((next_sizes + next_count), step_sizes,
		   sizeof(int)*(step_count));
	    next_count += step_count;

	    // Contents of step_words not freed because contents (addresses)
	    // were just copied to next_words hence, freeing them would cause
	    // TROUBLE!!
	    free(step_words);
	    step_words = NULL;
	}
	for(i = 0; i < current_count; i++) free(current_words[i]);
	free(current_words);
	current_words = NULL;
	free(current_sizes);
	current_sizes = NULL;

	// Step complete
	// Current words become next_words
	current_words = (unsigned short **) \
	    malloc(sizeof(unsigned short *)*(next_count));
	current_sizes = (int *) malloc(sizeof(int)*(next_count));
	if(current_words == NULL || current_sizes == NULL){
	    free(step_words);
	    free(step_sizes);
	    free(next_sizes);
	    for(i = 0; i < next_count; i++) free(next_words[i]);
	    if(current_words != NULL) free(current_words);
	    if(current_sizes != NULL) free(current_sizes);
	    free(word);
	    printf("Memory could not be alloc'd for current_words/current_sizes");
	    exit(1);
	}
	copy_words(current_words, current_sizes, next_words, next_sizes,
	           &next_count);
	current_count = next_count;
	free(next_words);
	free(next_sizes);
    }
}

//// copy_words function
// Copies words from source to destination without copying duplicates
void copy_words(unsigned short ** dest, int * dest_sizes,
		unsigned short ** source, int * source_sizes, int * count)
{
    int i = 0, j = 0, ctr = 0;
    short isInDest = 0;
	
    for(i = 0; i < *count; i++){
	isInDest = 0;
	for(j = 0; j < ctr && !(isInDest); j++){
	    if(source_sizes[i] == dest_sizes[j]){	// Could be a match
		isInDest = \
		    (memcmp(source[i], dest[j], sizeof(short)*(dest_sizes[j])) == 0);
	    }
	}
	if(!isInDest){	// Add seq to dest
	    j *= 2;
	    memcpy((dest + ctr), (source + i), sizeof(short *));
	    dest_sizes[ctr] = source_sizes[i];
	    ctr++;
	}
	else free(source[i]);
    }
    *count = ctr;
}


//// get_letters function
// Given DOW and its size, returns an array of the letters (which are ints)
// in that word.
unsigned short * get_letters(unsigned short * word, int size)
{
    unsigned short * letters = calloc(size, sizeof(unsigned short));
    short notInAlphabet = 0;
    short letter_ctr = 0;
    int i = 0, j =0;
	
    if(letters == NULL){
	free(word);
	printf("mem could not be alloc'd for letters");
	exit(1);
    }
    for(i = 0; i < size; i++){
	notInAlphabet = 1;
	for(j = 0; j < letter_ctr; j++){
	    if(word[i] == letters[j]) notInAlphabet = 0;
	}
	if(notInAlphabet)
	    letters[letter_ctr++] = word[i];
    }
    return letters;
}

//// occurrences function
// Given a word w, its size and a letter (int) in w, returns indices of each
// occurence of that letter in w.
int * occurrences(unsigned short * w, int size, unsigned short letter)
{
    int * occs = (int *) malloc(sizeof(int)*2);
    int n = 0, i = 0;
	
    for(i = 0; i < size; i++){
	if(w[i] == letter){
	    occs[n++] = i;
	}			
    }
    return occs;
}

//// is_double_occurrence function
// Given a word w and its size, returns 1 if w is double occurrence, else 0
short is_double_occurrence(unsigned short * word, int size)
{
    int i = 0, j = 0;
    short ltr_ctr = 0;
    unsigned short * letters;
	
    letters = get_letters(word, size);
    for(i = 0; i < size/2; i++){	// Iterates over ltr in letters
	ltr_ctr = 0;
	// Iterates over short in word and gets count of ltr in word
	for(j = 0; j < size; j++){
	    if (letters[i] == word[j]) ltr_ctr++;
	}
	if(ltr_ctr != 2){
	    free(letters);
	    return 0;
	}
    }
    free(letters);
    return 1;
}


//// get_repeat_return_words function
// Given a DOW w, its size and a pointer to an int len,returns maximal subwords
// of w using sequences(w) and updates len with the number of maximal subwords
// found.
unsigned short ** get_repeat_return_words(unsigned short * w, int size, int * len)
{
    unsigned short ** subwords = (unsigned short **) \
	malloc(sizeof(unsigned short *)*(size/2));
    short diff[size - 1];
    int i = 0, count = 0;
    short state = 0, place = 0;
	
    if(subwords == NULL){
	free(w);
	printf("Memory could not be alloc'd for subwords");
	exit(1);
    }
    *len = 0;
	
    // Algorithm uses diff = w[i+1] - w[i] for i in 0:size.
    // Note that if diff == [1 1 ... 1 0 -1 ... -1 -1] word is return word
    // and if diff == [1 1 ... 1 -n 1 ... 1 1] word is repeat word
    for(i = 0; i < size - 1; i++){
	diff[i] = (w[i+1] - w[i]);
    }
	
    // Checks for return words
    state = 0;
    place = 0;
    count = 0;
    for(i = 0; i < size - 1; i++){
	switch(diff[i]) {
	case 0:    // Expecting -1
	    if(state == 1 && i != size - 2) state = 2;
	    else{	// Must be a loop
		// Allocate space for loop
		subwords[*len] = (unsigned short *) malloc(sizeof(short)*2);
		if(subwords[*len] == NULL){	
		    for(i = 0; i < *len; i++) free(subwords[i]);
		    free(subwords);
		    free(w);
		    printf("Memory could not be alloc'd for loop");
		    exit(1);
		}
		memcpy(subwords[*len], (w + i), sizeof(short)*2);
		*len += 1;
		state = 0;
		count = 0;
	    }
	    break;
	case 1:
	    if(state == 0) place = i;  // Holds placement of first 1
	    else if(state == 2){
		place += count;
		subwords[*len] = (unsigned short *) malloc(sizeof(short)*((i - place)+ 1));
		if(subwords[*len] == NULL){
		    for(i = 0; i < *len; i++) free(subwords[i]);
		    free(subwords);
		    free(w);
		    printf("Memory could not be alloc'd for return word (1)");
		    exit(1);
		}
		memcpy(subwords[*len], (w + place), sizeof(short)*((i - place) + 1));
		*len += 1;
		place = i;
		count = 0;
	    }					
	    state = 1;	// Expecting 0
	    count++;
	    break;
	case -1:
	    if(state == 2){
		count--;
		if(count == 0 || (i == size - 2)){	// Return word found
		    if(i == size - 2)	place += count;
		    subwords[*len] = (unsigned short *) \
			malloc(sizeof(short)*((i - place) + 2));
		    if(subwords[*len] == NULL){	
			for(i = 0; i < *len; i++) free(subwords[i]);
			free(subwords);
			free(w);
			printf("Memory could not be alloc'd for return word (2)");
			exit(1);
		    }
		    memcpy(subwords[*len], (w + place), \
			   sizeof(short)*((i - place) + 2));
		    *len += 1;
		    state = 0;
		    count = 0;
		}
	    }
	    else{
		state = 0;
		count = 0;
	    }
	    break;
	default:
	    if(state == 2){	// Return word found
		place += count;
		subwords[*len] = (unsigned short *) \
		    malloc(sizeof(short)*((i - place) + 1));
		if(subwords[*len] == NULL){	
		    for(i = 0; i < *len; i++) free(subwords[i]);
		    free(subwords);
		    free(w);
		    printf("Memory could not be alloc'd for return word (3)");
		    exit(1);
		}
		memcpy(subwords[*len], (w + place), \
		       sizeof(short)*((i - place) + 1));
		*len += 1;
	    }		
	    state = 0;
	    count = 0;
	}
    }
	
    // Checks for repeat words
    state = 0;
    place = 0;
    count = 0;
    for(i = 0; i < size - 1; i++){
	switch(diff[i]) {
	case 1:
	    if(state == 0){
		state = 1;
		place = i;
		count++;
	    }
	    else if(state == 1){
		count++;
	    }
	    else if(state == 2){
		count--;
		if(count == 0){	// Repeat word found
		    subwords[*len] = (unsigned short *) \
			malloc(sizeof(short)*((i - place) + 2));
		    if(subwords[*len] == NULL){
			for(i = 0; i < *len; i++) free(subwords[i]);
			free(subwords);
			free(w);
			printf("Memory could not be alloc'd for return word");
			exit(1);
		    }
		    memcpy(subwords[*len], (w + place), \
			   sizeof(short)*((i - place) + 2));
		    *len += 1;
		    state = 0;
		    count = 0;					
		}
	    }
	    break;
	default:
	    if(state == 1){
		if(diff[i] < 0 && (-1)*count <= diff[i]){
		    place += (count + diff[i]);
		    count = (-1)*diff[i];	// Number of 1s expected
		    state = 2;
		}
		else{
		    state = 0;
		    count = 0;
		}
	    }
	    else{
		state = 0;
		count = 0;
	    }
	}
    }
    if(*len == 0){
	free(subwords);
	return NULL;
    }
    return subwords;
}


//// remove_sequences function
// Given assembly word w and a set of a subwords of w returns w - subwords,
// i.e., word obtained from w after removing subwords
unsigned short * remove_seqs(unsigned short * word, int size, 
			     unsigned short ** seqs, int seq_count, int * new_size)
{
    unsigned short * new_word = (unsigned short *) \
	malloc(sizeof(unsigned short)*(size));
    int i = 0;
	
    if(new_word == NULL){
	free(word);
	printf("Memory could not be alloc'd for new_word (remove_seqs)");
	exit(1);
    }
    *new_size = 0;

    for(i = 0; i < size; i++){
	if(!is_in_seq(word[i], seqs, seq_count)){
	    new_word[*new_size] = word[i];
	    *new_size += 1;
	}
    }
    return new_word;
}

//// is_in_seq function
// Given a letter and an array of words (seqs), with the number of subwords,
// returns 1 if letter appears in one of the subwords, else 1
short is_in_seq(short letter, unsigned short ** seqs, int seq_count)
{
    unsigned short * seq;
    int i, j;

    for(i = 0; i < seq_count; i++){
	seq = seqs[i];
	j = 0;
	do{
	    if(letter == seq[j++]) return 1;
	} while(seq[j] > seq[j-1]);
    }
    return 0;
}


//// remove_ltr function
// Given a DOW w, its size and a letter, returns word obtained from w after
// removing letter
unsigned short * remove_ltr(unsigned short * word, int size, unsigned short letter)
{
    unsigned short * new_word = (unsigned short *) \
	malloc(sizeof(unsigned short)*(size - 2));	// -2 for removed letter
    int i = 0, new_size = 0;
	
    if(new_word == NULL){
	free(word);
	printf("Memory could not be alloc'd for new_word (remove_ltr)");	
	exit(1);
    }
    i=0;
    while (word[i] != letter){
	new_word[new_size++] = word[i++];
    }
    i = size-1;
    while (word[i] != letter){
	new_word[i-2] = word[i];
	i--;
    }
    i--;
    while (word[i] != letter){
	new_word[new_size++] = word[i--];
    }
    return new_word;
}

////relabel function
// Given a DOW w and its size, returns the word obtained from w after relabelling
unsigned short * relabel(unsigned short * word, int size)
{
    unsigned short * new_word = (unsigned short *) \
	malloc(sizeof(unsigned short)*size);
    unsigned short new_ltr = 1;
    int i, j;
	
    if(new_word == NULL){
	free(word);
	printf("Memory could not be alloc'd for new_word (relabel)");
	exit(1);
    }
    for(i = 0; i < size; i++){
	for(j = i+1; j < size; j++){
	    if (word[i] == word[j]){
		new_word[i] = new_ltr;
		new_word[j] = new_ltr++;
	    }
	}
    }
    free(word);
    return new_word;
}


//// print_word function
// Prints word with or with comma delimitation depending on length of word,
// i.e., if word has letter >= 10. If return_bool is true, ends print statement
// with newline characters.
void print_word(unsigned short * word, int size, short return_bool)
{
    int i = 0;
    if(size >= 20){
	for(i = 0; i < size - 1; i++)
	    printf("%u,", word[i]);
	printf("%u", word[size - 1]);
    }
    else{
	for(i = 0; i < size; i++)
	    printf("%u", word[i]);
    }
    if(return_bool)	printf("\r\n");
}


//// file_print_word function
// same as print_word function but prints to file 
void file_print_word(FILE * file, unsigned short * word, int size, short return_bool)
{
    int i = 0;
    if(size >= 20){
	for(i = 0; i < size - 1; i++)
	    fprintf(file, "%u,", word[i]);
	fprintf(file, "%u", word[size - 1]);
    }
    else{
	for(i = 0; i < size; i++)
	    fprintf(file, "%u", word[i]);
    }
    if(return_bool)	fprintf(file, "\r\n");
}

//// get_reverse function
// Given a word and its size returns the reverse of that word.
unsigned short * get_reverse(unsigned short * word, int size)
{
    unsigned short * reverse = (unsigned short *) malloc(sizeof(short)*(size));
    int i = 0;

    if(reverse == NULL){
	free(word);
	printf("mem could not be alloc'd for reverse");
	exit(1);
    }
    for(i = 0; i < size; i++)
	reverse[i] = word[(size - 1) - i];
	
    reverse = relabel(reverse, size);
    return reverse;
}


//// get_isomorphisms functions
// Given a word, its size and a pointer to an int count, returns array of
// cyclically equivalent words and updates count with the number of words
// in returned array.
unsigned short ** get_isomorphisms(unsigned short * word, int size, int * count){
    unsigned short ** isomorphisms;
    unsigned short * temp_word, * reverse;
    int i = 0, offset;
    short isDup = 0;
	
    *count = 0;
    isomorphisms = (unsigned short **) malloc(sizeof(short *)*(size));
    temp_word = (unsigned short *) malloc(sizeof(short)*size);
	
    if(isomorphisms == NULL || temp_word == NULL){
	free(word);
	if(isomorphisms != NULL) free(isomorphisms);
	if(temp_word != NULL) free(temp_word);
	printf("mem could not be alloc'd for isomorphisms");
	exit(1);
    }
    offset = 0;
    isDup = 0;
    while(offset < size && !isDup){
	for(i = 0; i + offset < size; i++)
	    temp_word[i] = word[i + offset];
		
	for(i = 0; i < offset; i++)
	    temp_word[(size - offset) + i] = word[i];
		
	temp_word = relabel(temp_word, size);
		
	isDup = 0;
	for(i = 0; i < *count && !isDup; i++)
	    isDup = (memcmp(isomorphisms[i], temp_word, sizeof(short)*size) == 0);
	if(!isDup){
	    isomorphisms[*count] = temp_word;
	    *count += 1;
	    temp_word = malloc(sizeof(short)*(size));
	    if(temp_word == NULL){
		for(i = 0; i < *count; i++) free(isomorphisms[i]);
		free(isomorphisms);
		free(word);
		printf("Memory could not be alloc'd for temp_word (1)");
		exit(1);
	    }
	    offset++;
	}
    }
	
    // Same as above, only with reverse.
    reverse = get_reverse(word, size);
    offset = 0;
    isDup = 0;
    while(offset < size && !isDup){
	for(i = 0; i + offset < size; i++)
	    temp_word[i] = reverse[i + offset];
		
	for(i = 0; i < offset; i++)
	    temp_word[(size - offset) + i] = reverse[i];
		
	temp_word = relabel(temp_word, size);
		
	isDup = 0;
	for(i = 0; i < *count && !isDup; i++)
	    isDup = (memcmp(isomorphisms[i], temp_word, sizeof(short)*size) == 0);
	if(!isDup){
	    isomorphisms[*count] = temp_word;
	    *count += 1;
	    temp_word = malloc(sizeof(short)*(size));
	    if(temp_word == NULL){
		for(i = 0; i < *count; i++) free(isomorphisms[i]);
		free(isomorphisms);
		free(reverse);
		free(word);
		printf("Memory could not be alloc'd for temp_word (2)");
		exit(1);
	    }
	    offset++;
	}
    }
    free(reverse);
    free(temp_word);	
    return isomorphisms;
}
