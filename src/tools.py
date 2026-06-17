# wrapper around tools
import subprocess
import time


def run_command(command, timeout=None):
    start_time = time.time()
    result = subprocess.run(command, capture_output=True, text=True)
    end_time = time.time() - start_time

    return {
        "stdout": result.stdout,
        "stderr": result.stderr,
        "returncode": result.returncode,
        "time": end_time,
    }

def run_cppcheck(file):
    command = ["cppcheck", "--enable=all", "--std=c++17", file]
    return run_command(command)

def run_clang(file):
    command = ["clang", "--analyze", "--std=c++17", file]
    return run_command(command)

def run_infer(file):
    command = ["infer", "run", "--", "clang++", "-std=c++17", "-c", file]
    return run_command(command)
