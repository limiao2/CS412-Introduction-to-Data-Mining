This readme file provides you a brief description of the contents of this directory.

--Data:

topic-i.txt, i=0,..,4
	 Input file for frequent pattern mining algorithms
	 format: term1_index term2_index term3_index ...
	 Columns are separated by blank.

vocab.txt           
	Dictionary that maps term index to term
	format: term_index    term
	Columns are separated by Tab

--Source code
	featureGenerator.py: generate arff files for Weka

=======
Additional files: These files are for your understanding of how we generate topic-i.txt files.
	paper_raw.txt: raw data of paper titles
	paper.txt    : paper titles after removing stop words, changing to lower case and lemmatization