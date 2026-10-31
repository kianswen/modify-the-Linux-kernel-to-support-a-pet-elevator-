import argparse
import os
import re
import shutil
import subprocess
import sys

PART3_DIR = os.path.dirname(os.path.abspath(__file__))

TBL_LINES = (
    "548\tcommon\tstart_elevator\t\tsys_start_elevator\n"
    "549\tcommon\tissue_request\t\tsys_issue_request\n"
    "550\tcommon\tstop_elevator\t\tsys_stop_elevator\n"
)

PROTOTYPES = (
    "asmlinkage long sys_start_elevator(void);\n"
    "asmlinkage long sys_issue_request(int start_floor, int destination_floor, int type);\n"
    "asmlinkage long sys_stop_elevator(void);\n"
)


def read(path):
    with open(path) as f:
        return f.read()


def write(path, text):
    with open(path, "w") as f:
        f.write(text)


def copy_syscalls(kdir):
    # always recopy so edits to syscalls.c get picked up
    sys_dir = os.path.join(kdir, "syscalls")
    os.makedirs(sys_dir, exist_ok=True)
    shutil.copy(os.path.join(PART3_DIR, "syscalls.c"), sys_dir)
    write(os.path.join(sys_dir, "Makefile"), "obj-y := syscalls.o\n")
    print("copied syscalls.c and wrote syscalls/Makefile")


def add_table_entries(kdir):
    path = os.path.join(kdir, "arch/x86/entry/syscalls/syscall_64.tbl")
    text = read(path)
    if "sys_start_elevator" in text:
        print("syscall_64.tbl already done")
        return
    if not text.endswith("\n"):
        text += "\n"
    write(path, text + TBL_LINES)
    print("added 548-550 to syscall_64.tbl")


def add_prototypes(kdir):
    path = os.path.join(kdir, "include/linux/syscalls.h")
    text = read(path)
    if "sys_start_elevator" in text:
        print("syscalls.h already done")
        return
    # prototypes go right before the last #endif
    i = text.rfind("#endif")
    if i == -1:
        sys.exit("error: couldnt find #endif in syscalls.h")
    write(path, text[:i] + PROTOTYPES + text[i:])
    print("added prototypes to syscalls.h")


def add_core_y(kdir):
    path = os.path.join(kdir, "Makefile")
    text = read(path)
    match = re.search(r"^core-y\s*:=.*$", text, re.MULTILINE)
    if match is None:
        sys.exit("error: couldnt find the core-y := line in the kernel Makefile")
    line = match.group(0)
    if "syscalls/" in line:
        print("kernel Makefile already done")
        return
    write(path, text[:match.start()] + line.rstrip() + " syscalls/" + text[match.end():])
    print("added syscalls/ to core-y in kernel Makefile")


def run(cmd, kdir):
    print("+ " + " ".join(cmd))
    subprocess.run(cmd, cwd=kdir, check=True)


def build(kdir):
    # start from the running kernels config if theres no .config yet
    if not os.path.exists(os.path.join(kdir, ".config")):
        shutil.copy("/boot/config-" + os.uname().release, os.path.join(kdir, ".config"))
        run(["make", "olddefconfig"], kdir)
    run(["scripts/config", "--disable", "SYSTEM_TRUSTED_KEYS",
         "--disable", "SYSTEM_REVOCATION_KEYS"], kdir)
    run(["make", "olddefconfig"], kdir)
    run(["make", "-j" + str(os.cpu_count())], kdir)
    run(["make", "modules_install"], kdir)
    run(["make", "install"], kdir)
    print("done, reboot then check uname -r")


def main():
    parser = argparse.ArgumentParser(description="add the elevator syscalls to a kernel tree")
    parser.add_argument("kernel_dir", help="path to the kernel source, ex ~/linux-6.16.x")
    parser.add_argument("--build", action="store_true",
                        help="also compile and install the kernel")
    args = parser.parse_args()

    kdir = os.path.abspath(os.path.expanduser(args.kernel_dir))
    tbl = os.path.join(kdir, "arch/x86/entry/syscalls/syscall_64.tbl")
    if not os.path.isfile(os.path.join(kdir, "Makefile")) or not os.path.isfile(tbl):
        sys.exit("error: " + kdir + " doesnt look like a kernel source tree")

    copy_syscalls(kdir)
    add_table_entries(kdir)
    add_prototypes(kdir)
    add_core_y(kdir)

    if args.build:
        try:
            build(kdir)
        except subprocess.CalledProcessError as e:
            sys.exit("error: " + " ".join(e.cmd) + " failed")


if __name__ == "__main__":
    main()
