## metrics and evaluation


def check_findings(name, findings, expected):
    print(name)

    if not findings:
        print("No findings")
        return

    for finding in findings:
        if finding.category != expected:
            print(f"Unexpected category: {finding.category}")
        else:
            print("OK")
