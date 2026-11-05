#!/bin/sh
"$@"
STATUS=$?
case $0 in
  */*) ORIGIN=${0%/*} ;;
  *) ORIGIN=. ;;
esac
ORIGIN=$(CDPATH= cd -P "$ORIGIN" && pwd) || exit "$STATUS"
printf '\nProcess exited with status %d.\n' "$STATUS"
printf 'Press any key to close...\n'
"$ORIGIN/waitkey"
exit "$STATUS"
