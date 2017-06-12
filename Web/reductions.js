//-----------------------------------------------------------------------------
//reductions.js
//Purpose: This program houses the functions needed to reduce an
//         assembly word and write out the proper reductions
//         to an HTML document
//Author: Ryan Arredondo
//Date: 12.28.2011
//Update: 04.20.2012 - Fixed bug in sequences(w) function.
//						Possible that it could have gone out of subscript.
//					- Update to reduction algorithm since old one did not
//						recognize exact nesting index for all words
//----------------------------------------------------------------------------

////main function - called from reductions user interface
function perform_reduction(){
	var word_form = document.getElementById("WordInput");
	var output_form = document.getElementById("reductions");
	var circular_check = document.getElementById("circular");

	//assigns text entry value to word
    var word = word_form.value   
    if(word.match(",")!= null){     
	//word is seperated by commas - split word at commas
	    word = word.split(",");
	}
	else{  
	/*word is not seperated by commas
	split words at each char*/
	    word = word.split("");
	}
    for(var i = 0; i < word.length; i++){
	//converts chars into ints
	word[i] = parseInt(word[i]);   
    }

	//blanks output
    output_form.innerHTML = "";

    //checks if word is double occurrence
    if(!is_double_occurrence(word)){  
	//gives message that word is not double occurrence
	write(word.join("") + " is not a double occurrence word!");
    }
    else{  //reduce word
	
	//checks reductions type
	var type = "tau"

	var reductions = new Array();	

	//checks if circular is checked
	if(circular_check.checked){ 
		//reduce word and circular equivalencies
		
		if(word.length === 0){
			write("The empty word has no circular word equivalencies!");
			reductions.push(get_NI(word, type));
		}
		else{
	    	var equivalent_words = get_equivalencies(word);
	    	for(var i = 0; i < equivalent_words.length; i++){
				//reductions passed to get nesting index of word
				reductions.push(get_NI(equivalent_words[i], type));  
	    	}
		}
	}
	else{       //just reduce word
		//reductions passed to get nesting index of word
	    reductions.push(get_NI(word, type));  
	}
	
	//gets nesting index and writes it
	var min_NI = 0;
	if(reductions.length != 1){
		min_NI = reductions[0];
		for(i = 1; i < reductions.length; i++){
			if(reductions[i] < min_NI){min_NI = reductions[i]}
		}
	}
	else{min_NI = reductions[0]}
	write("<br/><br/><big><b>The nesting index is: " + min_NI + "</b></big>");   
    }
}

////function for to output to "reductions" div
function write(string){
	var output_form = document.getElementById("reductions");
    output_form.innerHTML += string;
}

////function to output reduction
function write_reduction(word_obj){
	var hist = word_obj.history;
	var path = word_obj.path;

	write("{");
	for(var i = 0; i < hist.length; i++){
		if(hist[i].length >= 20){
			write("[" + hist[i].join(",") + "], ");
		}
		else{
			write(hist[i].join("") + ", ");
		}
	}
	write("&epsilon;} <b>obtained by reduction operations:<b/> ");

	write(path.join(", "));
	write("</BR>" + "\n");
}	

////////////////////////////////////////////////////////////////////////////////////////

////Word object constructor
//a word object contains its history of reduction,
//the path by which the history of reduction is obtained,
//its word value, whether or not it's empty
function word_obj(word, history, path){
	this.value = word;
	this.history = history;
	this.path = path;
	this.isEmpty = (word.length === 0);
}

////step function
//Note: passed variable word is a word object defined above
//performs a step in the computation of the nesting index
//returns: word with A and B type seqs removed from w and words
//	with any letter not in A or B type sequence removed from w
function step(word, type){
	if(!type){var type = "tau"}

	var AB_list = new Array();
	var Drop_list = new Array();
	var Return_list = new Array();

	AB_list = tau_sequences(word.value);

	//Creates list of letters, not in AB seqs, to be dropped
	if(AB_list.length != 0){
		var alph_of_w = alpha(word.value);
		var alph_len = alph_of_w.length;
		var letter_in_seq = false;

		//checks if letters are in A or B type seq
		for(var i = 0; i < alph_len; i++){ //iterates over letters in word
			var letter_in_seq = false;
			var letter = alph_of_w[i];

			for(var j = 0; j < AB_list.length; j++){ //iterates over seqs in AB_list
				var seq = AB_list[j];
				for(var k = 0; k < seq.length; k++){ //iterates over letters in seq
					if(letter == seq[k]){letter_in_seq = true}
				}
			}
			if(!letter_in_seq){ //push letter to drop list
				Drop_list.push(letter);
			}
		}
		//Here we drop AB seqs and add derived word to return_list
		var w_drop_AB = relabel(remove_sequences(word.value, AB_list));
		var w_hist = new Array().concat(word.history);
		var w_path = new Array().concat(word.path);

		w_hist.push(word.value);
		w_path.push("1");

		var w_drop_AB_obj = new word_obj(w_drop_AB, w_hist, w_path);
		Return_list.push(new word_obj(w_drop_AB, w_hist, w_path));
	}
	else{ //every letter in word goes to drop list
		Drop_list = alpha(word.value);
	}
	
	//this gets words with letter from drop_list removed
	var Drop_len = Drop_list.length;
	if(Drop_len != 0){
		for(var i = 0; i < Drop_len; i++){
			var letter = Drop_list[i];
			var w_drop_letter = relabel(remove(word.value, letter));
			var w_hist = new Array().concat(word.history);
			var w_path = new Array().concat(word.path);

			w_hist.push(word.value);
			w_path.push("2 (removal of  " + letter + ")");

			Return_list.push(new word_obj(w_drop_letter, w_hist, w_path));
		}
	}

	return Return_list;
}

