x = [0.69,-1.31,0.39,0.05,1.29,0.49,0.19,-0.81,-0.31,0.71];
y = [0.89,-1.11,0.59,0.45,1.19,0.69,0.25,-0.71,-0.21,0.71];

n = 10;
sum = 0;
xmean = mean(x);
ymean = mean(y);

xstd = std(x);
ystd = std(y);



for m = 1 : 10
    sum = sum + x(m)*y(m);
end
%A
r = (sum - n*xmean*ymean)/n*xstd*ystd;

%C
xx = x- xmean;
yy = y-ymean;

k = [xx;yy];
ck = (X*transpose(X))/n;

%cc = cov(xx,yy)
[v,d] = eig(ck);

%scatter(xx,yy);
%axis([-2 2 -2 2])
%plot(v)

newx = linspace(-2,2);
newy = (v(2,1)/v(1,1))*newx;
newyy = (v(2,2)/v(1,2))*newx;
plot(newx,newy)
hold on
plot(newx,newyy)
hold on
axis([-2 2 -2 2])
%newyy = (v(2,2)/v(1,2))*newx;

scatter(x,y,'b')

hold on
scatter(0.05,0.45,'filled','d')
hold on
scatter(0.49,0.69,'filled','d')

aprime = [-0.7253,-0.6885]*(0.3461);
bprime = [-0.7253,-0.6885]*0.8304;

scatter(aprime(1),aprime(2),'filled','d')
hold on
scatter(bprime(1),bprime(2),'filled','d')
hold on

%scatter()
hold off