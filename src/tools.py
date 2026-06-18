# wrapper around tools
import json
import subprocess
import xml.etree.ElementTree as ET
import tempfile
import plistlib


def run_cppcheck(file) -> list[ET.Element]:
    p = subprocess.run(
        ["cppcheck", "--xml", "--xml-version=2", file],
        capture_output=True,
        text=True,
    )

    root = ET.fromstring(p.stderr)

    return root.findall(".//error")


def run_clang_sa(file) -> dict:
    with tempfile.NamedTemporaryFile(suffix=".plist") as report:
        subprocess.run(
            [
                "clang",
                "--analyze",
                "-Xanalyzer",
                "-analyzer-output=plist",
                "-o",
                report.name,
                file,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        return plistlib.load(open(report.name, "rb"))


def run_infer(file) -> dict:
    subprocess.run(["infer", "run", "--", "clang++","-std=c++17","-c", file],capture_output=True, text=True)

    #infer automatically stores the report in infer-out folder
    return json.load(open(f"infer-out/report.json", "r"))
