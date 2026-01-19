#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Task 3.8 Validation: Update Method Count Documentation ==="

# Check that README.md has updated method count
echo "Checking README.md..."
grep -q "128+ built-in methods" README.md || { echo "ERROR: README.md not updated with 128+ methods"; exit 1; }

# Check that METHODS_REFERENCE.md has updated counts
echo "Checking METHODS_REFERENCE.md..."
grep -q "String Methods (27)" METHODS_REFERENCE.md || { echo "ERROR: String methods count not updated"; exit 1; }
grep -q "Number Methods (39)" METHODS_REFERENCE.md || { echo "ERROR: Number methods count not updated"; exit 1; }
grep -q "Array Methods (18)" METHODS_REFERENCE.md || { echo "ERROR: Array methods count not updated"; exit 1; }
grep -q "Collection Methods (24)" METHODS_REFERENCE.md || { echo "ERROR: Collection methods section not found"; exit 1; }
grep -q "Option/Result Methods (18)" METHODS_REFERENCE.md || { echo "ERROR: Option/Result methods section not found"; exit 1; }

# Check that agent_prompt.md has updated counts
echo "Checking agent_prompt.md..."
grep -q "128+ methods implemented" ../internal-docs/agent_prompt.md || { echo "ERROR: agent_prompt.md not updated"; exit 1; }

# Check that FINAL_STATUS_REPORT.md has updated counts
echo "Checking FINAL_STATUS_REPORT.md..."
grep -q "128+ methods working" ../internal-docs/FINAL_STATUS_REPORT.md || { echo "ERROR: FINAL_STATUS_REPORT.md not updated"; exit 1; }

# Count actual methods in types.c
echo "Verifying method count..."
method_count=$(grep "^    {" src/types.c | grep -v "NULL" | wc -l | tr -d ' ')
echo "Actual methods implemented: $method_count"
[ "$method_count" -ge 128 ] || { echo "ERROR: Method count is less than 128"; exit 1; }

echo "✓ Task 3.8 Complete - Documentation updated with current method counts"
exit 0