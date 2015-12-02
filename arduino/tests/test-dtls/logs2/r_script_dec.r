require(grDevices) # for colours

tbl_dtls <- read.table(file="dtls.0.dec")
val_dtls <- tbl_dtls[1]

tbl_dtls_simplified <- read.table(file="dtls-simplified.0.dec")
val_dtls_simplified <- tbl_dtls_simplified[1]

#print(summary(val_dtls))
#print(summary(val_dtls_simplified))

#data <- data.frame(Stat11=val_dtls, Stat12=val_dtls_simplified)

#tN <- table(Ni <- stats::rpois(100, lambda = 5))
#r <- barplot(tN, col = rainbow(20))

#r <- barplot(height=30, data=data, col = rainbow(20))

#x1 <- t(tbl_dtls)
#x2 <- t(tbl_dtls_simplified)

x1 <- c(val_dtls)
x2 <- c(val_dtls_simplified)

#r <- barplot(x1,x2, col = c("red", "blue"))

data<-data.frame(Stat11=x1,
                 Stat12=x2)

#J <- data.frame(x1, x2)
#boxplot(data)
boxplot(data, las = 2
        , ylab = "durée déchiffrement d'un paquet (µs)", xlab = "Versions de DTLS"
        , names = c("DTLS", "DTLS\nsimplifié")
        , col=c("magenta", "cyan")
        )
