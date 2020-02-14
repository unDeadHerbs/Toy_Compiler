CC          = clang
NONWARNINGS = -Wno-padded -Wno-missing-prototypes
WARNINGS    = -pedantic-errors -Weverything -Wno-dangling-else -Werror -fno-builtin $(NONWARNINGS)
LIBARYFLAGS = 
CCFLAGS     = -std=c89 $(WARNINGS) $(LIBARYFLAGS) -g $(SANS)

.PHONY:all
all: format TAGS main.bin

# generate the etags file
TAGS:
	@rm -f TAGS
	@git ls-files|grep "[.][ch]$$"|xargs -r etags -a
	@echo "Generated Tags"

# use the etags file to find all excicutables
main.bin: main.o parser.o tokenizer.o
	$(CC) $(CCFLAGS) $(LIBARYFLAGS) $^ -o $@

%.o: %.c %.h
	$(CC) $(CCFLAGS) $(LIBARYFLAGS) -c $< -o $@

# emacs flycheck-mode
.PHONY:check-syntax csyntax
check-syntax: csyntax
csyntax:
	$(CC) $(CCFLAGS) -c ${CHK_SOURCES} -o /dev/null

.PHONY: clean
clean:
	rm -rf -- *.o *.bin .d/

.PHONY: format
format:
	@git ls-files|egrep '.*[.](c|h)$$'|xargs clang-format -i
	@echo "reformatted code"


.PHONY: spell
spell:
	@echo " - Searching for Non-words in Code files -"
	@git ls | cut -f 2 | egrep "[.][ch]$$" | \
	 while read f; do \
		w=`cat $$f | aspell list --camel-case | sort | uniq | awk '{ if(length($$1)>4)print $$1 } '` ; \
		[ -z "$$w" ] || echo "$$f :: $$w" | xargs ; \
	 done

# This should be remade into a clang tool that removes strings
.PHONY: histogram
histogram:
	@echo " - Building a histogram of used words - "
	@git ls | cut -f 2 | egrep "[.][ch]$$" | \
	 while read f; do cat $$f; done | \
	 sed 's/\(\s\|[]({,<=>;})[*/+-]\|[0-9]\)/\n/g'|grep -v "^$$"|\
	 sort|uniq -c|sort -gr|head

include $(wildcard $(DEPDIR)/*.d)
include $(wildcard *.d)

