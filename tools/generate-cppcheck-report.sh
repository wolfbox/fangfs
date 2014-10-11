#!/usr/bin/env sh
mkdir -p reports/cppcheck

cppcheck --enable=all --inconclusive ./src/ --xml 2> ./cppcheck.xml
cat cppcheck.xml | cppcheck-htmlreport --report-dir=reports/cppcheck
rm cppcheck.xml
