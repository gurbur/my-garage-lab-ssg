GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

CASES_DIR="tests/cases"
BUILD_DIR="builds"
TEMP_DIR=$(mktemp -d)

TOKENIZER_TEST="$BUILD_DIR/test_tokenizer"
PARSER_TEST="$BUILD_DIR/test_parser"

total_tests=0
passed_tests=0

echo "============================"
echo "  Running Tokenizer Tests   "
echo "============================"

for test_file in $CASES_DIR/*.md; do
    base_name=$(basename "$test_file" .md)
    expected_file="$CASES_DIR/$base_name.tokens.expected"
    actual_file="$TEMP_DIR/$base_name.tokens.actual"

    if [ -f "$expected_file" ]; then
				total_tests=$((total_tests + 1))

        printf "Testing %-30s ... " "$base_name.md"

        $TOKENIZER_TEST "$test_file" > "$actual_file"

        if diff -u "$expected_file" "$actual_file" > /dev/null; then
            printf "${GREEN}PASS${NC}\n"
						passed_tests=$((passed_tests + 1))
        else
            printf "${RED}FAIL${NC}\n"
						diff -u "$expected_file" "$actual_file"
        fi
    fi
done

echo ""

echo "============================"
echo "    Running Parser Tests    "
echo "============================"

for test_file in $CASES_DIR/*.md; do
    base_name=$(basename "$test_file" .md)
    expected_file="$CASES_DIR/$base_name.ast.expected"
    actual_file="$TEMP_DIR/$base_name.ast.actual"

    if [ -f "$expected_file" ]; then
        total_tests=$((total_tests + 1))
        printf "Testing %-22s ... " "$base_name.md"

        $PARSER_TEST "$test_file" > "$actual_file"

        if diff -u "$expected_file" "$actual_file" > /dev/null; then
            printf "${GREEN}PASS${NC}\n"
            passed_tests=$((passed_tests + 1))
        else
            printf "${RED}FAIL${NC}\n"
            diff -u "$expected_file" "$actual_file"
        fi
    fi
done

echo ""

echo "============================"
echo "      Test Summary"
echo "============================"
if [ $passed_tests -eq $total_tests ]; then
    echo -e "${GREEN}All $total_tests tests passed! ✔${NC}"
else
    echo -e "${RED}$((total_tests - passed_tests)) of $total_tests tests failed! ✖${NC}"
fi
echo "============================"

rm -rf "$TEMP_DIR"

if [ $passed_tests -ne $total_tests ]; then
    exit 1
fi
