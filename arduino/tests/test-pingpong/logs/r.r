#!env R

inp11 <- scan("11", list(ts=0))
inp15 <- scan("15", list(ts=0))
inp20 <- scan("20", list(ts=0))
inp25 <- scan("25", list(ts=0))

boxplot(inp11$ts, inp15$ts, inp20$ts, inp25$ts)
