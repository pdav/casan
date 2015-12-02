require(grDevices) # for colours

tbl_dtls <- read.table(file=commandArgs(TRUE)[1])
val_dtls <- tbl_dtls[1]

tbl_dtls_simplified <- read.table(file=commandArgs(TRUE)[2])
val_dtls_simplified <- tbl_dtls_simplified[1]

tbl_dtls_none <- read.table(file=commandArgs(TRUE)[3])
val_dtls_none <- tbl_dtls_none[1]

print(summary(val_dtls))
print(summary(val_dtls_simplified))
print(summary(val_dtls_none))
