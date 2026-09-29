#!/usr/bin/env python3
"""mutate.py FILE OLD NEW -- replace exactly one occurrence; exit 3 if absent."""
import sys
path, old, new = sys.argv[1], sys.argv[2], sys.argv[3]
text = open(path).read()
if text.count(old) != 1:
    sys.exit(3)
open(path, "w").write(text.replace(old, new))
