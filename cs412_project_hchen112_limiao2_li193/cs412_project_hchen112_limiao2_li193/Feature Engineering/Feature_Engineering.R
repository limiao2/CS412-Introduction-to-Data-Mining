##read three csv file##
newtrain = read.csv("newtrain.csv", header = T)
newtest = read.csv("newtest.csv", header = T)
sample = read.csv("sampleSubmission.csv", header = T)

##check how many missing values in train and test data set##
sum(is.na(newtrain))
sum(is.na(newtest))

##delete first two columns##
newtrain = newtrain[,c(-1,-2)]
newtest = newtest[,c(-1,-2)]

##missing value imputation for train set##
str(newtrain,list.len=ncol(newtrain))
which(colnames(newtrain)=="most_common_country")
newtrain=newtrain[,-201]

pred=matrix(0,dim(newtrain)[1],dim(newtrain)[2])

for (i in 1:dim(newtrain)[2]){
      if (sum(is.na(newtrain[,i]))!=0){
          tree.impute=tree(newtrain[,i]~.,data=newtrain)
          pred[,i]=predict(tree.impute,newtrain)
 }
}

for (i in 1:dim(newtrain)[2]){
      if (sum(is.na(newtrain[,i]))!=0){
        for (j in 1:dim(newtrain)[1]){
          if (is.na(newtrain[j,i])==TRUE){
            newtrain[j,i]=pred[j,i]
        }
     }
  }
}

################################################





##missing value imputation for test data##
str(newtest,list.len=ncol(newtest))
which(colnames(newtest)=="most_common_country")
newtest=newtest[,-200]

pred1=matrix(0,dim(newtest)[1],dim(newtest)[2])

for (i in 1:dim(newtest)[2]){
      if (sum(is.na(newtest[,i]))!=0){
          tree.impute=tree(newtest[,i]~.,data=newtest)
          pred1[,i]=predict(tree.impute,newtest)
 }
}

for (i in 1:dim(newtest)[2]){
      if (sum(is.na(newtest[,i]))!=0){
        for (j in 1:dim(newtest)[1]){
          if (is.na(newtest[j,i])==TRUE){
            newtest[j,i]=pred1[j,i]
        }
     }
  }
}

###########################################################

#####randomForest model for classification#####
library(randomForest)
rf.robots = randomForest(outcome ~ ., data = newtrain, mtry = sqrt(408), ntree = 500, importance = TRUE)
points=importance(rf.robots)

##delete features which make small contribution to model##
sum(points[,1]==0)
c=rep(0,dim(newtrain)[2])
d=rep(0,dim(newtest)[2])
for(i in 1:dim(points)[1]){
   if(points[i,1]==0){
    c[i]=i+1
    d[i]=i
}else{c[i]=0
      d[i]=0}
}

revised_train=newtrain[,-c]
revised_test=newtest[,-d]

##run randomForest using the selected features##
rf.robots = randomForest(outcome ~ ., data = revised_train, mtry = sqrt(305), ntree = 500, importance = TRUE)
yhat.rf = predict(rf.robots, newdata = revised_test)

##output the final results##
filepath = "E:/Stat/cs 412/final project/Mid_Submission.csv"
output=sample$bidder_id
myout = data.frame(bidder_id=output, prediction = yhat.rf)
write.csv(myout, file=filepath,  row.names=FALSE, quote = FALSE)