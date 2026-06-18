import tools
import parser


def main(argv):
    raw_cpp = tools.run_cppcheck(argv[1])
    raw_clang = tools.run_clang_sa(argv[1])
    raw_infer = tools.run_infer(argv[1])

    cpp_out = parser.process_cpp(raw_cpp) 
    clang_out = parser.process_clang(raw_clang)
    infer_out = parser.process_infer(raw_infer)

    print(argv[1])
    print("CPPCheck findings:")
    for finding in cpp_out:
        print(finding.category)

    print("Clang SA findings:")
    for finding in clang_out:
        print(finding.category)

    print("Infer findings:")
    for finding in infer_out:
        print(finding.category)
    print("-------------------------")


if __name__ == "__main__":
    import sys

    main(sys.argv)
