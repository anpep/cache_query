cache_query: cache_query.c

test: cache_query ; python3 measure.py

clean: ; rm -f cache_query

.PHONY: clean test

