require(grDevices) # for colours

tbl_dtls <- read.table(file="dtls.0.dur")
val_dtls <- tbl_dtls[1]

tbl_dtls_simplified <- read.table(file="dtls-simplified.0.dur")
val_dtls_simplified <- tbl_dtls_simplified[1]

print(summary(val_dtls))
print(summary(val_dtls_simplified))