////get_NI function
//function returns nesting index of a word
function get_NI(word, type){
	if(!type){var type = "tau"}
	var NI = 0;
	var current_words = new Array();
	var emptyWordNotInCurrent = true;
	var start_word = new word_obj(word, new Array(), new Array());

	if(start_word.isEmpty){
		//no reduction to do; NI = 0
		return NI;
	}

	current_words = step(start_word, type);
	NI = NI + 1;

	while(emptyWordNotInCurrent){

		var current_len = current_words.length

		//checks if any current_word is empty
		for(var i = 0; i < current_len; i++){
			var current_word = current_words[i];

			if(current_word.isEmpty){
				emptyWordNotInCurrent = false;
				
				//outputs reduction
				write_reduction(current_word);
			}
		}

		if(emptyWordNotInCurrent){ //preps next_words for next iteration
			var next_words = new Array();
			
			//increment NI
			NI = NI + 1;

			for(var i = 0; i < current_len; i++){ //iterates over words in current_words
				next_words = next_words.concat(step(current_words[i], type));
			}
		
			/*Step complete*/
			//Current words become next_words
			current_words = next_words;
		}
	}
	return NI;
}

////////////////////////////////////////////////////////////////////////////////////////////

////get_equivalencies function
//A function to get the circular word equivalencies of an assembly word
function get_equivalencies(word){
    var equivalencies = new Array();

    //gets equivalencies
    for(var i = 0; i < word.length; i++){
	word.unshift(word.pop());  //puts the last element in first position
	var relabelled_word = relabel(word);
	equivalencies.push(relabelled_word);
    }
    //removes duplicates
    for(i = 0; i < equivalencies.length; i++){
	for(var j = i + 1; j < equivalencies.length; j++){
	    if(equal(equivalencies[i], equivalencies[j])){   //j-th element matches i-th element
		equivalencies.splice(j, 1);         //removes j-th element
		j--;
	    }
	}
    }
    return equivalencies;
}

////equal function
//compares two arrays by their elements;
//if each element of array1 matches each element of array2 with similar ordering
//returns true; otherwise returns false;
function equal(array1, array2){
    if(array1.length != array2.length){
		return false;
    }
    else{
		for(var i = 0; i < array1.length; i++){
	    	if(array1[i] != array2[i]){
				return false;
	    	}
		}
		return true;
    }
}


////alpha function
//argument passed:a double occurence word w
//returns: alphabet used to construct w

function alpha(word){
    var alphabet = new Array();

    for(var i = 0; i < word.length; i++){
		var inAlphabet = false;

		for(var j = 0; j < alphabet.length; j++){
	    	if(alphabet[j] == word[i]){inAlphabet = true}
		}
		if(!inAlphabet){
			alphabet.push(word[i]);
		}
    }
    return alphabet;
}

////occurrences function
//Arguments passed: A word w and a letter in w
//Returns: the indices a letter in w
function occurrences(word, letter){
    var occs = new Array();
    for(var i = 0; i < word.length; i++){
		if(word[i] == letter){occs.push(i)}
    }
    return occs;
}

////is double occurrence function
//arguments passed: a word w
//returns: true if w is double occurrence and false otherwise
function is_double_occurrence(word){
    if((word.length)%2 == 1){  /*Word is not of even length*/
		return false;
    }
    else if(word.length == 2 && word[0] == word[1]){  /*Word is a loop*/
		return true;
    }
    else{
	var sorted_word = new Array().concat(word);
	sorted_word.sort();
	for(var i = 0; (2*i) < word.length; i++){
	    var j = 2*i;
	    if(sorted_word[j] != sorted_word[j+1] || sorted_word[j]==sorted_word[j-1]){
			return false;
	    }
	}
	return true;
    }
}

