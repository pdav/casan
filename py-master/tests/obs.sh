URL=http://localhost:8004/obs
TOK=bla
#RES=169/t1
RES=170/v1

H=hdr.out

# $1 = msg
# $2 = request
run ()
{
    echo "$1"
    R=$(curl --request "$2" --dump-header $H --silent $HDRFILE --data-ascii "" $URL/$TOK/$RES)
    echo "  $R"
    sed 's/^/    /' $H
}

run "Testing a non existent observer:" GET
run "Activate an observer:" POST
run "Get the current observer value (not ready):" GET
sleep 5
run "Get the current observer value (ok):" GET
run "Delete an observer:" DELETE
