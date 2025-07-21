#!/bin/bash

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

CASES_DIR="tests/cases"
BUILD_DIR="builds"

TEST_EXECUTABLES=(
    "test_tokenizer"
    "test_parser"
    "test_html_generator"
    "test_template_engine"
)

EXPECTED_EXTENSIONS=(
    "tokens.expected"
    "ast.expected"
    "html.expected"
    "final.html"
)

echo -e "${YELLOW}===========================================${NC}"
echo -e "${YELLOW}  Updating all test case expected files... ${NC}"
echo -e "${YELLOW}===========================================${NC}"
echo ""

for test_file in $CASES_DIR/*.md; do
    if [ ! -f "$test_file" ]; then continue; fi

    base_name=$(basename "$test_file" .md)
    echo "Processing case: $base_name.md"

    for i in ${!TEST_EXECUTABLES[@]}; do
        executable="$BUILD_DIR/${TEST_EXECUTABLES[$i]}"
        extension="${EXPECTED_EXTENSIONS[$i]}"
        expected_file="$CASES_DIR/$base_name.$extension"

        if [ -f "$executable" ]; then
            printf "  -> Generating new '%s'..." "$extension"
            $executable "$test_file" > "$expected_file"
            printf "${GREEN} Done.${NC}\n"
        else
            echo "  -> Executable not found, skipping: $executable"
        fi
    done
    echo ""
done

echo -e "${GREEN}===========================================${NC}"
echo -e "${GREEN}      All expected files are updated!      ${NC}"
echo -e "${GREEN}===========================================${NC}"
