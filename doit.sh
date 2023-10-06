cd userprog
make clean
make
cd build
source ../../activate
# pintos --gdb -- run priority-donate-chain
# pintos -- run priority-donate-chain
# pintos --gdb --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
# pintos --fs-disk=10 -p tests/userprog/open-missing:open-missing -- -q   -f run open-missing

pintos --fs-disk=10 -p tests/userprog/open-normal:open-normal -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run open-normal
#pintos -fs-disk=10 -p tests/userprog/open-boundary:open-boundary -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run open-boundary
# --fs-disk=10 -p tests/userprog/open-twice:open-twice -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run open-twice