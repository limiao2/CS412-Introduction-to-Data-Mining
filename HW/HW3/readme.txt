1. For step one, I implement the Apriori frequent pattern mining algorithm. In this python code, I implement 6 functions: readinfile, frequentitemset,join,apriori,readinvoc and writeout.
    The readinfile function read data from topic files, return transaction list, unique itemset and a general itemset includes all items.
    The frequentItemset function will return frequent itemset satisfy minimum support
    The join function will join candidates who satisfy minimum support
    The apriori function will return list L which includes from L1, L2... to Lk. And also related frequency.
    The readinvoc function will return a list with all of the vocabularies
    The writeout function will return a txt file which includes the frequency and name of frequent itemset which satisfy the minimum support
    
2. For mining closed patterns, we directly work on our result from step 1. I created a function named closed to find the closed itemset from step one, and use the same writeout function to return a txt file closed-i.txt

3. For mining max patterns, we directly work on our result from step 1. I created a function named findmax to find the max itemset from step one, and use the same writeout function to return a txt file max-i.txt

