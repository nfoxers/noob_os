set -euo pipefail

src="$1"
dst="$2"
img="$3"

find "$src" -type d | while read -r d; do
  e2mkdir "$img":"$dst/${d#$src/}"
done

find "$src" -type f | while read -r f; do
  e2cp "$f" "$img":"$dst/${f#$src/r}"
done