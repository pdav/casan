require(grDevices) # for colours

tbl_dtls <- read.table(file="screenlog-dtls.0.enc")
val_dtls <- tbl_dtls[1]

tbl_dtls_simplified <- read.table(file="screenlog-simplified.0.enc")
val_dtls_simplified <- tbl_dtls_simplified[1]

print(summary(val_dtls))
print(summary(val_dtls_simplified))
