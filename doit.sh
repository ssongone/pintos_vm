# source ./activate
# make clean
# cd userprog
# # make check

# # cd userprog
# # make clean
# make
# # cd build
# # source ../../activate
# # pintos --gdb -- run priority-donate-chain
# # pintos -- run priority-donate-chain
# pintos --gdb --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
# # pintos --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
# # make
# # pintos -- -q run 'args-single onearg'


cd userprog
make clean
make
cd build
source ../../activate
# pintos --gdb -- run priority-donate-chain
# pintos -- run priority-donate-chain
# pintos --gdb --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
pintos --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
# pintos -- -q run 'args-single onearg'






