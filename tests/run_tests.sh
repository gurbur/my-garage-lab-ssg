GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

CASES_DIR="tests/cases"
BUILD_DIR="builds"
TEMP_DIR=$(mktemp -d)

TOKENIZER_TEST="$BUILD_DIR/test_tokenizer"
PARSER_TEST="$BUILD_DIR/test_parser"
HTML_GENERATOR_TEST="$BUILD_DIR/test_html_generator"

total_tests=0
passed_tests=0

run_test_suite() {
    local test_suite_name="$1"
    local test_executable="$2"
    local expected_extension="$3"
    local actual_extension="$4"
    
    echo "========================================="
    printf "  Running %-26s Tests  \n" "$test_suite_name"
    echo "========================================="

    for expected_file in $CASES_DIR/*.$expected_extension; do
        if [ ! -f "$expected_file" ]; then continue; fi

        total_tests=$((total_tests + 1))
        
        local base_name=$(basename "$expected_file" .$expected_extension)
        local test_file="$CASES_DIR/$base_name.md"
        local actual_file="$TEMP_DIR/$base_name.$actual_extension"

        printf "Testing %-30s ... " "$base_name.md"

				$test_executable "$test_file" > "$actual_file"

        if diff -u --strip-trailing-cr "$expected_file" "$actual_file" > /dev/null; then
            printf "${GREEN}PASS${NC}\n"
            passed_tests=$((passed_tests + 1))
        else
            printf "${RED}FAIL${NC}\n"
            diff -u --strip-trailing-cr "$expected_file" "$actual_file"
        fi
    done
    echo ""
}

run_test_suite "Tokenizer"      "$TOKENIZER_TEST"       "tokens.expected" "tokens.actual"
run_test_suite "Parser"         "$PARSER_TEST"          "ast.expected"    "ast.actual"
run_test_suite "HTML Generator" "$HTML_GENERATOR_TEST"  "html.expected"   "html.actual"


echo "========================================="
echo "               Test Summary"
echo "========================================="
if [ $passed_tests -eq $total_tests ]; then
    echo -e "${GREEN}All $total_tests tests passed! ✔${NC}"
else
    echo -e "${RED}$((total_tests - passed_tests)) of $total_tests tests failed! ✖${NC}"
fi
echo "========================================="

rm -rf "$TEMP_DIR"

if [ $passed_tests -ne $total_tests ]; then
    exit 1
fi
