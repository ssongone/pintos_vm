cd userprog
make clean
make
cd build
source ../../activate
# pintos --gdb -- run priority-donate-chain
# pintos -- run priority-donate-chain

# pintos -- run priority-condvar
# pintos --gdb --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
# pintos --fs-disk=10 -p tests/userprog/open-missing:open-missing -- -q   -f run open-missing

# pintos --fs-disk=10 -p tests/userprog/open-normal:open-normal -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run open-normal
#pintos -fs-disk=10 -p tests/userprog/open-boundary:open-boundary -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run open-boundary
# --fs-disk=10 -p tests/userprog/open-twice:open-twice -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run open-twice

# pintos --fs-disk=10 -p tests/userprog/close-twice:close-twice -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run close-twice
# pintos --fs-disk=10 -p tests/userprog/read-normal:read-normal -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run read-normal

# pintos --fs-disk=10 -p tests/userprog/read-bad-ptr:read-bad-ptr -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run read-bad-ptr
# --fs-disk=10 -p tests/userprog/read-boundary:read-boundary -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run read-boundary
# --fs-disk=10 -p tests/userprog/read-zero:read-zero -p ../../tests/userprog/sample.txt:sample.txt -- -q   -f run read-zero

# pintos --fs-disk=10 -p tests/userprog/fork-once:fork-once -- -q   -f run fork-once

# pintos --fs-disk=10 -p tests/userprog/exec-once:exec-once -p tests/userprog/child-simple:child-simple -- -q   -f run exec-once

# pintos --fs-disk=10 -p tests/userprog/exec-missing:exec-missing -- -q   -f run exec-missing
# pintos --fs-disk=10 -p tests/userprog/wait-killed:wait-killed -p tests/userprog/child-bad:child-bad -- -q   -f run wait-killed

# --fs-disk=10 -p tests/userprog/rox-child:rox-child -p tests/userprog/child-rox:child-rox -- -q   -f run rox-child

# pintos --fs-disk=10 -p tests/userprog/rox-simple:rox-simple -- -q   -f run rox-simple
# pintos   --fs-disk=10 -p tests/filesys/base/syn-read:syn-read -p tests/filesys/base/child-syn-read:child-syn-read -- -q   -f run syn-read 

# pintos --fs-disk=10 -p tests/userprog/create-empty:create-empty -- -q   -f run create-empty 

# pintos  --fs-disk=10 -p tests/userprog/bad-read:bad-read -- -q   -f run bad-read 
pintos -v -k -T 600 -m 20 --fs-disk=10 -p tests/userprog/no-vm/multi-oom:multi-oom -- -q  -f run multi-oom

# pintos -v -k -T 300 -m 20   --fs-disk=10 -p tests/filesys/base/syn-read:syn-read -p tests/filesys/base/child-syn-read:child-syn-read -- -q   -f run syn-read