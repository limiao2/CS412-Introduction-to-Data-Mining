
# coding: utf-8

# In[1]:

from sklearn import tree
from sklearn.utils import resample
import math 
class myRF:
    forest = []
    def __init__(self, treeNum = 1000):
        for i in range(treeNum):
            self.treeNum = treeNum
            self.forest.append(tree.DecisionTreeClassifier(max_features = 'auto',criterion = 'entropy'))
    def fit(self, dataSet):
        for clt in self.forest:
            randSet= resample(dataSet)
            #print "randSet size = %d" % len(randSet)
            target = [x[0] for x in randSet]
            train = [x[1:] for x in randSet]
            clt.fit(train, target)
    
    def predict(self, testSet):
        output = 0
        for clt in self.forest:
            output = output + clt.predict(testSet)[0]
        if output < len(self.forest)/2:
            return 0
        else:
            return 1

    def predictprob(self,test):
        n = test.shape[0]
        finalpred = np.empty(n)
        predeachtime = np.empty([self.treeNum,n])
        for i in xrange(self.treeNum):
            predeachtime[i] = self.forest[i].predict_proba(test)[:,1]

        for i in xrange(n):
            finalpred[i] = np.mean(predeachtime[:,i])
        
        return finalpred


# In[2]:

import pandas as pd
import numpy as np
from Bagging import Bagging
from sklearn import preprocessing
from sklearn.utils import resample
from sklearn.tree import DecisionTreeClassifier 


# In[3]:

train = pd.DataFrame(pd.read_csv('revised_train1.csv'))
test = pd.DataFrame(pd.read_csv('revised_test1.csv'))
bidder = pd.read_csv('bidder.csv')


# In[4]:

number = preprocessing.LabelEncoder()
train['payment_account_prefix_same_as_address_prefix'] = number.fit_transform(train.payment_account_prefix_same_as_address_prefix)
train = np.array(train)
test = np.array(test)


# In[5]:

ft = myRF(1000)


# In[6]:

ft.fit(train)


# In[7]:

out = ft.predictprob(test)


# In[8]:

bidder['prediction'] = out


# In[9]:

bidder.to_csv("randomforestoutput.csv",index = False)