////sequences function
//argument passed: a double occurence word w
//returns: type A and type B sequences in w
function sequences(word){
    var seqs = new Array()  /*Array of arrays, holds sequences found*/
    var alph_of_w = alpha(word)
    //loops over the letters of w
    for(var i = 0; i < alph_of_w.length; i++){
		var n = 0;
		var occ1 = occurrences(word, alph_of_w[i])[0];
		var occ2 = occurrences(word, alph_of_w[i])[1];

		//checks for seqs of type A
		while(word[occ1 + n] == word[occ2 - n]){
		    if(((occ2 - n) - (occ1 + n)) == 1){ /*type A seq found*/
				var seq = new Array();
				for(var j = occ1; j <= occ2; j++){
				    seq.push(word[j]);
				}
				seqs.push(seq);  /*Adds seq to seqs*/
				break;
		    }
		    else{
			n = n + 1;
		    }
		}
		//checks for seqs of type B
		n = 0;
    	if((occ2 + (occ2 - occ1 - 1)) >= word.length || word.length == 2){   
			/*Word beginning with w[i] cannot be type B seq*/
			continue;
    	}
		do{
	    	n = n + 1; /*n=1 to prevent a loop from being double counted as A and B*/
	    	if(((occ2 - (occ1 + n)) == 1) && ((word[occ1 + n] == word[occ2 + n]))){
				var seq = new Array();
				for(var j = occ1; j <= (occ2+n); j++){
		    		seq.push(word[j]);
				}		
				seqs.push(seq);  /*Adds seq to seqs*/
				break;
	    	}
		}while(word[occ1 + n] == word[occ2 + n]);
    }
    return seqs;
}

////tau_sequences function
//arg passed: an assembly word w
//returns maximal subwords of w using sequences(w)
function tau_sequences(word){
    var seqs = sequences(word);
    var tau_seqs = new Array();
    for(var i = 0; i < seqs.length; i++){
		var seq = seqs[i];
		var occ1 = occurrences(word,seq[0])[0];
		var occ2 = occurrences(word,seq[0])[1];
		if(occ1 == 0 || occ2 == (word.length-1)){  /*sequence is maximal subword*/
		    tau_seqs.push(seq);
		}
		else if (seq[0] == seq[seq.length - 1]){ /***then sequence is of type A***/
		    if(word[occ1 - 1] != word[occ2 + 1]){  /*sequence is maximal subword*/
				tau_seqs.push(seq);
		    }
		}
		else{  /***sequence is of type B***/
		    tau_seqs.push(seq);  /*B type seqs can't be non-maximal*/
		}
    }
    return tau_seqs;
}

////sigma_sequences function
//arg passed: an assembly word w
//returns maximal length subwords of w using sequences(w)
function sigma_sequences(word){
    var seqs = sequences(word);
    var sig_seqs = new Array();
    var max_length = seqs[0].length;
    for(var i = 1; i < seqs.length; i++){
		if(seqs[i].length > max_length){max_length = seqs[i].length}
    }
    for(i = 0; i < seqs.length; i++){
		if(seqs[i].length == max_length){sig_seqs.push(seqs[i])}
    }
    return sig_seqs;
}

////remove_sequences function
//args passed: an assembly word w and a set of a subwords of w
//returns the word obtained from the removal of subwords from w
function remove_sequences(fixed_word, seqs){
    var word = new Array().concat(fixed_word);

    for(var i = 0; i < seqs.length; i++){
		var letters = alpha(seqs[i]);

		for(var j = 0; j < letters.length; j++){
		    var occs = occurrences(word, letters[j]);
		    word.splice(occs[1],1);   /*Occs[1] first so that it doesn't mess up occs[0]*/
		    word.splice(occs[0],1);
		}
    }
    return word;
}

////remove function
//args passed: an assembly word w and a letter of that assembly word
//returns word obtained from removing letter passed from word passed
function remove(word, letter){
    var occ1 = occurrences(word, letter)[0], occ2 = occurrences(word, letter)[1];
    var new_word = new Array().concat(word);
    new_word.splice(occ2,1);
    new_word.splice(occ1,1);
    return new_word;
}

////relabel function
//args passed: an assembly word w
//returns the word obtained by relabelling w in ascending order
function relabel(word){
    var new_word = new Array().concat(word);
    var old_letters = alpha(word);
    var new_letters = new Array();
    for(var i = 1; i <= (word.length / 2); i++){  /*gathers new letters*/ 
		new_letters.push(i);
    }
    for(i = 0; i < word.length; i++){
		for(var j = i+1; j < word.length; j++){
		    if (word[i] == word[j]){
				new_word[i] = new_letters[0];
				new_word[j] = new_letters.shift();
		    }
		}
    }
    return new_word;
}
