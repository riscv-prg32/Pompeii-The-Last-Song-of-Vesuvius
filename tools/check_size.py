#!/usr/bin/env python3
import pathlib,sys
p=pathlib.Path(sys.argv[1]); n=p.stat().st_size
print(f'{p.name}: {n} bytes / 131072 ({n/131072:.1%})')
sys.exit(0 if n<=131072 else 1)
