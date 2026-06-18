# parsing and normalization of the output of the tools

from dataclasses import dataclass

CATEGORY_MEMORY_LEAK = "MEMORY_LEAK"
CATEGORY_DOUBLE_FREE = "DOUBLE_FREE"
CATEGORY_USE_AFTER_FREE = "USE_AFTER_FREE"
CATEGORY_NULL_DEREFERENCE = "NULL_DEREFERENCE"
CATEGORY_UNINITIALIZED_VALUE = "UNINITIALIZED_VALUE"


NORMALIZED_CATEGORIES = {
    # cppcheck
    "deallocuse": CATEGORY_USE_AFTER_FREE,
    "memleak": CATEGORY_MEMORY_LEAK,
    "doubleFree": CATEGORY_DOUBLE_FREE,
    "nullPointer": CATEGORY_NULL_DEREFERENCE,
    "nullPointerOutOfMemory": CATEGORY_NULL_DEREFERENCE,
    "uninitvar": CATEGORY_UNINITIALIZED_VALUE,
    "uninitStructMember": CATEGORY_UNINITIALIZED_VALUE,
    # clang-sa
    "Use-after-free": CATEGORY_USE_AFTER_FREE,
    "Memory leak": CATEGORY_MEMORY_LEAK,
    "Double free": CATEGORY_DOUBLE_FREE,
    "Null pointer dereference": CATEGORY_NULL_DEREFERENCE,
    "Dereference of null pointer": CATEGORY_NULL_DEREFERENCE,
    "Result of operation is garbage or undefined": CATEGORY_UNINITIALIZED_VALUE,
    "Uninitialized argument value": CATEGORY_UNINITIALIZED_VALUE,
    # infer
    "MEMORY_LEAK_C": CATEGORY_MEMORY_LEAK,
    "USE_AFTER_FREE": CATEGORY_USE_AFTER_FREE,
    "USE_AFTER_DELETE": CATEGORY_USE_AFTER_FREE,
    "DOUBLE_FREE": CATEGORY_DOUBLE_FREE,
    "NULLPTR_DEREFERENCE": CATEGORY_NULL_DEREFERENCE,
    "PULSE_UNINITIALIZED_VALUE": CATEGORY_UNINITIALIZED_VALUE,
}


@dataclass
class Finding:
    tool: str
    category: str  # USE_AFTER_FREE
    severity: str  # ERROR/WARNING
    file: str
    line: int
    column: int
    rule: str  # deallocuse, unix.Malloc, USE_AFTER_FREE
    message: str
    cwe: str | None
    function: str | None


def process_cpp(raw_cpp):
    return [normalize_cppcheck(x) for x in raw_cpp]


def process_clang(raw_clang):
    return [normalize_clang(x, raw_clang["files"]) for x in raw_clang["diagnostics"]]


def process_infer(raw_infer):
    return [normalize_infer(x) for x in raw_infer]


def normalize_cppcheck(error):
    loc = error.find("location")

    rule = error.attrib["id"]

    return Finding(
        tool="cppcheck",
        category=NORMALIZED_CATEGORIES.get(rule, rule),
        severity=error.attrib["severity"].upper(),
        file=(loc.attrib["file"] if loc is not None else ""),
        line=(int(loc.attrib["line"]) if loc is not None else -1),
        column=(int(loc.attrib.get("column", 0)) if loc is not None else -1),
        rule=rule,
        message=error.attrib["msg"],
        cwe=error.attrib.get("cwe"),
        function=None,
    )


def normalize_clang(diag, files):
    loc = diag["location"]

    return Finding(
        tool="clang-sa",
        category=NORMALIZED_CATEGORIES.get(diag["type"], diag["type"]),
        # Clang SA plist doesn't expose severity directly, diagnostics are generally warnings.
        severity="WARNING",
        file=files[loc["file"]],
        line=loc["line"],
        column=loc["col"],
        rule=diag["check_name"],
        message=diag["description"],
        cwe=None,
        function=diag.get("issue_context"),
    )


def normalize_infer(issue):
    return Finding(
        tool="infer",
        category=NORMALIZED_CATEGORIES.get(issue["bug_type"], issue["bug_type"]),
        severity=issue["severity"].upper(),
        file=issue["file"],
        line=issue["line"],
        column=issue["column"],
        rule=issue["bug_type"],
        message=issue["qualifier"],
        cwe=None,
        function=issue["procedure"],
    )
