#!/bin/fish

for arg in $argv
	./cribbage-solitaire-solver $arg | tee $arg.out | tail -n 1
end
