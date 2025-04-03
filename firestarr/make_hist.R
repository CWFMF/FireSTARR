
BASE_DIR <- "."
result <- ""

input <- paste0(BASE_DIR, "\\Data\\output\\sizes_252_2017-09-09.csv")
values <- read.csv(input)$X
#~ hist(values, main="", xlab="Size (ha)", breaks=seq(0,35000,l=1000))

#~ hist(values, main="", xlab="Size (ha)", breaks=seq(0,35000,l=10))

#~ hist(values, main="", xlab="Size (ha)", breaks=seq(0,35000,l=1000))
#~ hist(values, main="", xlab="Size (ha)", breaks=seq(0,35000,l=10000))
#~ hist(values, main="", xlab="Size (ha)", breaks=seq(0,35000,l=100))
#~ hist(values, main="", xlab="Size (ha)", breaks=seq(0,35000,l=35000))

#~ boxplot(values)
#~ boxplot(values, horizontal=TRUE)
#~ boxplot(log10(values), horizontal=TRUE)

P <- ecdf(values)
#~ plot(P)

p = P(seq(0, max(values), by=1))
plot(p, xlab="Size (ha)", ylab="Cumulative Probability")
#~ v <- 10^seq(1,5)
#~ abline(h=P(v), v=v)

show_prob <- function(x, col) {
    y <- P(x)
    segments(x, -100000, x, y, col=col, lwd=2)
    segments(-100000, y, x, y, col=col, lwd=2)
    #~ axis(1, at=x, labels=paste0(x, 'ha'), pos=y, xaxs="i", yaxs="i")
    axis(1, at=x, labels=paste0(x, 'ha'), pos=y, tick=FALSE)
}

for (value in 0:5)
{
    show_prob(10^value, value + 2)
}
show_prob(max(values), 6 + 2)

#~ v <- 10 ^ seq(0, 4)
#~ axis(1, at=v, labels=paste0(v, 'ha'), pos=1)

