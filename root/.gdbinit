set can-use-hw-watchpoints 0
define asst0
dir ~/cs3231/asst0-src/kern/compile/ASST0
target remote unix:.sockets/gdb
b panic
end

set can-use-hw-watchpoints 0
define asst1
dir ~/cs3231/asst1-src/kern/compile/ASST1
target remote unix:.sockets/gdb
b panic
end

set can-use-hw-watchpoints 0
define ASST2
dir ~/cs3231/asst2-src/kern/compile/ASST2
target remote unix:.sockets/gdb
b panic
end

set can-use-hw-watchpoints 0
define asst3
dir ~/cs3231/asst3-src/kern/compile/ASST3
target remote unix:.sockets/gdb
b panic
end
