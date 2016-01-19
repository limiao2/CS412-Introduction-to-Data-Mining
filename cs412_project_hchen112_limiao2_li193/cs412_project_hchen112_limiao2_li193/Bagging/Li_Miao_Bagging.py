# Author: Li Miao
# CS412
# Bagging algorithm
import pandas as pd
import numpy as np
import random
from Bagging import Bagging
from sklearn import preprocessing
from sklearn.tree import DecisionTreeClassifier 
from utilities import shuffle_in_unison

# Bootstrap procedure for resampling
def shuffle_in_unison(a, b):
    '''
    bootstrap x and y
    '''
    rng_state = np.random.get_state()
    np.random.shuffle(a)
    np.random.set_state(rng_state)
    np.random.shuffle(b)

# Bagging class to implement bagging with decision trees
class Bagging(object):
    def __init__(self,ntrees = 500,bootstrap = 0.9):
        '''
        ntrees: number of trees in the bagging
        bootstrap: how to bootstrap the whole dataset
        '''
        self.ntrees = ntrees
        self.bootstrap = bootstrap
        self.bag = []
        
    def fit(self,x,y):
        n = len(y)
        subn = round(n*self.bootstrap)
        
        for i in xrange(self.ntrees):
            shuffle_in_unison(x,y)
            xsub = x[:subn]
            ysub = y[:subn]
            
            tree = DecisionTreeClassifier(criterion = 'entropy')
            tree.fit(xsub,ysub)
            self.bag.append(tree)
    
    def predictprob(self,test):
        n = test.shape[0]
        finalpred = np.empty(n)
        predeachtime = np.empty([self.ntrees,n])
        for i in xrange(self.ntrees):
            predeachtime[i] = self.bag[i].predict_proba(test)[:,1]
        
        for i in xrange(n):
            finalpred[i] = np.mean(predeachtime[:,i])
            
        return finalpred
        


# Reading in train and test data
train = pd.DataFrame(pd.read_csv('revised_train1.csv'))
test = pd.DataFrame(pd.read_csv('revised_test1.csv'))
bidder = pd.read_csv('bidder.csv')

# Data Preprocessing
# Convert categorical variables to numeric variables
number = preprocessing.LabelEncoder()
train['payment_account_prefix_same_as_address_prefix'] = number.fit_transform(train.payment_account_prefix_same_as_address_prefix)
newtrain = train
X = newtrain.drop('outcome',1)
Y = newtrain['outcome']
Y= np.array(Y)
X= np.array(X)

# Call bagging functions
bag = Bagging()

# Fit in the train data
bag.fit(X,Y)

# Predict the test data
out = bag.predictprob(test)
# Write out
bidder['prediction'] = out
bidder.to_csv("baggingoutput.csv",index = False)






