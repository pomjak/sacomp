import tools


def main(argv):
    result = [None] * 3
    result[0] = tools.run_cppcheck(argv[1])
    result[1] = tools.run_clang(argv[1])
    result[2] = tools.run_infer(argv[1])

    print("cppcheck result:", result[0])
    print("clang result:", result[1])
    print("infer result:", result[2])


if __name__ == "__main__":
    import sys

    main(sys.argv)
