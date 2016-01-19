d = read.csv("/Users/Li/Desktop/train.csv",header = TRUE)
summary(d)
d$size

I <-function(x,y)
{
  sum = x+y
  aa = 0
  if (x==0 || y==0){
    result = 0}
  else{
  result = -(x/sum)*log2(x/sum) - (y/sum)*log2(y/sum)}
  return (result)
}

Gain <-function(infod,infoa){
  result = infod - infoa
  return (result)
}


infod = 1
# Infogpa
infogpa = 3/12 * I(3,0) + 5/12 * I(3,2) + 4/12 * I(0,4)
# Inf0 univ
infouniv = 5/12*I(3,2) + 3/12*I(2,1) + 4/12*I(1,3)
#Info published
infopublished = 5/12*I(3,2) + 7/12*I(3,4)
#info recommendation
inforecom = 8/12*I(5,3) + 4/12*I(1,3)

gaingpa = Gain(infod,infogpa)
gainuniv = Gain(infod,infouniv)
gainpublished = Gain(infod,infopublished)
gainrecom = Gain(infod,inforecom)

head(d)
5*3*4*8
vars = names(d) %in% c("id")
d = d[!vars]
d$GPA = factor(d$GPA)
library(tree)
trees<-tree(accepted~.,d)
plot(trees)
text(trees ,pretty =0)

summary(trees)

I(1,0)
I(0,1)

(0.06944444 + 0.01388889)/2

a = 0.5*0.17*0.5*0.17
b = a*0.5
c = 0.33*0.67*0.5*0.5
b/(a+c)

a1 = 1/2 * 1/6 * 1/2 * 1/6
b1 = a1/2
c1 = 1/3 * 1/2 * 2/3 * 1/2
b1/(a1+c1)

log(17) - log(5) - log(9)

x = c(1,2.2,2.7,0.5,1.2,1.5)
y = c(0.5,1,2,1.5,2.3,2.7)
label = c('1','1','1','-1','-1','-1')
df = data.frame(x,y,label)
summary(df)
plot(x,y)
abline(v=1.5,colour = 'red')
library(ggplot2)
p<-ggplot(df,aes(x,y))
p<-p+aes(shape = label) + geom_point(aes(colour = label))
p<-p+geom_vline(xintercept = 1.5,color = 'red')+geom_abline(intercept = 1,slope=0,color = 'blue')+geom_abline(intercept = 2,slope=0,color = 'yellow')


p<-p+geom_rect(aes(xmin=0,xmax = 3,ymin=0,ymax=1),alpha = 0.1)+geom_rect(aes(xmin=1.5,xmax = 3,ymin=1,ymax=2),alpha = 0.1)

p + scale_x_continuous(expand = c(0, 0)) + scale_y_continuous(expand = c(0, 0))

